/*
    This header file is shared between the shaders and
    the actual c++ source code of the mod.

    The 'GLSL' macro variable is only present in the
    shader code and can be used to disinguish
*/

#ifndef SHADER_SHARED_H
#define SHADER_SHARED_H

#ifndef GLSL

#include <common.hpp>
// Set the correct std430 alignments
#define vec2 alignas(8)  glm::vec2
#define vec3 alignas(16) glm::vec3
#define vec4 alignas(16) glm::vec4
#define mat2 alignas(8)  glm::mat2
#define mat3 alignas(16) glm::mat3
#define mat4 alignas(16) glm::mat4
#define bool alignas(4) bool
#define uint unsigned int

struct alignas(4) RGBA { u8 r, g, b, a; };

// In GLSL, these defines are set by the renderer
#define GROUP_ID_LIMIT     0
#define TOTAL_OBJECT_COUNT 0

#define GLSL_ONLY(D)
#define CPP_ONLY(D) D

#define UNIFORM_BUFFER(_BINDING) struct
#define STORAGE_BUFFER(_BINDING) struct

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

// #extension GL_EXT_scalar_block_layout : enable

#define RGBA uint
#define RGBA_TO_VEC4(V) vec4( \
    float((V) & 0xff) / 255.0, \
    float(((V) >> 8) & 0xff) / 255.0, \
    float(((V) >> 16) & 0xff) / 255.0, \
    float(((V) >> 24) & 0xff) / 255.0 \
)
#define BitmapThing uint

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

#define GET_LOW16(V)  ( (V)       & 0xffff )
#define GET_HIGH16(V) ( (V >> 16) & 0xffff )

#define GLSL_ONLY(D) D
#define CPP_ONLY(D)

#define UNIFORM_BUFFER(_BINDING) layout (std140, binding = _BINDING) uniform
#define STORAGE_BUFFER(_BINDING) layout (std430, binding = _BINDING) buffer

#endif

#define COLOR_CHANNEL_COUNT  1101
#define GROUP_IDS_PER_OBJECT 10

#define A_COLOR_CHANNEL_IS_SPRITE_DETAIL 0x1000

#define OBJECT_FLAG_USES_AUDIO_SCALE   (1 << 0)
#define OBJECT_FLAG_CUSTOM_AUDIO_SCALE (1 << 1)
#define OBJECT_FLAG_IS_ORB             (1 << 2)
#define OBJECT_FLAG_IS_INVISIBLE_BLOCK (1 << 3)
#define OBJECT_FLAG_SPECIAL_GLOW_COLOR (1 << 4)
#define OBJECT_FLAG_HAS_BASE_HSV       (1 << 5)
#define OBJECT_FLAG_HAS_DETAIL_HSV     (1 << 6)
#define OBJECT_FLAG_IS_STATIC_OBJECT   (1 << 7)

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

#define HSV uint

#define HSV_HUE_BIT  16
#define HSV_HUE_MASK 0x1ff
#define HSV_SAT_BIT  8
#define HSV_SAT_MASK 0xff
#define HSV_VAL_BIT  0
#define HSV_VAL_MASK 0xff
#define HSV_SAT_ADD  0x20000000
#define HSV_VAL_ADD  0x10000000

/*
    Rendering info of an object that never changes.
*/
struct StaticObjectInfo {
    vec2 startPosition;
    float rotationSpeed;
    // Flags are the macro variables beginning with OBJECT_FLAG_*
    int flags;
    float audioScaleMin;
    float audioScaleMax;
    float fadeMargin;
    float opacity;
    HSV baseHSV;
    HSV detailHSV;
    uint groupCombinationIndex;
};

/*
    The current state of a group combination.
    This info can change every frame.
*/
struct GroupCombinationState {
    mat2 positionalTransform;
    mat2 localTransform;
    vec2 offset;
    float opacity;
    uint _padding;
};

