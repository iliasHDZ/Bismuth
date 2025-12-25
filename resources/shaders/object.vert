#extension GL_ARB_shading_language_include:require

#include "shared.h"

layout (location = 0) in vec2 a_pos;
layout (location = 1) in vec2 a_texCoord;
layout (location = 2) in int  a_colorChannel;
layout (location = 3) in int  a_spriteSheet;
layout (location = 4) in int  a_opacity;

uniform mat4 u_mvp;

layout (std430, binding = DYNAMIC_RENDERING_BUFFER_BINDING) uniform DRB {
    DynamicRenderingBuffer drb;
};

out vec2 t_texCoord;
flat out int t_spriteSheet;
out vec4 t_color;
flat out uint t_blending;

void main() {
    gl_Position = u_mvp * vec4(a_pos, 0.0, 1.0);

    t_texCoord    = a_texCoord;
    t_spriteSheet = a_spriteSheet;
    t_color       = RGBA_TO_VEC4(drb.channelColors[a_colorChannel]);

    t_blending = ( drb.colorChannelBlendingBitmap[a_colorChannel >> 5] >> (a_colorChannel & 0x1f) ) & 1;

    t_color.a *= float(a_opacity) / 255.0;
}