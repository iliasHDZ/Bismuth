#extension GL_ARB_shading_language_include:require

#include "shared.h"

layout (location = 0) in vec2 a_positionOffset;
layout (location = 1) in vec2 a_texCoord;
layout (location = 2) in int  a_srbIndex;
layout (location = 3) in int  a_colorChannel;
layout (location = 4) in int  a_spriteSheet;
layout (location = 5) in int  a_opacity;

uniform mat4  u_mvp;
uniform float u_timer;
uniform float u_audioScale;

layout (std430, binding = DYNAMIC_RENDERING_BUFFER_BINDING) uniform DRB {
    DynamicRenderingBuffer drb;
};

layout (std430, binding = STATIC_RENDERING_BUFFER_BINDING) buffer SRB {
    StaticRenderingBuffer srb;
};

#define SRB_OBJECT (srb.objects[a_srbIndex])

out vec2 t_texCoord;
flat out int t_spriteSheet;
out vec4 t_color;
flat out uint t_blending;

void main() {
    //// CALCULATING VERTEX POSITION ////

    vec2 position = a_positionOffset;

    if (SRB_OBJECT.rotationSpeed != 0.0)
        position = rotatePointAroundOrigin(position, -SRB_OBJECT.rotationSpeed * u_timer / 180 * PI);

    if ((SRB_OBJECT.flags & OBJECT_FLAG_USES_AUDIO_SCALE) != 0) {
        float scale;
        if ((SRB_OBJECT.flags & OBJECT_FLAG_CUSTOM_AUDIO_SCALE) != 0) {
            float minScale = SRB_OBJECT.audioScaleMin;
            float maxScale = SRB_OBJECT.audioScaleMax;
            scale = (u_audioScale - 0.1) * (maxScale - minScale) + minScale;
        } else
            scale = u_audioScale;

        if ((SRB_OBJECT.flags & OBJECT_FLAG_IS_ORB) != 0)
            scale = min(scale + 0.3, 1.2);

        position *= scale;
    }

    position += SRB_OBJECT.objectPosition;
    gl_Position = u_mvp * vec4(position, 0.0, 1.0);

    //// TRANSFERING VARIABLES TO FRAGMENT SHADER ////

    t_texCoord    = a_texCoord;
    t_spriteSheet = a_spriteSheet;
    t_color       = RGBA_TO_VEC4(drb.channelColors[a_colorChannel]);

    t_blending = ( drb.colorChannelBlendingBitmap[a_colorChannel >> 5] >> (a_colorChannel & 0x1f) ) & 1;

    t_color.a *= float(a_opacity) / 255.0;
}