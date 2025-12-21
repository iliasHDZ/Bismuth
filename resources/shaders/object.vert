#version 330 core

layout (location = 0) in vec2 a_pos;
layout (location = 1) in vec2 a_texCoord;
layout (location = 2) in int  a_spriteSheet;

uniform mat4 u_mvp;

     out vec2 p_texCoord;
flat out int  p_spriteSheet;

void main() {
    gl_Position = u_mvp * vec4(a_pos, 0.0, 1.0);

    p_texCoord    = a_texCoord;
    p_spriteSheet = a_spriteSheet;
}