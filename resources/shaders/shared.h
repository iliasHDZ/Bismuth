/*
    This header file is shared between the shaders and
    the actual c++ source code of the mod.

    The 'GLSL' macro variable is only present in the
    shader code and can be used to disinguish
*/

#ifndef GLSL

#include <common.hpp>
// Set the correct std430 alignments
#define vec2 alignas(8)  glm::vec2
#define vec3 alignas(16) glm::vec3
#define vec4 alignas(16) glm::vec4
#define bool alignas(4) bool
#define uint unsigned int

struct alignas(4) RGBA { u8 r, g, b, a; };

// In GLSL, these defines are set by the renderer
#define TOTAL_GROUP_COUNT  0
#define TOTAL_OBJECT_COUNT 0

#else

#extension GL_EXT_scalar_block_layout:require

#define RGBA uint
#define RGBA_TO_VEC4(V) vec4( \
    float((V) & 0xff) / 255.0, \
    float(((V) >> 8) & 0xff) / 255.0, \
    float(((V) >> 16) & 0xff) / 255.0, \
    float(((V) >> 24) & 0xff) / 255.0 \
)

#endif

#define COLOR_CHANNEL_COUNT  1101
#define GROUP_IDS_PER_OBJECT 10

/*
    Rendering info of an object that never changes.
*/
struct StaticObjectInfo {
    /*
        GLSL does not support 16-bit integers.
        Instead, 2 group ids are fit into a uint.
    */
    uint groupIds[GROUP_IDS_PER_OBJECT / 2];
};

/*
    The current state of a group.
    This info can change every frame.
*/
struct GroupState {
    float opacity;
};

#define DYNAMIC_RENDERING_BUFFER_BINDING 1
#define STATIC_RENDERING_BUFFER_BINDING  2

/*
    This is the structure of the dynamic rendering
    uniform buffer. This is the buffer that contains
    the required rendering information that has
    to change almost every frame. Stuff like the color
    channel colors and group transforms.
*/
struct DynamicRenderingBuffer {
    RGBA channelColors[COLOR_CHANNEL_COUNT];

    GroupState groupStates[TOTAL_GROUP_COUNT];
};

/*
    This is the structure of the static rendering
    uniform buffer. This is the buffer that contains
    the required rendering information never changes.
    Stuff like group ids of objects and object flags.
*/
struct StaticRenderingBuffer {
    StaticObjectInfo objects[TOTAL_OBJECT_COUNT];
};

#ifndef GLSL
// Remove the changed alignments
#undef vec2
#undef vec3
#undef vec4
#undef bool
#undef uint
#endif