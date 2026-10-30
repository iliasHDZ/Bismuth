/*
    This header file is shared between the shaders and
    the actual c++ source code of the mod.

    The 'GLSL' macro variable is only present in the
    shader code and can be used to disinguish
*/

#ifdef __cplusplus
#pragma once

#include <common.hpp>
#include <renderer/SAOBuffer.hpp>

#define CREATE_ALIGNED_STRUCT(Name_, Base_, Align_) \
    struct alignas(Align_) Name_ : public Base_ { \
        Name_() = default; \
        Name_(const Base_& v) : Base_(v) {} \
    };

CREATE_ALIGNED_STRUCT(std430vec2, glm::vec2, 8)
CREATE_ALIGNED_STRUCT(std430vec3, glm::vec3, 16)
CREATE_ALIGNED_STRUCT(std430vec4, glm::vec4, 16)
CREATE_ALIGNED_STRUCT(std430mat2, glm::mat2, 8)
CREATE_ALIGNED_STRUCT(std430mat3, glm::mat3, 16)
CREATE_ALIGNED_STRUCT(std430mat4, glm::mat4, 16)

// Set the correct std430 alignments
#define vec2 std430vec2
#define vec3 std430vec3
#define vec4 std430vec4
#define mat2 std430mat2
#define mat3 std430mat3
#define mat4 std430mat4
#define bool unsigned int
#define uint unsigned int

struct alignas(4) RGBA { u8 r, g, b, a; };

// In GLSL, these defines are set by the renderer
#define GROUP_ID_LIMIT     0
#define TOTAL_OBJECT_COUNT 0

#define GLSL_ONLY(D)
#define CPP_ONLY(D) D

#define UNIFORM_BUFFER(_BINDING) struct
#define STORAGE_BUFFER(_BINDING) struct

#else // GLSL

#define SPRITE_SHEET_GAME_1   0
#define SPRITE_SHEET_GAME_2   1
#define SPRITE_SHEET_TEXT     2
#define SPRITE_SHEET_FIRE     3
#define SPRITE_SHEET_SPECIAL  4
#define SPRITE_SHEET_GLOW     5
#define SPRITE_SHEET_PIXEL    6
#define SPRITE_SHEET_UNK      7
#define SPRITE_SHEET_PARTICLE 8

#define RGBA uint
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

mat2 rotationMatrix(float angleInRadians) {
    float s = sin(angleInRadians);
    float c = cos(angleInRadians);

    return mat2(
         c, s,
        -s, c
    );
}

mat2 scaleMatrix(float scale) {
    return mat2(
        scale, 0,
        0, scale
    );
}

#define BITMAP_GET(B, I) ( ( (B)[(I) >> 5] >> ((I) & 0x1f) ) & 1 )

#define GET_LOW16(V)  ( (V)       & 0xffff )
#define GET_HIGH16(V) ( (V >> 16) & 0xffff )

#define GLSL_ONLY(D) D
#define CPP_ONLY(D)

#define UNIFORM_BUFFER(BindingId_) layout (std140, binding = BindingId_) uniform
#define STORAGE_BUFFER(BindingId_) layout (std430, binding = BindingId_) buffer

#define _SAO_VARS_AS_BUFFERS(BufferName_, Binding_, Type_, Name_) \
    layout (std430, binding = Binding_) buffer Buffer_##BufferName_##_##Name_ { \
        Type_ BufferName_##_##Name_[]; \
    };

#define SAO_BUFFER(Vars_, Name_) \
    Vars_(Name_, _SAO_VARS_AS_BUFFERS)

#endif // GLSL

#include "hsv.h"

////////////////////////////////////////////////////
//// UNIFORM BUFFERS AND SHADER STORAGE BUFFERS ////
////////////////////////////////////////////////////

#define COLOR_CHANNEL_BUFFER_BINDING     0
#define GROUP_STATE_BUFFER_BINDING       1
#define OBJECT_INFO_BUFFER_BINDING       2
#define GAME_STATE_BUFFER_BINDING        3
#define INPUT_SPRITE_BUFFER_BINDING      4
#define WORK_GROUP_BUFFER_BINDING        5
#define CULLED_INDEX_BUFFER_BINDING      6
#define DRAW_COMMAND_BUFFER_BINDING      7
#define SPRITE_REMAP_BUFFER_BINDING      8
#define VERTEX_BUFFER_BINDING            9
#define CULLED_TRI_INDEX_BUFFER          10

#define PRE_CULLED_SPRITE(B, M) \
    M(B, 11, vec2, startPosition) \
    M(B, 12, uint, groupCombinationIndex)

#define RAW_SPRITE(B, M) \
    M(B, 13, vec4, color) \
    M(B, 14, vec2, position) \
    M(B, 15, mat2, localTransform)
    
#define PULSE_ENTRY_BUFFER_BINDING 16

SAO_BUFFER(RAW_SPRITE, RawSprite)
SAO_BUFFER(PRE_CULLED_SPRITE, PreCulledSprite)

struct ObjectVertex {
    vec2 positionOffset;
    vec2 texCoord;
    int  spriteIndex;
};

STORAGE_BUFFER(CULLED_TRI_INDEX_BUFFER) CulledTriIndexBuffer {
    uint u_culledTriIndicies[];
};

#include "colorChannelBuffer.h"
#include "groupStateBuffer.h"
#include "objectInfoBuffer.h"
#include "gameStateBuffer.h"
#include "rawSpriteBuffer.h"
#include "inputSpriteBuffer.h"
#include "workGroupBuffer.h"
#include "pulseEntryBuffer.h"

/*
    This is the structure of the draw command to
    be executed by glMultiDrawArraysIndirect
*/
struct DrawCommand {
    uint count;
    uint instanceCount;
    uint first;
    uint baseInstance;
};

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