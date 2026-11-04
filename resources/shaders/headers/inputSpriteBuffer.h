#define SPRITE_TYPE_BASE        0
#define SPRITE_TYPE_DETAIL      1
#define SPRITE_TYPE_BLACK       2
#define SPRITE_TYPE_GLOW        3
#define SPRITE_TYPE_BLACK_CHILD 4

/*
    This is the sprite info that is the input to the
    object culling compute shader.
*/
struct InputSprite {
    // Index into the object info buffer
    uint objectIndex;
    // The color type of the sprite, uses SPRITE_TYPE_* defines
    uint spriteType;
    // The place inside the index buffer where to find the first index belonging to this sprite
    uint indexStart;
    // Number of indicies belonging to this sprite
    uint indexCount;
};

STORAGE_BUFFER(INPUT_SPRITE_BUFFER_BINDING) InputSpriteBuffer {
    InputSprite u_inputSprites[CPP_ONLY(0)];
};