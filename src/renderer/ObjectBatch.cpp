#include "ObjectBatch.hpp"
#include "Renderer.hpp"
#include "SpriteMeshDictionary.hpp"
#include "common.hpp"
#include "glm/fwd.hpp"
#include "math/ConvexList.hpp"
#include "math/ConvexPolygon.hpp"
#include <culling/SpriteDrawMap.hpp>
#include "../profiler.hpp"

using namespace geode::prelude;

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

    currentSpriteVertexTransforms    = getSpriteVertexTransform(sprite, transform, spriteSheet);
    currentSpriteObjectStartPosition = ccPointToGLM(object->m_startPosition);

    currentSpriteObjectIndex = renderer.getObjectIndex(object);
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
        .objectIndex = (u32)currentSpriteObjectIndex,
        .spriteType = (u32)type,
        .indexStart = (u32)trianglesStart,
        .indexCount = (u32)trianglesCount
    });

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

    vertexCount = verticies.size();
    // indexCount  = indicies.size();

    vertexBuffer      = Buffer::createStaticDraw("VertexBuffer", verticies.data(), vertexCount * sizeof(ObjectVertex));
    inputSpriteBuffer = Buffer::createStaticDraw("InputSpriteBuffer", inputSprites.data(), inputSprites.size() * sizeof(InputSprite));

    rawSpriteBuffer   = RawSprite::Buffer::create(getSpriteCount(), GL_DYNAMIC_DRAW, false);
    culledIndexBuffer = Buffer::createGPUExclusive("CulledTriIndexBuffer", vertexCount / 3 * sizeof(u32));

    verticies.clear();
    // indicies.clear();
    inputSprites.clear();
}

void ObjectBatch::draw(const LayerKey& layer) {
    PROFILER_SCOPE("ObjectBatch::draw");

    if (!shouldDraw(layer))
        return;

    auto spriteSheet = renderer.getSpriteSheetTexture(layer.spriteSheet);
    if (!spriteSheet)
        return;

    const auto& range = sortedBatch->workGroupRangePerLayerKey[layer];

    auto shader = renderer.getDrawingShader();
    if (!shader->isInUse()) {
        shader->use();
        rawSpriteBuffer->bind();
        sortedBatch->drawCommandBuffer->bindAs(GL_DRAW_INDIRECT_BUFFER);
        vertexBuffer->bindAsStorageBuffer(VERTEX_BUFFER_BINDING);
        culledIndexBuffer->bindAsStorageBuffer(CULLED_TRI_INDEX_BUFFER);
    }

    ccGLBindTexture2D(spriteSheet->getName());

    if (layer.blending || layer.spriteSheet == SpriteSheet::GLOW)
        ccGLBlendFunc(GL_SRC_ALPHA, GL_ONE);
    else
        ccGLBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    glMultiDrawArraysIndirect(
        GL_TRIANGLES,
        (void*)(range.offset * sizeof(DrawCommand)),
        range.count,
        sizeof(DrawCommand)
    );
}

void ObjectBatch::predraw() {
    PROFILER_SCOPE("ObjectBatch::predraw");
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
}

void ObjectBatch::prepareCullingComputeShader() {
    inputSpriteBuffer->bindAsStorageBuffer(INPUT_SPRITE_BUFFER_BINDING);
    rawSpriteBuffer->bind();
}