/*
    This header file is shared between the shaders and
    the actual c++ source code of the mod.

    The 'GLSL' macro variable is only present in the
    shader code and can be used to disinguish
*/

#ifndef GLSL

#include <common.hpp>
// Set the correct std140 alignments
#define vec2 alignas(8)  glm::vec2
#define vec3 alignas(16) glm::vec3
#define vec4 alignas(16) glm::vec4
#define bool alignas(4) bool

struct alignas(4) RGBA { u8 r, g, b, a; };

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

#define COLOR_CHANNEL_COUNT 1101

#define DYNAMIC_RENDERING_BUFFER_BINDING 1

/*
    This is the structure of the dynamic rendering
    uniform buffer. This is the buffer that contains
    the required rendering information that has
    to change almost every frame. Stuff like the color
    channel colors and group transforms.
*/
struct DynamicRenderingBuffer {
    RGBA channelColors[COLOR_CHANNEL_COUNT];
};

// Remove the changed alignments
#ifndef GLSL
#undef vec2
#undef vec3
#undef vec4
#undef bool
#endif