#define A_COLOR_CHANNEL_IS_SPRITE_DETAIL 0x1000

#define OBJECT_FLAG_USES_AUDIO_SCALE   (1 << 0)
#define OBJECT_FLAG_CUSTOM_AUDIO_SCALE (1 << 1)
#define OBJECT_FLAG_IS_ORB             (1 << 2)
#define OBJECT_FLAG_IS_INVISIBLE_BLOCK (1 << 3)
#define OBJECT_FLAG_SPECIAL_GLOW_COLOR (1 << 4)
#define OBJECT_FLAG_IS_GLOW_COLOR_LBG  (1 << 5)
#define OBJECT_FLAG_HAS_BASE_HSV       (1 << 6)
#define OBJECT_FLAG_HAS_DETAIL_HSV     (1 << 7)
#define OBJECT_FLAG_IS_STATIC_OBJECT   (1 << 8)

/*
    Rendering info of an object that never changes.
*/
struct ObjectInfo {
    float rotationSpeed;
// Flags are the macro variables beginning with OBJECT_FLAG_*
    int flags;
    float audioScaleMin;
    float audioScaleMax;
    float fadeMargin;
    uint opacityValues;
    uint baseColorChannelId;
    PackedHSV baseHSV;
    uint detailColorChannelId;
    PackedHSV detailHSV;
};

/*
    This is the buffer that contains all the info of
    every object required in shader processing
*/
STORAGE_BUFFER(OBJECT_INFO_BUFFER_BINDING) ObjectInfoBuffer {
    ObjectInfo u_objects[CPP_ONLY(0)];
};