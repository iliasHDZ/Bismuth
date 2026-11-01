#include "ObjectBatch.hpp"
#include "BProfiler.hpp"
#include "Renderer.hpp"
#include "SpriteMeshDictionary.hpp"
#include "common.hpp"
#include "glm/fwd.hpp"
#include "math/ConvexList.hpp"
#include "math/ConvexPolygon.hpp"
#include <culling/SpriteDrawMap.hpp>
#include <string>
#include "../profiler.hpp"

using namespace geode::prelude;

#define QUAD_BL 0
#define QUAD_BR 1
#define QUAD_TL 2
#define QUAD_TR 3

ObjectBatch::~ObjectBatch() {
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
    SpriteSheet spriteSheet = ObjectUtils::getSpritesheetOfObject(object, type);
    if (spriteSheetFilter != (SpriteSheet)-1 && spriteSheet != spriteSheetFilter)
        return;

    u32 colorChannel = ObjectUtils::getSpriteColorChannel(type, object, sprite);
    if (type == SpriteType::DETAIL)
        colorChannel |= A_COLOR_CHANNEL_IS_SPRITE_DETAIL;

    currentSpriteVertexTransforms    = getSpriteVertexTransform(sprite, transform, spriteSheet);
    currentSpriteObjectStartPosition = ccPointToGLM(object->m_startPosition);

    currentSpriteSRBIndex    = renderer.getObjectSRBIndex(object);
    currentSpriteVertexIndex = verticies.size();
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

    vertex.spriteIndex = currentSpriteIndex;
}

/*
void ObjectBatch::writeSpriteIndex(u32 index) {
    indicies.push_back(currentSpriteVertexIndex + index);
}
*/

void ObjectBatch::writeSpriteMeshFromConvexList(const ConvexList& list) {
    u32 vertexIndex = 0;
    list.triangulate([&](const glm::vec2& p1, const glm::vec2& p2, const glm::vec2& p3) {
        writeSpriteVertex(p1);
        writeSpriteVertex(p2);
        writeSpriteVertex(p3);
        // writeSpriteIndex(vertexIndex + 0);
        // writeSpriteIndex(vertexIndex + 1);
        // writeSpriteIndex(vertexIndex + 2);
        vertexIndex += 3;
    });
}

static SpriteDrawMap drawMap;

void ObjectBatch::addSprite(
    GameObject* object,
    cocos2d::CCSprite* sprite,
    SpriteType type,
    const cocos2d::CCAffineTransform& transform
) {
    if (object->m_hasNoGlow && type == SpriteType::GLOW)
        return;

    spriteCount++;

    prepareSpriteMeshWrite(object, sprite, type, transform);

    usize trianglesStart = verticies.size() / 3;

    ConvexList* spriteMesh = SpriteMeshDictionary::getSpriteMeshForSprite(sprite);

    drawMap.addSprite(object, sprite, type, {});

    currentSpriteIndex = inputSprites.size();

    if (spriteMesh) {
        writeSpriteMeshFromConvexList(*spriteMesh);
    } else {
        writeSpriteVertex({ 0, 0 });
        writeSpriteVertex({ 1, 0 });
        writeSpriteVertex({ 1, 1 });
        writeSpriteVertex({ 1, 1 });
        writeSpriteVertex({ 0, 1 });
        writeSpriteVertex({ 0, 0 });

        // writeSpriteIndex(QUAD_BL);
        // writeSpriteIndex(QUAD_TL);
        // writeSpriteIndex(QUAD_TR);
        // writeSpriteIndex(QUAD_BL);
        // writeSpriteIndex(QUAD_TR);
        // writeSpriteIndex(QUAD_BR);
    }

    usize trianglesCount = verticies.size() / 3 - trianglesStart;

    inputSprites.push_back({
        .objectIndex = (u32)currentSpriteSRBIndex,
        .spriteType = (u32)type,
        .indexStart = (u32)trianglesStart,
        .indexCount = (u32)trianglesCount
    });

    /*
    preCulledSprites.push_back({
        .groupCombinationIndex = renderer.getGroupManager().getGroupCombinationIndexForObject(object),
        .startPosition = ccPointToGLM(object->getStartPos())
    });
    */

    ZLayer zLayer = object->getObjectZLayer();
    SpriteSheet spriteSheet = ObjectUtils::getSpritesheetOfObject(object, type);

    usedLayerKeys.insert({zLayer, spriteSheet, false});
    usedLayerKeys.insert({zLayer, spriteSheet, true});

    workGroupScheduler.addSprite(object, sprite, type, currentSpriteIndex, trianglesCount);
}

