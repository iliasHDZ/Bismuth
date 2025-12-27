#version 460

layout (location = 0) in vec2 a_position;

out vec2 t_texCoord;

void main() {
    gl_Position = vec4(a_position, 0.0, 1.0);
    t_texCoord = (a_position + 1.0) / 2.0;
}