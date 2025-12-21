#version 330 core

uniform sampler2D u_textures[9];

out vec4 FragColor;

     in vec2 p_texCoord;
flat in int  p_spriteSheet;

void main() {
    FragColor = texture(u_textures[p_spriteSheet], p_texCoord);
}