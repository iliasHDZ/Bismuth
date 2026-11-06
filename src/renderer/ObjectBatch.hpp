#pragma once

#include <Geode/binding/GameObject.hpp>
#include <common.hpp>
#include <Geode/Geode.hpp>
#include <vector>

#include "Buffer.hpp"
#include "glm/fwd.hpp"
#include "math/ConvexList.hpp"
#include "ObjectUtils.hpp"
#include "WorkGroupScheduler.hpp"
#include "../utils/GPUTimer.hpp"

using namespace geode;

class Renderer;

struct SpriteVertexTransforms {
    glm::vec2 positionBottomLeft;
    glm::vec2 positionRight;
    glm::vec2 positionUp;
    glm::vec2 texCoordBottomLeft;
    glm::vec2 texCoordRight;
    glm::vec2 texCoordUp;
};

class ObjectBatch {
public:
    inline ObjectBatch(Renderer& renderer)
        : renderer(renderer) {}

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
    // void writeSpriteIndex(u32 index);

    void writeSpriteMeshFromConvexList(const ConvexList& list);

    void addSprite(
        GameObject* parentObject,
        cocos2d::CCSprite* sprite,
        SpriteType type,
        const cocos2d::CCAffineTransform& transform
    );

    void writeGameObject(GameObject* object);

    void finishWriting();

    void draw(const LayerKey& layer);

    inline bool shouldDraw(const LayerKey& layer) const {
        if (!sortedBatch) return false;
        auto it = sortedBatch->workGroupRangePerLayerKey.find(layer);
        return it != sortedBatch->workGroupRangePerLayerKey.end() &&
               it->second.count > 0;
    }

    inline usize getVertexBufferSize() { return vertexCount * sizeof(ObjectVertex); }
    inline void setSpriteSheetFilter(SpriteSheet sheet) { spriteSheetFilter = sheet; }
    inline const std::set<LayerKey>& getUsedLayerIds() { return usedLayerKeys; }
    inline usize getSpriteCount() { return workGroupScheduler.getSpriteCount(); }

    void predraw();

    void prepareCullingComputeShader();

private:
    Renderer& renderer;

    SpriteSheet spriteSheetFilter = (SpriteSheet)-1;

    // Static buffers
    UPtr<Buffer> vertexBuffer = nullptr;
    UPtr<Buffer> indexBuffer = nullptr;
    UPtr<Buffer> inputSpriteBuffer = nullptr;

    // GPU buffers
    UPtr<RawSprite::Buffer> rawSpriteBuffer = nullptr;
    UPtr<Buffer> culledIndexBuffer = nullptr;

    GPUTimer cullingComputeShaderTimer { "Culling compute shader time" };

    WorkGroupScheduler workGroupScheduler;
    SortedObjectBatch* sortedBatch = nullptr;

    usize spriteCount = 0;

    std::vector<ObjectVertex> verticies;
    u32 vertexCount = 0;

    // std::vector<u32> indicies;
    // u32 indexCount = 0;

    std::vector<InputSprite> inputSprites;
    // std::vector<PreCulledSprite> preCulledSprites;

    std::set<LayerKey> usedLayerKeys;

    SpriteVertexTransforms currentSpriteVertexTransforms;
    glm::vec2 currentSpriteObjectStartPosition;
    u32 currentSpriteVertexIndex;
    u32 currentSpriteObjectIndex;
    u32 currentSpriteIndex;
};