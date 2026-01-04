#extension GL_ARB_shading_language_include:require

uniform sampler2D u_spriteSheets[9];

out vec4 FragColor;

     in vec2 t_texCoord;
flat in int  t_spriteSheet;
     in vec4 t_color;
flat in uint t_blending;

void main() {
    if (t_blending == 0) {
        FragColor = texture(u_spriteSheets[t_spriteSheet], t_texCoord) * t_color;
    } else {
        vec4 texColor = texture(u_spriteSheets[t_spriteSheet], t_texCoord);
        FragColor = texColor * t_color;
        FragColor.rgb *= texColor.a;
    }
}