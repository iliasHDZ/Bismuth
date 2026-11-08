#pragma once

#include <common.hpp>
#include <ObjectUtils.hpp>
#include "Buffer.hpp"
#include <map>
#include <queue>

#define WORK_GROUPS_PER_GPU_CORES 2.5

class Renderer;

using ColorChannelBlendStates = std::array<bool, 1101>;

struct WorkGroupRange {
    u32 offset, count;
};

struct SortedObjectBatch {
    u32 workGroupCount;
    std::map<LayerKey, WorkGroupRange> workGroupRangePerLayerKey;

    //// BUFFERS ////
    bool haveBuffersBeenCreated = false;
    UPtr<Buffer> spriteRemapBuffer;
    UPtr<Buffer> workGroupBuffer;
    UPtr<Buffer> drawCommandBuffer;
    UPtr<PreCulledSprite::Buffer> preCulledSpritesBuffer;

    //// BUFFER DATAS ////
    std::vector<u32> inputSpriteIndicies;
    std::vector<WorkGroup> workGroups;
    std::vector<DrawCommand> drawCommands;

    void bindBuffers();
};

class WorkGroupScheduler {
public:
    WorkGroupScheduler();
    ~WorkGroupScheduler();

    void addSprite(
        GameObject* object,
        cocos2d::CCSprite* sprite,
        SpriteType type,
        u32 inputSpriteIndex,
        u32 triCount
    );

    inline usize getSpriteCount() const {
        return spriteEntries.size();
    }

    SortedObjectBatch* fetchSortedBatchForBlendStates(const ColorChannelBlendStates& blendStates);

    inline void waitForJobs() {
        for (auto& [k, job] : currentGenerationJobs)
            if (job->thread.joinable())
                job->thread.join();
    }

private:
    void cleanUpFinishedJobs();

    void addGenerationJob(const ColorChannelBlendStates& blendStates);

private:
    struct SpriteEntry {
        i32 zOrder;
        u32 entryIndex;
        ZLayer zLayer;
        SpriteSheet spriteSheet;
        u16 colorChannelId;
        u32 inputSpriteIndex;
        u32 triCount;
        u32 groupCombinationIndex;
        GameObject* object;

        LayerKey getFullLayerKey(const ColorChannelBlendStates& blendStates) const;
    };

    std::vector<SpriteEntry> spriteEntries;

    struct GenerateJob {
        ColorChannelBlendStates blendStates;
        UPtr<SortedObjectBatch> sortedBatch;
    };

    struct GenerationJob {
        //// INPUTS ////
        // scheduler may only be accessed if its mutex has been locked!
        WorkGroupScheduler& scheduler;
        
        ColorChannelBlendStates blendStates;
        UPtr<PreCulledSprite::Buffer> preCulledSprites;

        isize maxWorkGroupCount = -1;

        //// THREAD ////
        std::thread thread;

        //// VARIABLES USED IN GENERATION ////
        usize currentSpriteIndex;

        std::unordered_map<LayerKey, std::vector<const SpriteEntry*>> sortedSpriteLayers;
        std::vector<u32> inputSpriteIndicies;
        std::vector<WorkGroup> workGroups;

        inline ~GenerationJob() {
            if (thread.joinable())
                thread.join();
        }

        void generateSortedBatchForBlendStates();

        void loadAndSortSprites(const std::vector<SpriteEntry>& sprites);

        void generateWorkGroupsForLayer(
            LayerKey layerKey,
            u32 spritesPerWorkGroup,
            u32& culledIndexBufferOutputIndex
        );
    };

    std::thread thread;

    isize maxWorkGroupCount = -1;

    std::map<ColorChannelBlendStates, UPtr<GenerationJob>> currentGenerationJobs;

    //// VARIABLES ACCESSED BY ALL THREADS ////
    
    std::mutex mutex;

    std::map<ColorChannelBlendStates, UPtr<SortedObjectBatch>> sortedBatchesPerBlendStates;
    std::set<GenerationJob*> finishedGenerationJobs;
};