#include "GroupManager.hpp"
#include "Renderer.hpp"
#include "glm/ext/matrix_transform.hpp"
#include <Geode/binding/EffectGameObject.hpp>
#include <Geode/binding/GameObject.hpp>
#include "../profiler.hpp"
#include <optional>
#include <string>

using namespace geode::prelude;

static std::optional<GroupID> getTargetGroupIdOfTransformingTrigger(GameObject* object) {
    switch (object->m_objectID) {
    case MOVE_TRIGGER_ID:
    case ROTATE_TRIGGER_ID:
    case FOLLOW_TRIGGER_ID:
        return ((EffectGameObject*)object)->m_targetGroupID;
    }
    return std::nullopt;
}

GroupCombination::GroupCombination(std::array<GroupID, 10>* groupIds, i32 count) {
    i32 i = 0;
    for (; i < count; i++)
        ids[i] = groupIds->at(i);
    for (; i < 10; i++)
        ids[i] = 0;
    this->count = count;
    if (count < 2)
        return;
    std::sort(ids.data(), &ids[count]);
}

GroupCombination::GroupCombination(std::vector<GroupID> groupIds) {
    count = groupIds.size();
    std::copy(groupIds.begin(), groupIds.end(), ids.begin());
    std::fill(ids.begin() + count, ids.end(), 0);
    std::sort(ids.begin(), ids.begin() + count);
}

void GroupCombination::removeGroupIdsNotInSet(const std::set<GroupID> maskSet) {
    for (i32 i = 0; i < count; i++) {
        if (!maskSet.contains(ids[i]))
            ids[i] = 0;
    }

    i32 newCount = 0;
    i32 srcI = 0, dstI = 0;
    for (; srcI < count; srcI++) {
        if (ids[srcI] != 0) {
            ids[dstI] = ids[srcI];
            dstI++;
            newCount++;
        }
    }
    for (; dstI < 10; dstI++)
        ids[dstI] = 0;
    count = newCount;
}

GroupCombination::operator std::string() const {
    std::string ret = "[";
    for (u32 i = 0; i < count; i++) {
        if (i != 0) ret += ", ";
        ret += std::to_string(ids[i]);
    }
    return ret + "]";
}

GroupManager::~GroupManager() {}

static std::set<GameObject*> dobjects;

void GroupManager::initWithObjects(cocos2d::CCArray* objects) {
    auto emptyGroupComb = GroupCombination(std::vector<GroupID>{});
    addGroupCombination(emptyGroupComb, 0);
    GroupCombinationIndex groupCombIndex = 1;

    for (auto object : CCArrayExt<GameObject*>(objects)) {
        std::optional<GroupID> targetGroupId = getTargetGroupIdOfTransformingTrigger(object);

        if (targetGroupId.has_value())
            transformGroupIds.insert(targetGroupId.value());
        else if (object->m_objectID == ALPHA_TRIGGER_ID || object->m_objectID == TOGGLE_TRIGGER_ID)
            alphaGroupIds.insert(((EffectGameObject*)object)->m_targetGroupID);

        dobjects.insert(object);
        
        GroupCombination comb = GroupCombination(object);

        if (groupCombinationIndicies.find(comb) == groupCombinationIndicies.end()) {
            addGroupCombination(comb, groupCombIndex);
            groupCombIndex++;
        }
    }

    groupCombinationCount = groupCombIndex;

    u32 transformCombIndex = 0;
    u32 alphaCombIndex     = 0;

    alphaIndiciesPerGroupCombinationIndex.resize(groupCombinationCount);

    for (auto object : CCArrayExt<GameObject*>(objects)) {
        GroupCombination comb = GroupCombination(object);
        auto combIndex = groupCombinationIndicies[comb];

        GroupCombination transformComb = comb;
        transformComb.removeGroupIdsNotInSet(transformGroupIds);

        if (transformCombinationIndicies.find(transformComb) == transformCombinationIndicies.end()) {
            transformCombinationIndicies[transformComb] = transformCombIndex;
            firstGroupIndexPerTransformIndex.push_back(combIndex);
            transformCombIndex++;
        }
        
        GroupCombination alphaComb = comb;
        comb.removeGroupIdsNotInSet(alphaGroupIds);

        if (alphaIndicies.find(alphaComb) == alphaIndicies.end()) {
            alphaIndicies[alphaComb] = alphaCombIndex;
            alphaCombIndex++;

            alphaIndexGroupIdFastStructure.push_back(alphaComb.getCount());
            for (auto id : alphaComb.getSpan())
                alphaIndexGroupIdFastStructure.push_back(id);
        }

        alphaIndiciesPerGroupCombinationIndex[combIndex] = alphaIndicies[alphaComb];
    }

    alphaIndiciesCount = alphaCombIndex;

    transformCombinationCount = transformCombIndex;
    log::info("Number of unique group transformations: {}", transformCombinationCount);

    alphaValuesPerGroupId.resize(maxGroupId + 1);
    alphaValuesPerAlphaIndex.resize(alphaIndiciesCount);

    groupStateBuffer = Buffer::createDynamicDraw("GroupStateBuffer", getGroupStateBufferSize());
    groupStates = (GroupCombinationState*)groupStateBuffer->mapReadWrite();

    isGroupToggledOn.resize(maxGroupId + 1, true);

    fetchVanilaRotateGroups();
}
    
