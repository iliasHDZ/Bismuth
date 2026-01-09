out vec4 FragColor;

in vec2 t_texCoord;

uniform sampler2D u_texture;

vec3 getColorForValue(float v) {
    if (v <= 3.0)
        return mix(vec3(0, 0, 1), vec3(0, 1, 0), v / 3.0);
    if (v <= 7.0)
        return mix(vec3(0, 1, 0), vec3(1, 1, 0), (v - 3.0) / 4.0);
    if (v <= 12.0)
        return mix(vec3(1, 1, 0), vec3(1, 0, 0), (v - 7.0) / 5.0);
    return mix(vec3(1, 0, 0), vec3(0.8, 0, 0), min((v - 12.0) / 5.0, 1.0));
}

void main() {
    FragColor = vec4(getColorForValue(texture(u_texture, t_texCoord).r * 255.0), 0.5);
}