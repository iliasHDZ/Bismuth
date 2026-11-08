#include "WorkGroupScheduler.hpp"
#include "Renderer.hpp"
#include "../cl.hpp"

void SortedObjectBatch::bindBuffers() {
    if (!haveBuffersBeenCreated) {
        spriteRemapBuffer = Buffer::createStaticDraw(
            "SpriteRemapBuffer",
            inputSpriteIndicies.data(),
            inputSpriteIndicies.size() * sizeof(u32)
        );

        if (!spriteRemapBuffer)
            return;

        workGroupBuffer = Buffer::createStaticDraw(
            "WorkGroupBuffer",
            workGroups.data(),
            workGroups.size() * sizeof(WorkGroup)
        );

        if (!workGroupBuffer)
            return;

        drawCommandBuffer = Buffer::createGPUExclusive(
            "DrawCommandBuffer",
            workGroups.size() * sizeof(DrawCommand)
        );

        if (!drawCommandBuffer)
            return; // FIXME: Make sure that this shows an error and exits the level

        drawCommandBuffer->write(drawCommands.data(), drawCommandBuffer->getSize());

        preCulledSpritesBuffer->flushAllAndUnmap();

        inputSpriteIndicies.clear();
        workGroups.clear();
        drawCommands.clear();
        haveBuffersBeenCreated = true;
    }

    spriteRemapBuffer->bindAsStorageBuffer(SPRITE_REMAP_BUFFER_BINDING);
    workGroupBuffer->bindAsStorageBuffer(WORK_GROUP_BUFFER_BINDING);
    drawCommandBuffer->bindAsStorageBuffer(DRAW_COMMAND_BUFFER_BINDING);
    preCulledSpritesBuffer->bind();
}

WorkGroupScheduler::WorkGroupScheduler() {
    if (maxWorkGroupCount == -1) {
        maxWorkGroupCount = getGPUCoreCount();
        if (maxWorkGroupCount == -1)
            maxWorkGroupCount = 8000; // dummy value
        else
            maxWorkGroupCount = (double)maxWorkGroupCount * WORK_GROUPS_PER_GPU_CORES;
    }
}

WorkGroupScheduler::~WorkGroupScheduler() {
    if (thread.joinable())
        thread.join();
}

void WorkGroupScheduler::addSprite(
    GameObject* object,
    cocos2d::CCSprite* sprite,
    SpriteType type,
    u32 inputSpriteIndex,
    u32 triCount
) {
    spriteEntries.push_back({
        .zOrder = object->getObjectZOrder(),
        .entryIndex = (u32)spriteEntries.size(),
        .zLayer = object->getObjectZLayer(),
        .spriteSheet = ObjectUtils::getSpritesheetOfObject(object, type),
        .colorChannelId = (u16)ObjectUtils::getSpriteColorChannel(type, object, sprite),
        .inputSpriteIndex = inputSpriteIndex,
        .triCount = triCount,
        .groupCombinationIndex = Renderer::get()->getGroupManager().getGroupCombinationIndexForObject(object),
        .object = object
    });
}

SortedObjectBatch* WorkGroupScheduler::fetchSortedBatchForBlendStates(const ColorChannelBlendStates& blendStates) {
    cleanUpFinishedJobs();
    
    {
        std::scoped_lock lock(mutex);
        if (sortedBatchesPerBlendStates.contains(blendStates))
            return sortedBatchesPerBlendStates[blendStates].get();
    }

    addGenerationJob(blendStates);
    return nullptr;
}

void WorkGroupScheduler::cleanUpFinishedJobs() {
    std::scoped_lock lock(mutex);

    while (!finishedGenerationJobs.empty()) {
        GenerationJob* job = *finishedGenerationJobs.begin();
        finishedGenerationJobs.erase(job);

        for (auto it = currentGenerationJobs.begin(); it != currentGenerationJobs.end(); it++) {
            if (it->second.get() == job) {
                currentGenerationJobs.erase(it);
                break;
            }
        }
    }
}

void WorkGroupScheduler::addGenerationJob(const ColorChannelBlendStates& blendStates) {
    std::scoped_lock lock(mutex);
    if (currentGenerationJobs.contains(blendStates))
        return;

    auto job = std::make_unique<GenerationJob>(*this);

    job->blendStates = blendStates;
    job->preCulledSprites = PreCulledSprite::Buffer::create(spriteEntries.size(), GL_STATIC_DRAW);
    job->maxWorkGroupCount = maxWorkGroupCount;

    job->thread = std::thread(&GenerationJob::generateSortedBatchForBlendStates, job.get());

    currentGenerationJobs[blendStates] = std::move(job);
}

LayerKey WorkGroupScheduler::SpriteEntry::getFullLayerKey(const ColorChannelBlendStates& blendingStates) const {
    return { zLayer, spriteSheet, blendingStates[colorChannelId] };
}

