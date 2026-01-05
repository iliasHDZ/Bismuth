#extension GL_ARB_shading_language_include:require

#include "shared.h"
#include "shaderSprites.glsl"

#ifdef USE_SEPERATE_SPRITESHEET_TEXTURES
uniform sampler2D u_spriteSheets[9];

#define SPRITESHEET_TEXTURE u_spriteSheets[t_shaderSprite]
#else
uniform sampler2D u_spriteSheet;

#define SPRITESHEET_TEXTURE u_spriteSheet
#endif

out vec4 FragColor;

     in vec2 t_texCoord;
     in vec4 t_color;
flat in int  t_spriteSheet;
flat in uint t_blending;
flat in uint t_shaderSprite;

vec4 getTextureColor() {
    if (t_shaderSprite == 0)
        return texture(SPRITESHEET_TEXTURE, t_texCoord);

    return sampleShaderSprite(t_shaderSprite, t_texCoord);
}

void main() {
    if (t_blending == 0) {
        FragColor = getTextureColor() * t_color;
    } else {
        vec4 texColor = getTextureColor();
        FragColor = texColor * t_color;
        FragColor.rgb *= texColor.a;
    }
}