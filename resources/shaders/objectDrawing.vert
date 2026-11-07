#extension GL_ARB_shading_language_include:require

#include "shared.h"

//// VARIABLES TO BE TRANSFERED TO THE FRAGMENT SHADER ////
out vec2 t_texCoord;
out vec4 t_color;

STORAGE_BUFFER(VERTEX_BUFFER_BINDING) VertexBuffer {
    ObjectVertex u_verticies[];
};

//// MAIN FUNCTION ////
void main() {
    uint vertexIndex = u_culledTriIndicies[gl_VertexID / 3] * 3 + gl_VertexID % 3;

    vec2 positionOffset = u_verticies[vertexIndex].positionOffset;
    vec2 texCoord       = u_verticies[vertexIndex].texCoord;
    uint spriteIndex    = u_verticies[vertexIndex].spriteIndex;

    vec2 vertexPos = RawSprite_position[spriteIndex] + RawSprite_localTransform[spriteIndex] * positionOffset;

    gl_Position = u_mvp * vec4(vertexPos, 0.0, 1.0);

    t_color    = RawSprite_color[spriteIndex];
    t_texCoord = texCoord;
}