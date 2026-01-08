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
    else
        return vec4(1.0, 1.0, 1.0, 1.0);

    // return sampleShaderSprite(t_shaderSprite, t_texCoord);
}

void main() {
    // vec4 texColor;
    
    // if (t_shaderSprite == 0)
    //     texColor = texture(SPRITESHEET_TEXTURE, t_texCoord);
    // else
    //     texColor = vec4(1.0, 1.0, 1.0, 1.0);

    if (t_blending == 0) {
        FragColor = texture(SPRITESHEET_TEXTURE, t_texCoord) * t_color;
    } else {
        vec4 texColor = texture(SPRITESHEET_TEXTURE, t_texCoord);
        FragColor = texColor * t_color;
        FragColor.rgb *= texColor.a;
    }
}