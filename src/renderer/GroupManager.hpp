#pragma once

#include <Geode/binding/GameObject.hpp>
#include <common.hpp>
#include <span>

/*
    Objects can have up to 10 group ids. Alpha, move, rotate
    and scale triggers can then change opacity or transformation
    depending on which group id they have.
    
    In GD, this is done by keeping track of the opacity and
    transformation of every object. And when a trigger gets
    triggered, it goes through every object with the target
    group id and applies these properties.

    But, we're going to do it differently.

    We are going to get every combination of group ids of all
    the objects. We then keep track of opacity and transformation
    per group combination rather than per object. In the case
    of a trigger. The properties of that trigger get only applied
    to the group combination that contains the target group id.

    The state of every group combination is then sent to the shader
    to be applied to every object in parallel.

    To do that, every group combination is assigned an index.
*/

using GroupID = i16;
using GroupCombinationIndex = u32;

class GroupCombination {
public:
    GroupCombination(std::array<GroupID, 10>* groupIds, i32 count);

    inline GroupCombination(GameObject* object)
        : GroupCombination(object->m_groups, object->m_groupCount) {}

    inline bool operator<(const GroupCombination& o) const {
        if (count < o.count) return true;
        if (count > o.count) return false;

        for (i32 i = count - 1; i >= 0; i--) {
            if (ids[i] < o.ids[i])
                return true;
        }
        return false;
    }

    inline std::span<GroupID> getSpan() {
        return std::span<GroupID>(ids.data(), &ids[count]);
    }

    operator std::string() const;

private:
    std::array<GroupID, 10> ids;
    i32 count;
};

class Renderer;

class GroupManager {
public:
    inline GroupManager(Renderer& renderer)
        : renderer(renderer) {}
    
    GroupCombinationIndex getGroupCombinationIndexForObject(GameObject* object);

    inline GroupID getMaxGroupId() const { return maxGroupId; }

    inline u32 getGroupCombinationCount() const { return currentGroupCombinationIndex; }

    //// TRIGGER ACTIONS ////

    void moveGroup(GroupID groupId, float deltaX, float deltaY);

    void rotateGroup(
        GroupID groupId,
        float angle,
        bool lockObjectRotation,
        std::optional<glm::vec2> centerPoint = std::nullopt
    );

    void toggleGroup(GroupID groupId, bool visible);

    void updateOpacities();

    //// RESET ////

    void resetGroupStates();

private:
    void addGroupCombination(GroupCombination& comb, GroupCombinationIndex index);

private:
    Renderer& renderer;

    GroupCombinationIndex currentGroupCombinationIndex = 0;
    std::map<GroupCombination, GroupCombinationIndex> groupCombinationIndicies;

    /*
        This is a map with the key being a group id and the value being
        an array of all group combination indicies it belongs to.

        This is used for fast lookup to see which group combinations
        need to be changed when a group id gets affected.
    */
    std::unordered_map<GroupID, std::vector<GroupCombinationIndex>> groupCombinationIndiciesPerGroupId;

    GroupID maxGroupId = 0;
    std::set<GroupID> usedGroupIds;
    std::set<GroupID> disabledGroups;
};