void ObjectBatch::writeGameObject(GameObject* object) {
    float originalScaleX = object->getScaleX();
    float originalScaleY = object->getScaleY();

    if (object->m_usesAudioScale) {
        object->setScaleX(object->m_scaleX);
        object->setScaleY(object->m_scaleY);
    }

    ObjectUtils::unpackObjectIntoSprites(object, [&](const UnpackedSprite& sprite) {
        addSprite(sprite.parentObject, sprite.sprite, sprite.type, sprite.transform);
    });

    object->setScaleX(originalScaleX);
    object->setScaleY(originalScaleY);
}

void ObjectBatch::finishWriting() {
    assert(vertexBuffer == nullptr);

    storeGLStates();

    vertexCount = verticies.size();
    indexCount  = indicies.size();

    vertexBuffer      = Buffer::createStaticDraw("VertexBuffer", verticies.data(), vertexCount * sizeof(ObjectVertex));
    // indexBuffer       = Buffer::createStaticDraw("ObjectIndexBuffer", indicies.data(), indexCount * sizeof(u32));
    inputSpriteBuffer = Buffer::createStaticDraw("InputSpriteBuffer", inputSprites.data(), inputSprites.size() * sizeof(InputSprite));
    // preCulledSpritesBuffer = Buffer::createStaticDraw("Pre-culled sprites buffer", preCulledSprites.data(), preCulledSprites.size() * sizeof(PreCulledSprite));

    rawSpriteBuffer   = RawSprite::Buffer::create(getSpriteCount(), GL_DYNAMIC_DRAW, false);
    culledIndexBuffer = Buffer::createGPUExclusive("CulledTriIndexBuffer", vertexCount / 3 * sizeof(u32));

    verticies.clear();
    indicies.clear();
    inputSprites.clear();
    // preCulledSprites.clear();

    prepareVAO();
    restoreGLStates();
}

void ObjectBatch::draw(const LayerKey& layer) {
    PROFILER_PUSH("ObjectBatch::draw");

    if (!sortedBatch->workGroupRangePerLayerKey.contains(layer)) {
        PROFILER_POP();
        return;
    }
    const auto& range = sortedBatch->workGroupRangePerLayerKey[layer];

    sortedBatch->bindBuffers();
    prepareCullingComputeShader();
    renderer.bindCullingComputeShaderBuffers();
    sortedBatch->drawCommandBuffer->bindAs(GL_DRAW_INDIRECT_BUFFER);
    // glBindVertexArray(vao);
    vertexBuffer->bindAsStorageBuffer(VERTEX_BUFFER_BINDING);
    culledIndexBuffer->bindAsStorageBuffer(CULLED_TRI_INDEX_BUFFER);

    glMultiDrawArraysIndirect(
        GL_TRIANGLES,
        (void*)(range.offset * sizeof(DrawCommand)),
        range.count,
        sizeof(DrawCommand)
    );

    PROFILER_POP();
}

void ObjectBatch::predraw() {
    PROFILER_PUSH("ObjectBatch::predraw");
    cullingComputeShaderTimer.start();

    const auto& blendStates = renderer.getColorChannelBlendStates();

    // This uses a different thread to avoid a lag spike
    auto newSortedBatch = workGroupScheduler.fetchSortedBatchForBlendStates(blendStates);
    if (newSortedBatch)
        sortedBatch = newSortedBatch;
    else if (!sortedBatch) {
        workGroupScheduler.waitForJobs();
        sortedBatch = workGroupScheduler.fetchSortedBatchForBlendStates(blendStates);
        if (!sortedBatch)
            return;
    }

    Shader* computeShader = renderer.getCullingComputeShader();

    computeShader->use();

    prepareCullingComputeShader();
    renderer.bindCullingComputeShaderBuffers();
    sortedBatch->bindBuffers();

    glDispatchCompute(sortedBatch->workGroupCount, 1, 1);

    cullingComputeShaderTimer.stop();
    PROFILER_POP();
}

void ObjectBatch::prepareCullingComputeShader() {
    // log::info("{} {} {} {} {}", (void*)indexBuffer.get(), (void*)inputSpriteBuffer.get(), (void*)rawSpriteBuffer.get(), (void*)culledIndexBuffer.get(), (void*)shader);
    // indexBuffer->bindAsStorageBuffer(shader);
    inputSpriteBuffer->bindAsStorageBuffer(INPUT_SPRITE_BUFFER_BINDING);
    rawSpriteBuffer->bind();
    // culledIndexBuffer->bindAsStorageBuffer(CULLED_INDEX_BUFFER_BINDING);
    culledIndexBuffer->bindAsStorageBuffer(CULLED_INDEX_BUFFER_BINDING);
    // preCulledSpritesBuffer->bindAsStorageBuffer(PRE_CULLED_SPRITE_BUFFER_BINDING);
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

    // glBindVertexArray(vao);
    // vertexBuffer->bindAs(GL_ARRAY_BUFFER);

    // OBJECT_VERTEX_ATTRIBUTES(VERTEX_ATTRIBUTE_AS_ATTRIB_POINTER_CALL)
}