GroupCombinationIndex GroupManager::getGroupCombinationIndexForObject(GameObject* object) {
    auto comb = GroupCombination(object);

    auto it = groupCombinationIndicies.find(comb);
    assert(it != groupCombinationIndicies.end());
    return it->second;
}

GroupCombinationIndex GroupManager::getTransformCombinationIndexForObject(GameObject* object) {
    auto comb = GroupCombination(object);
    comb.removeGroupIdsNotInSet(transformGroupIds);

    auto it = transformCombinationIndicies.find(comb);
    assert(it != transformCombinationIndicies.end());

    return it->second;
}

void GroupManager::fetchVanilaRotateGroups() {
    PlayLayer* layer = renderer.getPlayLayer();

    vanillaRotateGroups.resize(std::max(layer->m_staticGroups.size(), layer->m_optimizedGroups.size()));

    for (GroupID id = 1; id < layer->m_staticGroups.size(); id++) {
        CCArray* group = layer->m_staticGroups[id];
        if (!group) continue;

        auto& dstGroup = vanillaRotateGroups[id];

        for (auto object : CCArrayExt<GameObject*, false>(group)) {
            if (object && object->m_objectType != GameObjectType::Decoration)
                dstGroup.insert(object);
        }
    }

    for (GroupID id = 1; id < vanillaRotateGroups.size(); id++) {
        GameObject* mainObject = layer->tryGetMainObject(id);
        if (mainObject) {
            for (usize i = 0; i < mainObject->m_groupCount; i++)
                vanillaRotateGroups[mainObject->m_groups->at(i)].insert(mainObject);
        }
    }
}

void GroupManager::updateGroupStateBuffer() {
    PROFILER_PUSH("GroupManager::updateGroupStateBuffer");
    
    updateAlphaValues();

    updatePulseEntries();

    groupStateBuffer->flush(0, groupStateBuffer->getSize());
    PROFILER_POP();
}

void GroupManager::storeGroupStates(StoredGroupStates& storage) {
    storage.groupStates.resize(groupCombinationCount);
    std::memcpy(storage.groupStates.data(), groupStates, getGroupStateBufferSize());

    storage.isGroupToggledOn.resize(isGroupToggledOn.size());
    std::copy(isGroupToggledOn.begin(), isGroupToggledOn.end(), storage.isGroupToggledOn.begin());
}

void GroupManager::restoreGroupStates(StoredGroupStates& storage) {
    usize numToCopy = std::min<usize>(storage.groupStates.size(), groupCombinationCount);
    std::memcpy(groupStates, storage.groupStates.data(), numToCopy * sizeof(GroupCombinationState));

    std::copy(storage.isGroupToggledOn.begin(), storage.isGroupToggledOn.end(), isGroupToggledOn.begin());
}

