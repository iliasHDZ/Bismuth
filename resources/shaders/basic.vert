layout (location = 0) in vec2 a_pos;

uniform mat4 u_mvp;

void main() {
    gl_Position = u_mvp * vec4(a_pos, 0.0, 1.0);
}