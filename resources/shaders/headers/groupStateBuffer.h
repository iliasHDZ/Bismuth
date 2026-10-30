#define GROUP_IDS_PER_OBJECT 10

/*
    The current state of a group combination.
    This info can change every frame.
*/
struct GroupCombinationState {
    mat2 positionalTransform;
    mat2 localTransform;
    vec2 offset;
    float opacity;
    uint pulseIndex;
};

STORAGE_BUFFER(GROUP_STATE_BUFFER_BINDING) GroupStateBuffer {
    GroupCombinationState u_groupCombinationStates[CPP_ONLY(0)];
};