void GroupManager::storeCheckpoint(CheckpointObject* checkpoint) {
    auto& groupStates = storedCheckpointGroupStates[checkpoint];
    storeGroupStates(groupStates);
}

void GroupManager::restoreCheckpoint(CheckpointObject* checkpoint) {
    if (!storedCheckpointGroupStates.contains(checkpoint))
        return;

    auto& groupStates = storedCheckpointGroupStates[checkpoint];
    restoreGroupStates(groupStates);
}

void GroupManager::removeCheckpoint(CheckpointObject* checkpoint) {
    storedCheckpointGroupStates.erase(checkpoint);
}

void GroupManager::moveGroup(GroupID groupId, float deltaX, float deltaY) {
    auto combIndicies = groupCombinationIndiciesPerGroupId[groupId];
    for (auto combIndex : combIndicies)
        groupStates[combIndex].offset += glm::vec2(deltaX, deltaY);
}

void GroupManager::rotateGroup(
    GroupID groupId,
    float angle,
    bool lockObjectRotation,
    std::optional<glm::vec2> centerPoint
) {
    float cos = cosf(glm::radians(angle));
    float sin = sinf(glm::radians(angle));
    glm::mat2 matrix = {
        { cos, -sin },
        { sin,  cos }
    };

    auto combIndicies = groupCombinationIndiciesPerGroupId[groupId];
    for (auto combIndex : combIndicies) {
        auto& groupState = groupStates[combIndex];

        if (!lockObjectRotation)
            groupState.localTransform *= matrix;

        if (!centerPoint.has_value())
            return;

        auto center = centerPoint.value();
        groupState.positionalTransform *= matrix;
        groupState.offset = matrix * (groupState.offset - center) + center;
    }
}

void GroupManager::toggleGroup(GroupID groupId, bool enabled) {
    isGroupToggledOn[groupId] = enabled;
}

void GroupManager::resetGroupStates() {
    for (auto& groupState : getGroupStates()) {
        groupState.positionalTransform = glm::identity<glm::mat2>();
        groupState.localTransform = glm::identity<glm::mat2>();
        groupState.offset = glm::vec2(0, 0);
    }
    std::fill(isGroupToggledOn.begin(), isGroupToggledOn.end(), true);
}

void GroupManager::addGroupCombination(GroupCombination& comb, GroupCombinationIndex index) {
    if (index >= groupCombinations.size())
        groupCombinations.resize(index + 1);
    groupCombinations[index] = comb;

    groupCombinationIndicies[comb] = index;

    for (auto groupId : comb.getSpan()) {
        usedGroupIds.insert(groupId);

        if (groupId > maxGroupId)
            maxGroupId = groupId;

        if (groupId >= groupCombinationIndiciesPerGroupId.size())
            groupCombinationIndiciesPerGroupId.resize(groupId + 1);

        groupCombinationIndiciesPerGroupId[groupId].push_back(index);
    }
}

void GroupManager::updateAlphaValues() {
    for (u32 id = 1; id <= maxGroupId; id++) {
        if (isGroupToggledOn[id])
            alphaValuesPerGroupId[id] = renderer.getPlayLayer()->m_effectManager->opacityModForGroup(id);
        else
            alphaValuesPerGroupId[id] = 0.0;
    }

    u32* alphaFast = alphaIndexGroupIdFastStructure.data();
    for (u32 alphaIndex = 0; alphaIndex < alphaIndiciesCount; alphaIndex++) {
        u32 combCount = *(alphaFast++);
        float alpha = 1.0;
        while (combCount > 0) {
            alpha *= alphaValuesPerGroupId[*(alphaFast++)];
            combCount--;
        }
        alphaValuesPerAlphaIndex[alphaIndex] = alpha;
    }

    for (u32 combIndex = 0; combIndex < groupCombinationCount; combIndex++)
        groupStates[combIndex].opacity = alphaValuesPerAlphaIndex[alphaIndiciesPerGroupCombinationIndex[combIndex]];
}

struct PulseEntry {
    u32 flags = 0;
    union {
        u32 hsv;
        ccColor4B color;
    };

