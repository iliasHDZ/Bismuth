#include "ObjectBatch.hpp"
#include "Geode/cocos/cocoa/CCAffineTransform.h"
#include "Geode/cocos/sprite_nodes/CCSpriteFrame.h"
#include "ObjectSpriteUnpacker.hpp"
#include "Renderer.hpp"
#include "SpriteMeshDictionary.hpp"
#include "common.hpp"
#include "glm/fwd.hpp"
#include "math/ConvexList.hpp"
#include "math/ConvexPolygon.hpp"
#include <string>

using namespace geode::prelude;

#define QUAD_BL 0
#define QUAD_BR 1
#define QUAD_TL 2
#define QUAD_TR 3

ObjectBatch::~ObjectBatch() {
    if (vertexBuffer)
        Buffer::destroy(vertexBuffer);
    if (indexBuffer)
        Buffer::destroy(indexBuffer);
    if (vao)
        glDeleteVertexArrays(1, &vao);
}

SpriteVertexTransforms ObjectBatch::getSpriteVertexTransform(
    cocos2d::CCSprite* sprite,
    const cocos2d::CCAffineTransform& transform,
    SpriteSheet spriteSheet
) {
    CCRect crop = sprite->getTextureRect();
    
    CCTexture2D* texture = renderer.getSpriteSheetTexture(spriteSheet);
    if (texture == nullptr) return {};

    glm::vec2 posBottomLeft  = ccPointToGLM(CCPointApplyAffineTransform(sprite->getOffsetPosition(), transform));
    glm::vec2 posRightVector = glm::vec2(transform.a, transform.b) * crop.size.width;
    glm::vec2 posUpVector    = glm::vec2(transform.c, transform.d) * crop.size.height;

    glm::vec2 texBottomLeft;
    glm::vec2 texRightVector;
    glm::vec2 texUpVector;

    if (!sprite->isTextureRectRotated()) {
        texBottomLeft  = { crop.origin.x, crop.origin.y + crop.size.height };
        texRightVector = { crop.size.width, 0 };
        texUpVector    = { 0, -crop.size.height };
    } else {
        texBottomLeft  = { crop.origin.x, crop.origin.y };
        texRightVector = { 0, crop.size.width };
        texUpVector    = { crop.size.height, 0 };
    }
    
    if (sprite->isFlipX()) {
        posBottomLeft += posRightVector;
        posRightVector = -posRightVector;
    }
    if (sprite->isFlipY()) {
        posBottomLeft += posUpVector;
        posUpVector = -posUpVector;
    }

    float contentScaleFactor = CCDirector::get()->getContentScaleFactor();
    glm::vec2 texCoordFactor = glm::vec2(
        contentScaleFactor / texture->getPixelsWide(),
        contentScaleFactor / texture->getPixelsHigh()
    );

    texBottomLeft  *= texCoordFactor;
    texRightVector *= texCoordFactor;
    texUpVector    *= texCoordFactor;

    return {
        posBottomLeft, posRightVector, posUpVector,
        texBottomLeft, texRightVector, texUpVector
    };
}

void ObjectBatch::prepareSpriteMeshWrite(
    GameObject* object,
    cocos2d::CCSprite* sprite,
    SpriteType type,
    const cocos2d::CCAffineTransform& transform
) {
    SpriteSheet spriteSheet = unpacker.getSpritesheetOfObject(object, type);
    if (spriteSheetFilter != (SpriteSheet)-1 && spriteSheet != spriteSheetFilter)
        return;

    u32 colorChannel = type == SpriteType::DETAIL ? object->m_activeDetailColorID : object->m_activeMainColorID;

    bool isSpriteBlack = (sprite == object) ? object->m_isObjectBlack : object->m_isColorSpriteBlack;
    if (isSpriteBlack)
        colorChannel = COLOR_CHANNEL_BLACK;
    if (type == SpriteType::GLOW && object->m_glowColorIsLBG)
        colorChannel = COLOR_CHANNEL_LBG;
    if (type == SpriteType::DETAIL)
        colorChannel |= A_COLOR_CHANNEL_IS_SPRITE_DETAIL;

    currentSpriteVertexTransforms    = getSpriteVertexTransform(sprite, transform, spriteSheet);
    currentSpriteObjectStartPosition = ccPointToGLM(object->m_startPosition);

    currentSpriteSRBIndex     = renderer.getObjectSRBIndex(object);
    currentSpriteColorChannel = colorChannel;
    currentSpriteSpriteSheet  = (u8)spriteSheet;
    currentSpriteVertexIndex  = verticies.size();
}