/*
    This contains a crop rectangle of a sprite
    in a spritesheet.
struct SpriteCrop {
    *
        For both of the following members, the
        lower 16-bit contains the minimum value
        and the higher 16-bit contains the maximum
        value of the crop.
    *
    uint xCrop;
    uint yCrop;
};

// This is for decoding the aSpriteCrop vertex attribute
#define A_SPRITE_CROP_MASK             0x1fffffff
#define A_SPRITE_CROP_IS_SHADER_SPRITE 0x20000000
#define A_SPRITE_CROP_IS_RIGHT         0x40000000
#define A_SPRITE_CROP_IS_TOP           0x80000000
*/

////////////////////////////////////////////////////
//// UNIFORM BUFFERS AND SHADER STORAGE BUFFERS ////
////////////////////////////////////////////////////

#define DYNAMIC_RENDERING_BUFFER_BINDING 0
#define STATIC_RENDERING_BUFFER_BINDING  1
#define RENDERER_UNIFORM_BUFFER_BINDING  2
//#define SPRITE_CROP_BUFFER_BINDING       3

/*
    This is the dynamic rendering buffer. This
    is the buffer that contains the required
    rendering information that has to change
    almost every frame. Stuff like the color
    channel colors and group transforms.
*/
STORAGE_BUFFER(DYNAMIC_RENDERING_BUFFER_BINDING) DynamicRenderingBuffer {
    RGBA channelColors[COLOR_CHANNEL_COUNT];
    /*
        Bitmap for whether the color channels
        have blending enabled.
    */
    uint colorChannelBlendingBitmap[COLOR_CHANNEL_COUNT / 32 + 1];

    GroupCombinationState groupCombinationStates[CPP_ONLY(0)];
} GLSL_ONLY(drb);

/*
    This is the static rendering buffer. This is the
    buffer that contains the required rendering
    information never changes. Stuff like group ids
    of objects and object flags.
*/
STORAGE_BUFFER(STATIC_RENDERING_BUFFER_BINDING) StaticRenderingBuffer {
    StaticObjectInfo objects[CPP_ONLY(0)];
} GLSL_ONLY(srb);

/*
    This contains a list of all sprite crops needed in
    rendering. Every sprite to be drawn is passed with
    a spriteCropIndex which indexes into the array in
    here. If index is negative, it is a shader sprite
    instead.
STORAGE_BUFFER(SPRITE_CROP_BUFFER_BINDING) SpriteCropBuffer {
    SpriteCrop spriteCrops[];
};
*/

/*
    This is the uniform buffer used to store
    simple uniforms.
*/
UNIFORM_BUFFER(RENDERER_UNIFORM_BUFFER_BINDING) RendererUniformBuffer {
    mat4  u_mvp;
    float u_timer;
    float u_audioScale;
    vec2  u_cameraPosition;
    vec2  u_cameraViewSize;

    // Used for invisible block calculation
    vec2  u_winSize;
    float u_screenRight;
    float u_cameraUnzoomedX;
    vec3  u_specialLightBGColor;

    uint  u_gameStateFlags;
};

/*
    Some sprites are so simple that they rather
    be generated in the fragment shader then be
    sampled from a texture. This is surprisingly
    faster as you don't have to worry about the
    texture cache and slow accesses.

    Here are all of the shader sprites:
*/
#define SHADER_SPRITE_SOLID_BLOCK 1
#define SHADER_SPRITE_SOLID_SLOPE 2
#define SHADER_SPRITE_GRADIENT_LINEAR 3
#define SHADER_SPRITE_GRADIENT_RADIAL 4
#define SHADER_SPRITE_GRADIENT_RADIAL_CORNER 5

#ifndef GLSL
// Remove the changed alignments
#undef vec2
#undef vec3
#undef vec4
#undef mat2
#undef mat3
#undef mat4
#undef bool
#undef uint
#undef HSV
#undef UNIFORM_BUFFER
#undef STORAGE_BUFFER
#endif

#endif // SHADER_SHARED_H