    inline static PulseEntry fromAction(const PulseEffectAction& action) {
        PulseEntry ret;

        ret.flags |= PULSE_FLAG_IS_ACTIVE;
        ret.flags |= (u32)(action.m_currentValue * 65535.0f) & 0xffff;

        if (action.m_mainOnly)     ret.flags |= PULSE_FLAG_IS_BASE_ONLY;
        if (action.m_detailOnly)   ret.flags |= PULSE_FLAG_IS_DETAIL_ONLY;
        if (action.m_isDynamicHsv) ret.flags |= PULSE_FLAG_IS_DYNAMIC_HSV;

        if (action.m_pulseEffectType == PulseEffectType::Hsv) {
            ret.flags |= PULSE_FLAG_IS_HSV;
            ret.flags |= (action.m_colorIndex & 0x3ff) << 16;
            ret.hsv = convertToShaderHSV(action.m_hsv);
        } else {
            ret.color = { action.m_color.r, action.m_color.g, action.m_color.b, 255 };
        }

        return ret;
    }

    inline static PulseEntry endIndicator() { return PulseEntry(); }
};

void GroupManager::updatePulseEntries() {
    PROFILER_PUSH("GroupManager::updatePulseEntries");

    auto effectManager = renderer.getPlayLayer()->m_effectManager;

    for (usize i = 0; i < getGroupCombinationCount(); i++)
        groupStates[i].pulseIndex = 0;

    std::vector<GroupCombinationIndex> pulsingGroupCombs;

    for (GroupID gid = 0; gid < effectManager->m_unkVector2f0.size(); gid++) {
        if (!effectManager->m_unkVector2f0[gid] || gid >= groupCombinationIndiciesPerGroupId.size())
            continue;

        for (const auto& groupComb : groupCombinationIndiciesPerGroupId[gid])
            pulsingGroupCombs.push_back(groupComb);
    }

    std::vector<PulseEntry> pulseEntries;

    // This is so that pulse index 0 can be skipped immediately without
    // having to be checked
    pulseEntries.push_back(PulseEntry::endIndicator());

    usize maxEntryCount = 0;

    for (auto groupCombIndex : pulsingGroupCombs) {
        u32 pulseIndex = pulseEntries.size();

        const auto& comb = groupCombinations[groupCombIndex];

        for (GroupID gid : comb.getSpan()) {
            if (!effectManager->m_unkVector2f0[gid])
                continue;

            const auto& pulseActions = effectManager->m_pulseEffectMap[gid];

            for (const auto& action : pulseActions) {
                if (
                    action.m_pulseEffectType != PulseEffectType::Hsv &&
                    action.m_pulseEffectType != PulseEffectType::Color
                ) {
                    continue;
                }

                pulseEntries.push_back(PulseEntry::fromAction(action));     
                maxEntryCount++;       
            }
        }

        if (pulseIndex == pulseEntries.size())
            continue;

        pulseEntries.push_back(PulseEntry::endIndicator());
        groupStates[groupCombIndex].pulseIndex = pulseIndex;
    }

    pulseEntryBufferEnsureSize(pulseEntries.size() * sizeof(PulseEntry));
    pulseEntryBuffer->write(std::span<PulseEntry>(pulseEntries));
    pulseEntryBuffer->flush(0, pulseEntries.size() * sizeof(PulseEntry));

    PROFILER_POP();
}

#define PULSE_ENTRY_BUFFER_SIZE_STEP 32 * 1024

void GroupManager::pulseEntryBufferEnsureSize(usize size) {
    if (pulseEntryBuffer && pulseEntryBuffer->getSize() >= size)
        return;

    pulseEntryBuffer = nullptr;

    usize newSize = (size / PULSE_ENTRY_BUFFER_SIZE_STEP + 1) * PULSE_ENTRY_BUFFER_SIZE_STEP;

    log::info("Resized PulseEntryBuffer to {}", byteSizeToString(newSize));

    pulseEntryBuffer = Buffer::createDynamicDraw("PulseEntryBuffer", newSize);
}