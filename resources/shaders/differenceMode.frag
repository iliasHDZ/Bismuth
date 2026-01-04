out vec4 FragColor;

in vec2 t_texCoord;

uniform sampler2D u_vanillaFrame;
uniform sampler2D u_bismuthFrame;

uniform float u_intensity;
uniform float u_backdropIntensity;

void main() {
    vec3 vcol = texture(u_vanillaFrame, t_texCoord).rgb;
    vec3 bcol = texture(u_bismuthFrame, t_texCoord).rgb;

    vec3 diff = abs(vcol - bcol) * u_intensity + vcol * u_backdropIntensity;
    FragColor = vec4(diff, 1.0);
}