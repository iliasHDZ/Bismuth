#pragma once

#include <common.hpp>
#include <Geode/Geode.hpp>
#include <vector>

#include "Buffer.hpp"

using namespace geode;

/*
    These are the vertex attributes of the
    object batch. They are written in this
    macro so you don't have duplicate code
    with the and the attrib pointer calls.

    ATTRIB(attributeId, type, name)
*/
#define OBJECT_VERTEX_ATTRIBUTES(ATTRIB) \
    ATTRIB(0, vec2, position) \
    ATTRIB(1, vec2, texCoord) \
    ATTRIB(2, u16,  colorChannel) \
    ATTRIB(3, u8,   spriteSheet)

////////////////////////////////////////////////

#define VERTEX_ATTRIBUTE_AS_STRUCT_MEMBER(ID, TYPE, NAME) \
    TYPE NAME;

struct ObjectVertex {
    OBJECT_VERTEX_ATTRIBUTES(VERTEX_ATTRIBUTE_AS_STRUCT_MEMBER);
};

struct ObjectQuad {
    ObjectVertex verticies[4];
};

////////////////////////////////////////////////

enum class SpriteSheet {
    GAME_1,
    GAME_2,
    TEXT,
    FIRE,
    SPECIAL,
    GLOW,
    PIXEL,
    _UNK,
    PARTICLE,

    COUNT
};

class Renderer;

class ObjectBatch {
public:
    inline ObjectBatch(Renderer* renderer)
        : renderer(renderer) {}
    ~ObjectBatch();

    void reserveForGameObject(GameObject* object);

    void allocateReservations();

    // textureCrop values go from 0.0 to 1.0 instead of to the size of the texture
    ObjectQuad& writeQuad(
        cocos2d::CCAffineTransform absoluteNodeTransform,
        cocos2d::CCPoint innerVertexOffset,
        cocos2d::CCSize contentSize,
        cocos2d::CCRect textureCrop,
        bool textureCropRotated,
        bool flipX, bool flipY
    );

    void writeGameObjects(Ref<cocos2d::CCArray> objects);

    void writeGameObject(GameObject* object);

    void finishWriting();

    inline void bind() {
        glBindVertexArray(vao);
        indexBuffer->bindAs(GL_ELEMENT_ARRAY_BUFFER);
    }

    inline u32 indexCount() {
        return quadCount * 6;
    }

    inline usize getQuadCount() {
        return quadCount;
    }

    void draw();

private:
    void writeSprite(
        cocos2d::CCSprite* sprite,
        cocos2d::CCAffineTransform transform,
        SpriteSheet sheet,
        u32 colorChannel,
        bool isBlack
    );

    void prepareVAO();

private:
    Renderer* renderer;

    usize reservedQuadCount = 0;

    // This is only used when writing. After writing, it is cleared.
    std::vector<ObjectQuad> quads;
    usize currentQuadIndex = 0;

    Buffer* vertexBuffer = nullptr;
    Buffer* indexBuffer = nullptr;
    u32 vao = 0;
    u32 quadCount = 0;
};