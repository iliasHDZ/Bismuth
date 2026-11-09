#pragma once

#include <Geode/binding/GameObject.hpp>
#include <common.hpp>
#include "Geode/cocos/sprite_nodes/CCSpriteBatchNode.h"
#include "ObjectBatch.hpp"
#include "ObjectUtils.hpp"
#include "OverdrawView.hpp"
#include "Shader.hpp"
#include "DebugText.hpp"
#include "DifferenceMode.hpp"
#include "GroupManager.hpp"
#include "ObjectBatchNode.hpp"
#include <unordered_map>

using namespace geode;

PackedHSV packHSV(const cocos2d::ccHSVValue& hsv);

struct TriggerStatistics {
    usize moveTriggerApplyCount = 0;
    usize rotateTriggerApplyCount = 0;
    usize followTriggerApplyCount = 0;
};

class Renderer : public cocos2d::CCNode {
private:
    inline Renderer()
        : objectBatch(*this), groupManager(*this), differenceMode(*this) {}
    ~Renderer() override;

    bool init(PlayLayer* layer, cocos2d::CCArray* objects);

    void generateBatchNodes();

    void terminate();

    void prepareShaderUniforms();

    void prepareColorChannelBuffer();

    void generateObjectInfoBuffer(std::vector<GameObject*>& objects);

    void draw() override;

    void finish();

    void updateDebugText();

    void bindCullingComputeShaderBuffers();

    void fetchBlendingStates();

    friend class FinishNode;
    friend class ObjectBatch;

public:
    void update(float dt) override;

    cocos2d::CCSpriteBatchNode* getSpriteBatchNodeWithLayerId(LayerKey id);

    inline bool isColorChannelBlending(i32 channel) {
        return colorChannelsIsBlending[channel];
    }

    inline cocos2d::CCTexture2D* getSpriteSheetTexture(SpriteSheet sheet) {
        if ((i32)sheet < 0 || (i32)sheet >= (i32)SpriteSheet::COUNT)
            return nullptr;
        return spriteSheets[(i32)sheet];
    }

    inline PlayLayer* getPlayLayer() {
        return layer;
    }

    inline usize getObjectIndex(GameObject* object) {
        return objectIndicies[object];
    }

    inline bool isEnabled() { return enabled; }

    inline DifferenceMode* getDifferenceMode() {
        if (!differenceModeEnabled) return nullptr;
        return &differenceMode;
    }

    inline void toggleDebugText() {
        debugTextEnabled = !debugTextEnabled;
    }

    inline bool isDifferenceModeEnabled() const { return differenceModeEnabled; }
    inline void setDifferenceModeEnabled(bool enabled) {    
        if (!useOptimizations())
            differenceModeEnabled = enabled;
    }

    inline bool isDebugToolsEnabled() const { return debugToolsEnabled; }

    inline bool isPaused() {
        return layer->m_isPaused;
    }

    inline ObjectBatch& getObjectBatch() { return objectBatch; }

    inline Shader* getCullingComputeShader() const { return cullingComputeShader.get(); }
    inline Shader* getDrawingShader() const { return shader.get(); }

    bool useOptimizations();

    void setEnabled(bool enabled);

    inline GroupManager& getGroupManager() { return groupManager; }

    inline const ColorChannelBlendStates& getColorChannelBlendStates() { return colorChannelsIsBlending; }
    
    inline TriggerStatistics& getTriggerStatistics() {
        if (!triggerStatistics) triggerStatistics = TriggerStatistics {};
        return triggerStatistics.value();
    }

    void reset();

public:
    static Ref<Renderer> create(PlayLayer* layer, cocos2d::CCArray* objects);

    /**
     * Returns the current renderer. If the game
     * is not in a PlayLayer. This returns nullptr.
     */
    static Ref<Renderer> get();

private:
    PlayLayer* layer;

    bool enabled = false;
    bool debugTextEnabled = false;
    bool debugToolsEnabled = false;

    u64 rendererStartTime = 0;

    std::unordered_map<GameObject*, usize> objectIndicies;

    glm::vec2 cameraCenterPos;

    std::set<cocos2d::CCSpriteBatchNode*> vanillaBatchNodes;
    std::vector<ObjectBatchNode*> batchNodes;

    cocos2d::CCTexture2D* spriteSheets[(i32)SpriteSheet::COUNT] = { nullptr };

    ColorChannelBlendStates colorChannelsIsBlending;

    DebugText* debugText;

    std::optional<TriggerStatistics> triggerStatistics;

    GPUTimer renderingTimer { "Rendering time" };

    GroupManager groupManager;

    ObjectBatch objectBatch;
    UPtr<Shader> shader = nullptr;
    UPtr<Shader> basicShader = nullptr;
    UPtr<Shader> cullingComputeShader = nullptr;

    ColorChannelBuffer* colorChannelBuffer;
    UPtr<Buffer> colorChannelBufferObject = nullptr;

    UPtr<Buffer> objectInfoBuffer = nullptr;

    GameStateBuffer gameState;
    UPtr<Buffer> gameStateBuffer = nullptr;

    bool differenceModeEnabled = false;
    DifferenceMode differenceMode;

    float gameTimer = 0.0;

    i64 renderedGameObjectCount;
};