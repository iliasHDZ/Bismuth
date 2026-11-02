layout (location = 0) in vec2 a_position;
layout (location = 1) in vec2 a_radiusVector;

out vec2 t_radiusVector;
out vec2 t_texCoord;

uniform mat4 CC_MVPMatrix;
uniform vec2 u_contentSize;

void main() {
    gl_Position = CC_MVPMatrix * vec4(a_position, 0.0, 1.0);
	t_radiusVector = a_radiusVector;
    t_texCoord = a_position / u_contentSize;
}