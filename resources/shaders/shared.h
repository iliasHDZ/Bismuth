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

#define SPRITE_SHEET_GAME_1   0
#define SPRITE_SHEET_GAME_2   1
#define SPRITE_SHEET_TEXT     2
#define SPRITE_SHEET_FIRE     3
#define SPRITE_SHEET_SPECIAL  4
#define SPRITE_SHEET_GLOW     5
#define SPRITE_SHEET_PIXEL    6
#define SPRITE_SHEET_UNK      7
#define SPRITE_SHEET_PARTICLE 8

#extension GL_EXT_scalar_block_layout:require

#define RGBA uint
#define RGBA_TO_VEC4(V) vec4( \
    float((V) & 0xff) / 255.0, \
    float(((V) >> 8) & 0xff) / 255.0, \
    float(((V) >> 16) & 0xff) / 255.0, \
    float(((V) >> 24) & 0xff) / 255.0 \
)

const float PI = 3.1415926535897932384626433832795;

vec2 rotatePointAroundOrigin(vec2 point, float angleInRadians) {
    float rotSin = sin(angleInRadians);
    float rotCos = cos(angleInRadians);

    return vec2(
        point.x * rotCos - point.y * rotSin,
        point.x * rotSin + point.y * rotCos
    );
}

#define BITMAP_GET(B, I) ( ( (B)[(I) >> 5] >> ((I) & 0x1f) ) & 1 )

#endif

#define COLOR_CHANNEL_COUNT  1101
#define GROUP_IDS_PER_OBJECT 10

#define OBJECT_FLAG_USES_AUDIO_SCALE   (1 << 0)
#define OBJECT_FLAG_CUSTOM_AUDIO_SCALE (1 << 1)
#define OBJECT_FLAG_IS_ORB             (1 << 2)
#define OBJECT_FLAG_IS_INVISIBLE_BLOCK (1 << 3)
#define OBJECT_FLAG_SPECIAL_GLOW_COLOR (1 << 4)

#define GAME_STATE_IS_PLAYER_DEAD (1 << 0)

#define COLOR_CHANNEL_BG      1000
#define COLOR_CHANNEL_G1      1001
#define COLOR_CHANNEL_LINE    1002
#define COLOR_CHANNEL_3DL     1003
#define COLOR_CHANNEL_OBJ     1004
#define COLOR_CHANNEL_P1      1005
#define COLOR_CHANNEL_P2      1006
#define COLOR_CHANNEL_LBG     1007
#define COLOR_CHANNEL_G2      1009
#define COLOR_CHANNEL_BLACK   1010
#define COLOR_CHANNEL_WHITE   1011
#define COLOR_CHANNEL_LIGHTER 1012
#define COLOR_CHANNEL_MG      1013
#define COLOR_CHANNEL_MG2     1014

/*
    Rendering info of an object that never changes.
*/
struct StaticObjectInfo {
    vec2 objectPosition;
    float rotationSpeed;
    // Flags are the macro variables beginning with OBJECT_FLAG_*
    int flags;
    float audioScaleMin;
    float audioScaleMax;
    float fadeMargin;
    /*
        GLSL does not support 16-bit integers.
        Instead, 2 group ids are fit into a uint.
    */
    // uint groupIds[GROUP_IDS_PER_OBJECT / 2];
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
    /*
        Bitmap for whether the color channels
        have blending enabled.
    */
    uint colorChannelBlendingBitmap[COLOR_CHANNEL_COUNT / 32 + 1];

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