#include "GroupManager.hpp"
#include "Renderer.hpp"

using namespace geode::prelude;

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

GroupCombination::operator std::string() const {
    std::string ret = "[";
    for (u32 i = 0; i < count; i++) {
        if (i != 0) ret += ", ";
        ret += std::to_string(ids[i]);
    }
    return ret + "]";
}
    
GroupCombinationIndex GroupManager::getGroupCombinationIndexForObject(GameObject* object) {
    auto comb = GroupCombination(object);

    auto it = groupCombinationIndicies.find(comb);
    if (it != groupCombinationIndicies.end())
        return it->second;

    GroupCombinationIndex index = currentGroupCombinationIndex;
    currentGroupCombinationIndex++;

    addGroupCombination(comb, index);
    return index;
}

void GroupManager::moveGroup(GroupID groupId, float deltaX, float deltaY) {
    auto it = groupCombinationIndiciesPerGroupId.find(groupId);
    if (it == groupCombinationIndiciesPerGroupId.end())
        return;

    GroupCombinationState* groupCombStates = renderer.getGroupCombinationStates();
    for (auto combIndex : it->second)
        groupCombStates[combIndex].offset += glm::vec2(deltaX, deltaY);
}

void GroupManager::rotateGroup(
    GroupID groupId,
    float angle,
    bool lockObjectRotation,
    std::optional<glm::vec2> centerPoint
) {
    auto it = groupCombinationIndiciesPerGroupId.find(groupId);
    if (it == groupCombinationIndiciesPerGroupId.end())
        return;
    
    float cos = cosf(glm::radians(angle) * 0.5);
    float sin = sinf(glm::radians(angle) * 0.5);
    glm::mat2 matrix = {
        { cos, -sin },
        { sin,  cos }
    };

    GroupCombinationState* groupCombStates = renderer.getGroupCombinationStates();
    for (auto combIndex : it->second) {
        auto& groupState = groupCombStates[combIndex];

        groupState.localTransform *= matrix;
        if (!centerPoint.has_value())
            return;

        auto center = centerPoint.value();
        groupState.positionalTransform *= matrix;
        groupState.offset = matrix * (groupState.offset - center) + center;
    }
}

void GroupManager::toggleGroup(GroupID groupId, bool visible) {
    if (visible)
        disabledGroups.erase(groupId);
    else
        disabledGroups.insert(groupId);
}

void GroupManager::updateOpacities() {
    GroupCombinationState* groupCombStates = renderer.getGroupCombinationStates();

    for (u32 i = 0; i < getGroupCombinationCount(); i++)
        groupCombStates[i].opacity = 1.f;

    for (auto groupId : usedGroupIds) {
        auto it = groupCombinationIndiciesPerGroupId.find(groupId);
        if (it == groupCombinationIndiciesPerGroupId.end())
            continue;

        bool isDisabled = disabledGroups.contains(groupId);

        for (auto combIndex : it->second) {
            if (isDisabled)
                groupCombStates[combIndex].opacity = 0.0;
            else
                groupCombStates[combIndex].opacity *= renderer.getPlayLayer()->m_effectManager->opacityModForGroup(groupId);
        }
    }
}

void GroupManager::resetGroupStates() {
    auto groupCombStates = renderer.getGroupCombinationStates();
    for (i32 i = 0; i < getGroupCombinationCount(); i++) {
        auto& groupState = groupCombStates[i];
        groupState.positionalTransform = glm::mat4(1.0);
        groupState.localTransform = glm::mat4(1.0);
        groupState.offset = glm::vec2(0, 0);
    }
    disabledGroups.clear();
}

void GroupManager::addGroupCombination(GroupCombination& comb, GroupCombinationIndex index) {
    groupCombinationIndicies[comb] = index;
    

    for (auto groupId : comb.getSpan()) {
        usedGroupIds.insert(groupId);

        if (groupId > maxGroupId)
            maxGroupId = groupId;

        auto it = groupCombinationIndiciesPerGroupId.find(groupId);
        if (it != groupCombinationIndiciesPerGroupId.end()) {
            it->second.push_back(index);
        } else {
            groupCombinationIndiciesPerGroupId[groupId] = { index };
        }
    }
}