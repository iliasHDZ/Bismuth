#extension GL_ARB_shading_language_include:require

#include "shared.h"

//// VERTEX ATTRIBUTES ////
layout (location = 0) in vec2 a_positionOffset;
layout (location = 1) in vec2 a_texCoord;
layout (location = 2) in int  a_srbIndex;
layout (location = 3) in uint a_colorChannel;
layout (location = 4) in int  a_spriteSheet;
layout (location = 5) in uint a_shaderSprite;

//// VARIABLES TO BE TRANSFERED TO THE FRAGMENT SHADER ////
     out vec2 t_texCoord;
     out vec4 t_color;
flat out int  t_spriteSheet;
flat out uint t_blending;
flat out uint t_shaderSprite;

#define SRB_OBJECT (srb.objects[a_srbIndex])

//// GLOBALS ////
uint  objectFlags;
vec2  objectPosition;
float objectOpacity = 1.0;
vec2  vertexOffset;

//// HELPER FUNCTION PREDECLARATIONS ////
float calculateAudioScale();
vec4 calculateInvisibleBlockColorAndOpacity(vec4 color);
vec4 applyHSV(HSV hsvValue, vec4 color);

//// MAIN FUNCTION ////
void main() {
    //// CALCULATING VERTEX POSITION ////
    
    objectPosition = SRB_OBJECT.startPosition;

    // APPLY GROUP COMBINATION STATE
    GroupCombinationState state = drb.groupCombinationStates[SRB_OBJECT.groupCombinationIndex];
    objectPosition = state.positionalTransform * objectPosition + state.offset;
    
    vertexOffset = a_positionOffset;
    objectFlags  = SRB_OBJECT.flags;

    objectOpacity *= SRB_OBJECT.opacity;
    objectOpacity *= state.opacity;

    if ((objectFlags & OBJECT_FLAG_IS_STATIC_OBJECT) == 0)
        vertexOffset = state.localTransform * vertexOffset;

    if (SRB_OBJECT.rotationSpeed != 0.0) // TODO: Just supply rotation speed in radians instead
        vertexOffset = rotatePointAroundOrigin(vertexOffset, -SRB_OBJECT.rotationSpeed * u_timer / 180 * PI);

    if ((objectFlags & OBJECT_FLAG_USES_AUDIO_SCALE) != 0)
        vertexOffset *= calculateAudioScale();

    gl_Position = u_mvp * vec4(objectPosition + vertexOffset, 0.0, 1.0);

    //// TRANSFERING VARIABLES TO FRAGMENT SHADER ////

    uint colorChannel = a_colorChannel & 0xfff;

    t_spriteSheet  = a_spriteSheet;
    t_color        = RGBA_TO_VEC4(drb.channelColors[colorChannel]);
    t_shaderSprite = a_shaderSprite;
    t_texCoord     = a_texCoord;

    if (a_spriteSheet == SPRITE_SHEET_GLOW && (objectFlags & OBJECT_FLAG_SPECIAL_GLOW_COLOR) != 0)
        t_color = vec4(u_specialLightBGColor, 1.0);

    t_blending = BITMAP_GET(drb.colorChannelBlendingBitmap, colorChannel);
    if (a_spriteSheet == SPRITE_SHEET_GLOW)
        t_blending = 1;
    
    if ((objectFlags & OBJECT_FLAG_IS_INVISIBLE_BLOCK) != 0)
        t_color = calculateInvisibleBlockColorAndOpacity(t_color);

    if ((a_colorChannel & A_COLOR_CHANNEL_IS_SPRITE_DETAIL) == 0) {
        if ((objectFlags & OBJECT_FLAG_HAS_BASE_HSV) != 0)
            t_color = applyHSV(SRB_OBJECT.baseHSV, t_color);
    } else {
        if ((objectFlags & OBJECT_FLAG_HAS_DETAIL_HSV) != 0)
            t_color = applyHSV(SRB_OBJECT.detailHSV, t_color);
    }

    t_color.a *= objectOpacity;

    if (t_color.a < 0.01) {
        gl_Position = vec4(5, 5, 5, 1);
        return;
    }

    t_color.rgb *= t_color.a;
    if (t_blending != 0) {
        t_color.rgb *= t_color.a;
        t_color.a = 0.0;
    }
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
    float result = u_cameraViewSize.x * 0.5;

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

// Credits to sam hocevar for the following two functions
vec3 rgb2hsv(vec3 c) {
    vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
    vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
    vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));
    float d = q.x - min(q.w, q.y);
    float e = 1.0e-10;
    return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

vec4 applyHSV(HSV hsvValue, vec4 color) {
    float hue = float(int((hsvValue >> HSV_HUE_BIT) & HSV_HUE_MASK) - 256) / 360.0;
    float sat = float((hsvValue >> HSV_SAT_BIT) & HSV_SAT_MASK) / 127.5;
    float val = float((hsvValue >> HSV_VAL_BIT) & HSV_VAL_MASK) / 127.5;
    
    vec3 hsv = rgb2hsv(color.rgb);

    hsv.x = mod(hsv.x + hue, 1.0);

    if ((hsvValue & HSV_SAT_ADD) != 0)
        hsv.y += sat - 1.0;
    else
        hsv.y *= sat;

    if ((hsvValue & HSV_VAL_ADD) != 0)
        hsv.z += val - 1.0;
    else
        hsv.z *= val;

    hsv.y = clamp(hsv.y, 0.0, 1.0);
    hsv.z = clamp(hsv.z, 0.0, 1.0);
    return vec4(hsv2rgb(hsv), color.a);
}