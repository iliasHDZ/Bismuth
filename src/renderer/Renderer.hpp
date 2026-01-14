#pragma once

#include <Geode/binding/GameObject.hpp>
#include <common.hpp>
#include "ObjectBatch.hpp"
#include "ObjectSorter.hpp"
#include "OverdrawView.hpp"
#include "Shader.hpp"
#include <unordered_map>
#include <set>

#include "DifferenceMode.hpp"
#include "GroupManager.hpp"
#include "ShaderSpriteManager.hpp"
#include "ObjectBatchNode.hpp"
#include "../../resources/shaders/shared.h"

using namespace geode;

class Renderer : public cocos2d::CCNode {
private:
    inline Renderer()
        : objectBatch(*this), groupManager(*this), differenceMode(*this),
          shaderSpriteManager(*this), overdrawView(*this) {}
    ~Renderer() override;

    bool init(PlayLayer* layer);

    void generateBatchNodes(ObjectSorter& sorter);

    void terminate();

    void prepareShaderUniforms();

    void prepareDynamicRenderingBuffer();

    void generateStaticRenderingBuffer(ObjectSorter& sorter);

    void draw() override;

    void updateDebugText();

protected:
    Shader* prepareDraw();

    void finishDraw();

    inline GroupCombinationState* getGroupCombinationStates() {
        if (drb == nullptr) return nullptr;
        return drb->groupCombinationStates;
    }

    friend class GroupManager;
    friend class ObjectBatchNode;

public:
    void update(float dt) override;

    bool isObjectInView(usize srbIndex);

    inline cocos2d::CCTexture2D* getSpriteSheetTexture(SpriteSheet sheet) {
        if ((i32)sheet < 0 || (i32)sheet >= (i32)SpriteSheet::COUNT)
            return nullptr;
        return spriteSheets[(i32)sheet];
    }

    inline PlayLayer* getPlayLayer() {
        return layer;
    }

    inline usize getObjectSRBIndex(GameObject* object) {
        return objectSRBIndicies[object];
    }

    inline bool isEnabled() { return enabled; }

    inline DifferenceMode* getDifferenceMode() {
        if (!differenceModeEnabled) return nullptr;
        return &differenceMode;
    }

    inline void toggleDebugText() {
        geode::log::info("syjdstu");
        debugTextEnabled = !debugTextEnabled;
    }

    inline bool isDifferenceModeEnabled() const { return differenceModeEnabled; }
    inline void setDifferenceModeEnabled(bool enabled) {
        differenceModeEnabled = enabled;
    }

    inline bool canEnableDisableIngame() const { return ingameEnableDisable; }

    inline void setGJBGLUpdateTime(u64 time) {gjbglUpdateTime = time; }
    inline void setTotalFrameTime(u64 time) { totalFrameTime = time; }

    inline bool isPaused() {
        return layer->m_isPaused;
    }

    inline bool isUseIndexCulling() const { return useIndexCulling; }

    bool useOptimizations();

    void setEnabled(bool enabled);

    inline GroupManager& getGroupManager() { return groupManager; }
    inline ShaderSpriteManager& getShaderSpriteManager() { return shaderSpriteManager; }

    void reset();

    void drawLine(const glm::vec2& p1, const glm::vec2& p2, const glm::vec4& color);

public:
    static Ref<Renderer> create(PlayLayer* layer);

    /**
     * Returns the current renderer. If the game
     * is not in a PlayLayer. This returns nullptr.
     */
    static Ref<Renderer> get();

private:
    PlayLayer* layer;

    bool enabled = false;
    bool debugTextEnabled = false;
    bool ingameEnableDisable = false;

    bool useIndexCulling = false;

    u64 rendererStartTime = 0;

    u64 drbGenerationTime = 0;
    u64 drawFuncTime = 0;
    u64 gjbglUpdateTime = 0;
    u64 totalFrameTime = 0;

    usize spritesOnScreen = 0;

    std::unordered_map<GameObject*, usize> objectSRBIndicies;

    // Used by the isObjectInView() function
    std::vector<GroupCombinationIndex> groupCombIndexPerObjectSRBIndex;
    std::vector<glm::vec2> startPositionPerObjectSRBIndex;
    glm::vec2 cameraCenterPos;

    std::vector<ObjectBatchNode*> batchNodes;

    cocos2d::CCTexture2D* spriteSheets[(i32)SpriteSheet::COUNT] = { nullptr };

    Ref<cocos2d::CCLabelBMFont> debugText;
    Ref<cocos2d::CCLabelBMFont> debugTextOutline1;
    Ref<cocos2d::CCLabelBMFont> debugTextOutline2;

    std::set<i16> usedGroupIds;
    std::set<i16> disabledGroups;

    GroupManager groupManager;

    ShaderSpriteManager shaderSpriteManager;

    ObjectBatch objectBatch;
    Shader* shader = nullptr;
    Shader* basicShader = nullptr;

    DynamicRenderingBuffer* drb = nullptr;
    Buffer* drbBuffer = nullptr;
    bool isDrbStorageBuffer;

    Buffer* srbBuffer = nullptr;

    RendererUniformBuffer uniforms;
    Buffer* uniformBuffer = nullptr;

    bool differenceModeEnabled = false;
    DifferenceMode differenceMode;
    OverdrawView overdrawView;

    float gameTimer = 0.0;

    i64 groupStateCount;
    i64 renderedGameObjectCount;
    i64 renderTime;
};

void storeGLStates();

void restoreGLStates();