void WorkGroupScheduler::GenerationJob::generateSortedBatchForBlendStates() {
    usize spriteCount;
    {
        std::scoped_lock lock(scheduler.mutex);
        spriteCount = scheduler.spriteEntries.size();
        loadAndSortSprites(scheduler.spriteEntries);

        if (spriteCount <= 0) {
            scheduler.sortedBatchesPerBlendStates[blendStates] = nullptr;
            scheduler.finishedGenerationJobs.insert(this);
            return;
        }
    }

    inputSpriteIndicies.clear();
    inputSpriteIndicies.reserve(spriteCount);

    u32 approxWorkGroupCount = std::min<u32>(spriteCount, maxWorkGroupCount);
    u32 approxSpritesPerWorkGroup = spriteCount / approxWorkGroupCount;

    workGroups.clear();
    workGroups.reserve(approxWorkGroupCount + 100);

    currentSpriteIndex = 0;

    u32 outIndex = 0;
    std::map<LayerKey, WorkGroupRange> workGroupRangePerLayerKey;

    for (auto& [key, layer] : sortedSpriteLayers) {
        // log::info("Process layer: {}", key.toString());
        u32 workGroupStartIndex = workGroups.size();
        generateWorkGroupsForLayer(key, approxSpritesPerWorkGroup, outIndex);
        u32 workGroupEndIndex = workGroups.size();

        workGroupRangePerLayerKey[key] = {
            .offset = workGroupStartIndex,
            .count  = workGroupEndIndex - workGroupStartIndex
        };

        // log::info("Layer processed {}: (index={}, size={})", key.toString(), workGroupStartIndex, workGroupEndIndex - workGroupStartIndex);
    }

    std::vector<DrawCommand> drawCommands;
    drawCommands.resize(workGroups.size());

    for (usize i = 0; i < workGroups.size(); i++) {
        drawCommands[i].baseInstance  = 0;
        drawCommands[i].instanceCount = 1;
        drawCommands[i].first = workGroups[i].culledIndexBufferOutputIndex * 3;
    }

    usize totalSize = inputSpriteIndicies.size() * sizeof(u32) +
                      workGroups.size() * sizeof(WorkGroup) +
                      drawCommands.size() * sizeof(DrawCommand) +
                      preCulledSprites->getByteSize();

    log::info("Generated SortedObjectBatch: workGroupCount={}, vramUsage={}", workGroups.size(), byteSizeToString(totalSize));

    auto sortedBatch = std::make_unique<SortedObjectBatch>();

    sortedBatch->workGroupCount = (u32)workGroups.size();
    sortedBatch->workGroupRangePerLayerKey = std::move(workGroupRangePerLayerKey);
    sortedBatch->haveBuffersBeenCreated = false; // Buffers can only be created on the main thread!
    sortedBatch->inputSpriteIndicies = std::move(inputSpriteIndicies);
    sortedBatch->workGroups = std::move(workGroups);
    sortedBatch->drawCommands = std::move(drawCommands);
    sortedBatch->preCulledSpritesBuffer = std::move(preCulledSprites);

    {
        std::scoped_lock lock(scheduler.mutex);

        scheduler.sortedBatchesPerBlendStates[blendStates] = std::move(sortedBatch);
        scheduler.finishedGenerationJobs.insert(this);
    }
}

void WorkGroupScheduler::GenerationJob::loadAndSortSprites(const std::vector<SpriteEntry>& sprites) {
    sortedSpriteLayers.clear();

    for (auto& sprite : sprites) {
        auto key = sprite.getFullLayerKey(blendStates);
        sortedSpriteLayers[key].push_back(&sprite);
    }

    for (auto& [key, layer] : sortedSpriteLayers) {
        std::sort(layer.begin(), layer.end(), [](const SpriteEntry* a, const SpriteEntry* b) {
            if (a->zOrder == b->zOrder) return a->entryIndex < b->entryIndex;
            return a->zOrder < b->zOrder;
        });
    }
}

void WorkGroupScheduler::GenerationJob::generateWorkGroupsForLayer(
    LayerKey layerKey,
    u32 spritesPerWorkGroup,
    u32& culledIndexBufferOutputIndex
) {
    assert(spritesPerWorkGroup > 0);

    const auto& sprites = sortedSpriteLayers[layerKey];

    u32 i = 0;
    while (i < sprites.size()) {
        u32 spritesToAdd = spritesPerWorkGroup;

        u32 culledOutputIndex = culledIndexBufferOutputIndex;
        u32 startIndex = inputSpriteIndicies.size();

        while (spritesToAdd > 0 && i < sprites.size()) {
            auto sprite = sprites[i];
            inputSpriteIndicies.push_back(sprite->inputSpriteIndex);
            culledIndexBufferOutputIndex += sprite->triCount;

            (*preCulledSprites)[currentSpriteIndex].groupCombinationIndex = sprite->groupCombinationIndex;
            (*preCulledSprites)[currentSpriteIndex].startPosition = ccPointToGLM(sprite->object->m_startPosition);

            currentSpriteIndex++;
            spritesToAdd--;
            i++;
        }

        u32 endIndex = inputSpriteIndicies.size();

        // log::info("  - Work group {} = (index={}, count={}), {}", workGroups.size(), startIndex, endIndex - startIndex, culledOutputIndex);

        workGroups.push_back({
            .inputSpriteStart = startIndex,
            .inputSpriteEnd   = endIndex,
            .culledIndexBufferOutputIndex = culledOutputIndex
        });
    }
}