void ObjectBatch::writeSpriteVertex(glm::vec2 pos) {
    isize index = verticies.size();
    verticies.resize(index + 1);
    ObjectVertex& vertex = verticies[index];

    auto& transforms = currentSpriteVertexTransforms;

    vertex.positionOffset = transforms.positionRight * pos.x +
                            transforms.positionUp    * pos.y +
                            transforms.positionBottomLeft - currentSpriteObjectStartPosition;
    
    vertex.texCoord = transforms.texCoordRight * pos.x +
                      transforms.texCoordUp    * pos.y +
                      transforms.texCoordBottomLeft;

    vertex.srbIndex     = currentSpriteSRBIndex;
    vertex.colorChannel = currentSpriteColorChannel;
    vertex.spriteSheet  = currentSpriteSpriteSheet;
}

void ObjectBatch::writeSpriteIndex(u32 index) {
    indicies.push_back(currentSpriteVertexIndex + index);
}

void ObjectBatch::writeSpriteMeshFromConvexList(const ConvexList& list) {
    u32 vertexIndex = 0;
    list.triangulate([&](const glm::vec2& p1, const glm::vec2& p2, const glm::vec2& p3) {
        writeSpriteVertex(p1);
        writeSpriteVertex(p2);
        writeSpriteVertex(p3);
        writeSpriteIndex(vertexIndex + 0);
        writeSpriteIndex(vertexIndex + 1);
        writeSpriteIndex(vertexIndex + 2);
        vertexIndex += 3;
    });
}

void ObjectBatch::receiveUnpackedSprite(
    GameObject* object,
    cocos2d::CCSprite* sprite,
    SpriteType type,
    const cocos2d::CCAffineTransform& transform
) {
    prepareSpriteMeshWrite(object, sprite, type, transform);

    ConvexList* spriteMesh = SpriteMeshDictionary::getSpriteMeshForSprite(sprite);

    if (spriteMesh) {
        writeSpriteMeshFromConvexList(*spriteMesh);
    } else {
        writeSpriteVertex({ 0, 0 });
        writeSpriteVertex({ 1, 0 });
        writeSpriteVertex({ 0, 1 });
        writeSpriteVertex({ 1, 1 });

        writeSpriteIndex(QUAD_BL);
        writeSpriteIndex(QUAD_TL);
        writeSpriteIndex(QUAD_TR);
        writeSpriteIndex(QUAD_BL);
        writeSpriteIndex(QUAD_TR);
        writeSpriteIndex(QUAD_BR);
    }
}

void ObjectBatch::writeGameObject(GameObject* object) {
    float originalScaleX = object->getScaleX();
    float originalScaleY = object->getScaleY();

    if (object->m_usesAudioScale) {
        object->setScaleX(object->m_scaleX);
        object->setScaleY(object->m_scaleY);
    }

    unpacker.unpackObject(object);

    object->setScaleX(originalScaleX);
    object->setScaleY(originalScaleY);
}

void ObjectBatch::finishWriting() {
    /*
    quadCount = currentQuadIndex;

    indicies.resize(quadCount);

    usize vertexIndex = 0;
    for (usize i = 0; i < quadCount; i++) {
        ObjectIndicies& objIndicies = indicies[i];
        objIndicies.indicies[0] = vertexIndex + QUAD_BL;
        objIndicies.indicies[1] = vertexIndex + QUAD_TL;
        objIndicies.indicies[2] = vertexIndex + QUAD_TR;
        objIndicies.indicies[3] = vertexIndex + QUAD_BL;
        objIndicies.indicies[4] = vertexIndex + QUAD_TR;
        objIndicies.indicies[5] = vertexIndex + QUAD_BR;
        vertexIndex += 4;
    }
    
    quadsSrbIndicies.resize(quadCount);

    for (usize i = 0; i < quadCount; i++)
        quadsSrbIndicies[i] = quads[i].verticies[0].srbIndex;
    */

    storeGLStates();

    if (vertexBuffer) {
        Buffer::destroy(vertexBuffer);
        vertexBuffer = nullptr;
    }

    if (indexBuffer) {
        Buffer::destroy(indexBuffer);
        indexBuffer = nullptr;
    }

    vertexCount = verticies.size();
    indexCount  = indicies.size();



    vertexBuffer = Buffer::createStaticDraw(verticies.data(), vertexCount * sizeof(ObjectVertex));
    // if (renderer.isUseIndexCulling()) {
    //     culledIndicies.resize(quadCount);
        indexBuffer = Buffer::createStaticDraw(indicies.data(), indexCount * sizeof(u32));
    // } else {
    //     indexBuffer = Buffer::createStaticDraw(indicies.data(), indicies.size() * sizeof(ObjectIndicies));
    //     indicies.clear();
    // }

    indicies.clear();
    verticies.clear();
    
    prepareVAO();
    restoreGLStates();
}

