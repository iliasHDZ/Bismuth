struct PackedHSV {
    uint hueAndFlags;
    uint satAndVal;
};

#define HSV_SAT_ADD  0x20000000
#define HSV_VAL_ADD  0x10000000

#ifdef GLSL

// FIXME: The HSV conversion may be incorrect
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

struct HSVStruct {
    float hue;
    float sat;
    float val;
    bool satAdd;
    bool valAdd;
};

vec4 applyHSVStruct(HSVStruct hsvValue, vec4 color) {
    vec3 hsv = rgb2hsv(color.rgb);

    hsv.x = mod(hsv.x + hsvValue.hue, 1.0);

    if (hsvValue.satAdd)
        hsv.y += hsvValue.sat;
    else
        hsv.y *= hsvValue.sat;

    if (hsvValue.valAdd)
        hsv.z += hsvValue.val;
    else
        hsv.z *= hsvValue.val;

    hsv.y = clamp(hsv.y, 0.0, 1.0);
    hsv.z = clamp(hsv.z, 0.0, 1.0);
    return vec4(hsv2rgb(hsv), color.a);
}

HSVStruct readHSV(PackedHSV packedHsv) {
    HSVStruct hsv;
    hsv.hue = unpackHalf2x16(packedHsv.hueAndFlags).x / 360.0;

    vec2 satAndVal = unpackHalf2x16(packedHsv.satAndVal);
    hsv.sat = satAndVal.x;
    hsv.val = satAndVal.y;

    hsv.satAdd = (packedHsv.hueAndFlags & HSV_SAT_ADD) != 0;
    hsv.valAdd = (packedHsv.hueAndFlags & HSV_VAL_ADD) != 0;

    return hsv;
}

vec4 applyHSV(PackedHSV hsvValue, vec4 color) {
    return applyHSVStruct(readHSV(hsvValue), color);
}

HSVStruct getMultipliedHSV(HSVStruct hsv, float factor) {
    HSVStruct ret;
    ret.hue = hsv.hue * factor;
    ret.sat = hsv.satAdd ? hsv.sat * factor : hsv.sat * factor + (1.f - factor);
    ret.val = hsv.valAdd ? hsv.val * factor : hsv.val * factor + (1.f - factor);
    ret.satAdd = hsv.satAdd;
    ret.valAdd = hsv.valAdd;
    return ret;
}

#endif