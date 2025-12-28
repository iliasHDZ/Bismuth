#extension GL_ARB_shading_language_include:require

#include "shared.h"

//// VERTEX ATTRIBUTES ////
layout (location = 0) in vec2 a_positionOffset;
layout (location = 1) in vec2 a_texCoord;
layout (location = 2) in int  a_srbIndex;
layout (location = 3) in int  a_colorChannel;
layout (location = 4) in int  a_spriteSheet;
layout (location = 5) in int  a_opacity;

//// UNIFORMS ////
uniform mat4  u_mvp;
uniform float u_timer;
uniform float u_audioScale;
uniform vec2  u_cameraPosition;
uniform float u_cameraWidth;

// Used for invisible block calculation
uniform vec2  u_winSize;
uniform float u_screenRight;
uniform float u_cameraUnzoomedX;
uniform vec3  u_specialLightBGColor;

uniform uint  u_gameStateFlags;

//// UNIFORM and STORAGE BUFFERS ////
layout (std430, binding = DYNAMIC_RENDERING_BUFFER_BINDING) uniform DRB {
    DynamicRenderingBuffer drb;
};

layout (std430, binding = STATIC_RENDERING_BUFFER_BINDING) buffer SRB {
    StaticRenderingBuffer srb;
};

//// TRANSFER VARIABLES ////
out vec2 t_texCoord;
flat out int t_spriteSheet;
out vec4 t_color;
flat out uint t_blending;

#define SRB_OBJECT (srb.objects[a_srbIndex])

//// GLOBALS ////
uint objectFlags;
vec2 objectPosition;

//// HELPER FUNCTION PREDECLARATIONS ////
float calculateAudioScale();
vec4 calculateInvisibleBlockColorAndOpacity(vec4 color);

//// MAIN FUNCTION ////
void main() {
    //// CALCULATING VERTEX POSITION ////

    vec2 position = a_positionOffset;

    objectFlags    = SRB_OBJECT.flags;
    objectPosition = SRB_OBJECT.objectPosition;

    if (SRB_OBJECT.rotationSpeed != 0.0)
        position = rotatePointAroundOrigin(position, -SRB_OBJECT.rotationSpeed * u_timer / 180 * PI);

    if ((objectFlags & OBJECT_FLAG_USES_AUDIO_SCALE) != 0)
        position *= calculateAudioScale();

    position += objectPosition;
    gl_Position = u_mvp * vec4(position, 0.0, 1.0);

    //// TRANSFERING VARIABLES TO FRAGMENT SHADER ////

    t_texCoord    = a_texCoord;
    t_spriteSheet = a_spriteSheet;
    t_color       = RGBA_TO_VEC4(drb.channelColors[a_colorChannel]);

    if (a_spriteSheet == SPRITE_SHEET_GLOW && (objectFlags & OBJECT_FLAG_SPECIAL_GLOW_COLOR) != 0)
        t_color = vec4(u_specialLightBGColor, 1.0);

    t_blending = BITMAP_GET(drb.colorChannelBlendingBitmap, a_colorChannel);
    if (a_spriteSheet == SPRITE_SHEET_GLOW)
        t_blending = 1;

    t_color.a *= float(a_opacity) / 255.0;

    if ((objectFlags & OBJECT_FLAG_IS_INVISIBLE_BLOCK) != 0)
        t_color = calculateInvisibleBlockColorAndOpacity(t_color);
}

//// HELPER FUNCTIONS ////

float calculateAudioScale() {
    float scale;
    if ((objectFlags & OBJECT_FLAG_CUSTOM_AUDIO_SCALE) != 0) {
        float minScale = SRB_OBJECT.audioScaleMin;
        float maxScale = SRB_OBJECT.audioScaleMax;
        scale = (u_audioScale - 0.1) * (maxScale - minScale) + minScale;
    } else
        scale = u_audioScale;

    if ((objectFlags & OBJECT_FLAG_IS_ORB) != 0)
        scale = min(scale + 0.3, 1.2);
    return scale;
}

float getRelativeMod(float xPos, float left, float right, float offset) {
    float result = u_cameraWidth * 0.5;

    if (xPos > result + u_cameraPosition.x)
        result = (result - (xPos - offset - u_cameraPosition.x - result)) * right;
    else
        result = (result - (result + u_cameraPosition.x - xPos - offset)) * left;

    return clamp(result, 0.0, 1.0);
}

vec2 calculateInvisibleBlockOpacity() {
    float centerLeftX  = u_winSize.x * 0.5 - 75.0;
    float centerRightX = centerLeftX + 110.0;
    float someScreenLeft = u_screenRight - centerRightX - 90.0;

    float fadePosX = objectPosition.x;
    if (fadePosX <= u_cameraUnzoomedX)
        fadePosX += SRB_OBJECT.fadeMargin;
    else
        fadePosX -= SRB_OBJECT.fadeMargin;
    
    float relMod = getRelativeMod(fadePosX, 0.02, 0.014285714, 0.0);

    // INVISIBLE BLOCK OPACITY IS INACCURATE AND I CAN'T SEEM TO FIGURE IT OUT, I'M GOING CRAZY

    float someWidth1;
    if (fadePosX <= centerRightX + u_cameraPosition.x)
        someWidth1 = (centerLeftX + u_cameraPosition.x - fadePosX) / max(centerLeftX - 30.0, 1.0);
    else
        someWidth1 = (fadePosX - u_cameraPosition.x - centerRightX) / max(someScreenLeft, 1.0);

    someWidth1 = clamp(someWidth1, 0.0, 1.0);

    return vec2(
        min(someWidth1 * 0.95 + 0.05, relMod),
        min(someWidth1 * 0.85 + 0.15, relMod)
    );
}

vec4 calculateInvisibleBlockColorAndOpacity(vec4 color) {
    if ((u_gameStateFlags & GAME_STATE_IS_PLAYER_DEAD) != 0) {
        if (a_spriteSheet == SPRITE_SHEET_GLOW)
            return vec4(u_specialLightBGColor, color.a);
        return color;
    }

    vec2 opacity = calculateInvisibleBlockOpacity();

    if (a_spriteSheet != SPRITE_SHEET_GLOW) {
        color.a *= opacity.x;
    } else {
        // Might have to do some of these calculations on the cpu instead
        vec3 colorBG  = RGBA_TO_VEC4(drb.channelColors[COLOR_CHANNEL_BG]).rgb;
        vec3 colorLBG = RGBA_TO_VEC4(drb.channelColors[COLOR_CHANNEL_LBG]).rgb;

        vec3 colorB;
        if ((colorBG.r + colorBG.g + colorBG.b) >= (150.0 / 255.0))
            colorB = vec3(1, 1, 1);
        else
            colorB = colorLBG;

        vec3 glowColor;
        if (opacity.x <= 0.8)
            glowColor = u_specialLightBGColor;
        else
            glowColor = mix(colorB, u_specialLightBGColor, (1.0 - (opacity.x - 0.8) / 0.2) * 0.3 + 0.7);
        color = vec4(glowColor, color.a * opacity.y);
    }

    return color;
}