usize ObjectBatch::generateCulledIndicies() {
//    if (renderer.isPaused())
//        return prevCulledIndiciesCount;
//
//    usize outIndex = 0;
//
//    for (usize i = 0; i < quadCount; i++) {
//        if (!renderer.isObjectInView(quadsSrbIndicies[i]))
//            continue;
//
//        culledIndicies[outIndex] = indicies[i];
//        outIndex++;
//    }
//
//    indexBuffer->write(culledIndicies.data(), outIndex * sizeof(ObjectIndicies));
//    prevCulledIndiciesCount = outIndex * INDICIES_PER_QUAD;
//    return outIndex * INDICIES_PER_QUAD;
    return 0;
}

usize ObjectBatch::draw() {
    bind();
//    usize indiciesCount = indexCount();
//    if (renderer.isUseIndexCulling())
//        indiciesCount = generateCulledIndicies();
//
//    if (indiciesCount == 0)
//        return 0;

    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, nullptr);
    return indexCount / INDICIES_PER_QUAD;
}

struct AttribTypeInfo {
    i32 openGlType;
    i32 componentCount;
    u32 size;
};

static AttribTypeInfo getInfoOfAttributeTypeString(std::string type) {
    if (type == "float") return { GL_FLOAT, 1, sizeof(float) * 1 };
    if (type == "vec2")  return { GL_FLOAT, 2, sizeof(float) * 2 };
    if (type == "vec3")  return { GL_FLOAT, 3, sizeof(float) * 3 };
    if (type == "vec4")  return { GL_FLOAT, 4, sizeof(float) * 4 };

    if (type == "i8")                   return { GL_BYTE,  1, sizeof(i8)  };
    if (type == "i16")                  return { GL_SHORT, 1, sizeof(i16) };
    if (type == "int" || type == "i32") return { GL_INT,   1, sizeof(i32) };

    if (type == "u8")  return { GL_UNSIGNED_BYTE,  1, sizeof(u8)  };
    if (type == "u16") return { GL_UNSIGNED_SHORT, 1, sizeof(u16) };
    if (type == "u32") return { GL_UNSIGNED_INT,   1, sizeof(u32) };
    
    return { 0, 0 };
}

static void vertexAttribPointer(u32 id, const AttribTypeInfo& info, usize stride, usize offset) {
    if (info.openGlType == GL_FLOAT)
        glVertexAttribPointer(id, info.componentCount, info.openGlType, GL_FALSE, sizeof(ObjectVertex), (void*)offset);
    else if (info.openGlType == GL_DOUBLE)
        glVertexAttribLPointer(id, info.componentCount, info.openGlType, sizeof(ObjectVertex), (void*)offset);
    else
        glVertexAttribIPointer(id, info.componentCount, info.openGlType, sizeof(ObjectVertex), (void*)offset);
}

#define VERTEX_ATTRIBUTE_AS_ATTRIB_POINTER_CALL(ID, TYPE, NAME) \
    { \
        auto info = getInfoOfAttributeTypeString(#TYPE); \
        vertexAttribPointer(ID, info, sizeof(ObjectVertex), offsetof(ObjectVertex, NAME)); \
        glEnableVertexAttribArray(ID); \
    }

void ObjectBatch::prepareVAO() {
    if (vao == 0)
        glGenVertexArrays(1, &vao);

    glBindVertexArray(vao);
    vertexBuffer->bindAs(GL_ARRAY_BUFFER);

    OBJECT_VERTEX_ATTRIBUTES(VERTEX_ATTRIBUTE_AS_ATTRIB_POINTER_CALL)
}