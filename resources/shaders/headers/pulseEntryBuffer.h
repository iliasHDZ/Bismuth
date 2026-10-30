#define PULSE_FLAG_IS_BASE_ONLY   (1 << 26)
#define PULSE_FLAG_IS_DETAIL_ONLY (1 << 27)
#define PULSE_FLAG_IS_HSV         (1 << 28)
#define PULSE_FLAG_IS_DYNAMIC_HSV (1 << 29)
#define PULSE_FLAG_IS_ACTIVE      (1 << 30)
#define PULSE_FLAG_GET_VALUE(F) (float((F) & 0xffff) / 65535.0f)
#define PULSE_FLAG_GET_COLOR_CHANNEL(F) (((F) >> 16) & 0x3ff)

struct PulseGroupEntry {
    uint flags;
    uint colorOrHSV;
};

STORAGE_BUFFER(PULSE_ENTRY_BUFFER_BINDING) PulseEntryBuffer {
    PulseGroupEntry u_pulseEntries[CPP_ONLY(0)];
};

#ifdef GLSL

vec3 applyPulseEntry(vec3 color, uint flags, uint colorOrHSV);

vec3 applyPulseEntries(vec3 color, uint pulseIndex, bool isBaseColor) {
    while (true) {
        uint flags = u_pulseEntries[pulseIndex].flags;
        if (flags == 0)
            break;

        if ((flags & PULSE_FLAG_IS_BASE_ONLY) != 0 && !isBaseColor)
            continue;

        if ((flags & PULSE_FLAG_IS_DETAIL_ONLY) != 0 && isBaseColor)
            continue;

        color = applyPulseEntry(color, flags, u_pulseEntries[pulseIndex].colorOrHSV);
        pulseIndex++;
    }

    return color;
}

vec3 applyPulseEntry(vec3 color, uint flags, uint colorOrHSV) {
    float value = PULSE_FLAG_GET_VALUE(flags);

    if (value <= 0.0f)
        return color;

    if ((flags & PULSE_FLAG_IS_HSV) != 0) {
        uint colorChannel = PULSE_FLAG_GET_COLOR_CHANNEL(flags);
        HSVStruct hsv = readHSV(colorOrHSV);

        vec3 srcColor = color;
        if (colorChannel != 0)
            srcColor = getColorChannelColor(colorChannel).rgb;

        if (value == 1.0f)
            return applyHSVStruct(hsv, vec4(srcColor, 1.0)).rgb;

        if ((flags & PULSE_FLAG_IS_DYNAMIC_HSV) != 0)
            hsv = getMultipliedHSV(hsv, value);

        vec3 dstColor = applyHSVStruct(hsv, vec4(srcColor, 1.0)).rgb;

        return mix(color, dstColor, value);
    }

    vec3 dstColor = unpackUnorm4x8(colorOrHSV).rgb;

    return mix(color, dstColor, value);
}

#endif // GLSL