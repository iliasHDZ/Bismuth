#extension GL_ARB_shading_language_include:require

uniform sampler2D u_spriteSheets[9];

out vec4 FragColor;

in vec2 t_texCoord;
flat in int t_spriteSheet;
in vec4 t_color;
flat in uint t_blending;

void main() {
    FragColor = texture(u_spriteSheets[t_spriteSheet], t_texCoord) * t_color;

    // FragColor.rgb *= FragColor.a;
    if (t_blending != 0) {
        FragColor.rgb *= FragColor.a;
        FragColor.a = 0;
    }
}