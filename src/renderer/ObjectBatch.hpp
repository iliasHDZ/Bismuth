#pragma once

#include <Geode/binding/GameObject.hpp>
#include <common.hpp>
#include <Geode/Geode.hpp>
#include <vector>

#include "Buffer.hpp"
#include "Geode/cocos/textures/CCTexture2D.h"
#include "ObjectSpriteUnpacker.hpp"
#include "glm/fwd.hpp"

using namespace geode;

/*
    These are the vertex attributes of the
    object batch. They are written in this
    macro so you don't have duplicate code
    with the and the attrib pointer calls.

    ATTRIB(attributeLocation, type, name)

    NOTE: The positionOffset attribute is this
          vertex' offset from the position of
          the object this sprite belongs to.
*/
#define OBJECT_VERTEX_ATTRIBUTES(ATTRIB) \
    ATTRIB(0, vec2, positionOffset) \
    ATTRIB(1, vec2, texCoord) \
    ATTRIB(2, u32,  srbIndex) \
    ATTRIB(3, u16,  colorChannel) \
    ATTRIB(4, u8,   spriteSheet) \
    ATTRIB(5, u8,   shaderSprite)

////////////////////////////////////////////////

#define VERTEX_ATTRIBUTE_AS_STRUCT_MEMBER(ID, TYPE, NAME) \
    TYPE NAME;

#define VERTICIES_PER_QUAD 4
#define INDICIES_PER_QUAD  6

struct ObjectVertex {
    OBJECT_VERTEX_ATTRIBUTES(VERTEX_ATTRIBUTE_AS_STRUCT_MEMBER);
};

struct ObjectQuad {
    union {
        ObjectVertex verticies[VERTICIES_PER_QUAD];
        struct {
            ObjectVertex bl;
            ObjectVertex br;
            ObjectVertex tl;
            ObjectVertex tr;
        };
    };
};

struct ObjectIndicies {
    u32 indicies[INDICIES_PER_QUAD];
};

////////////////////////////////////////////////

class Renderer;

struct SpriteVertexTransforms {
    glm::vec2 positionBottomLeft;
    glm::vec2 positionRight;
    glm::vec2 positionUp;
    glm::vec2 texCoordBottomLeft;
    glm::vec2 texCoordRight;
    glm::vec2 texCoordUp;
};

class ObjectBatch : public ObjectSpriteUnpackerDelegate {
public:
    inline ObjectBatch(Renderer& renderer)
        : renderer(renderer), unpacker(*this) {}
    ~ObjectBatch();

    SpriteVertexTransforms getSpriteVertexTransform(
        cocos2d::CCSprite* sprite,
        const cocos2d::CCAffineTransform& transform,
        SpriteSheet spriteSheet
    );

    void prepareSpriteMeshWrite(
        GameObject* parentObject,
        cocos2d::CCSprite* sprite,
        SpriteType type,
        const cocos2d::CCAffineTransform& transform
    );

    void writeSpriteVertex(glm::vec2 pos);
    void writeSpriteIndex(u32 index);

    void receiveUnpackedSprite(
        GameObject* parentObject,
        cocos2d::CCSprite* sprite,
        SpriteType type,
        const cocos2d::CCAffineTransform& transform
    ) override;

    void writeGameObject(GameObject* object);

    void finishWriting();

    inline void bind() {
        glBindVertexArray(vao);
        indexBuffer->bindAs(GL_ELEMENT_ARRAY_BUFFER);
    }

    // inline u32 indexCount() {
    //     return quadCount * 6;
    // }

    inline usize getQuadCount() {
        return quadCount;
    }

    inline void setSpriteSheetFilter(SpriteSheet sheet) {
        spriteSheetFilter = sheet;
    }

    usize generateCulledIndicies();

    usize draw();

private:
    void prepareVAO();

private:
    Renderer& renderer;
    ObjectSpriteUnpacker unpacker;

    SpriteSheet spriteSheetFilter = (SpriteSheet)-1;

    // This is only used when writing. After writing, it is cleared.
    std::vector<ObjectQuad> quads;
    usize currentQuadIndex = 0;
    usize prevCulledIndiciesCount = 0;

    Buffer* vertexBuffer = nullptr;
    Buffer* indexBuffer = nullptr;

    std::vector<u32> quadsSrbIndicies;
    std::vector<ObjectIndicies> culledIndicies;

    u32 vao = 0;
    u32 quadCount = 0;

    std::vector<u32> indicies;
    std::vector<ObjectVertex> verticies;
    u32 vertexCount = 0;
    u32 indexCount  = 0;

    SpriteVertexTransforms currentSpriteVertexTransforms;
    glm::vec2 currentSpriteObjectStartPosition;
    u32 currentSpriteVertexIndex;
    u32 currentSpriteSRBIndex;
    u16 currentSpriteColorChannel;
    u8  currentSpriteSpriteSheet;
};