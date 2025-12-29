#pragma once

#include <common.hpp>
#include "ObjectBatch.hpp"
#include "ObjectSorter.hpp"
#include "Shader.hpp"
#include <unordered_map>
#include <set>

#include "../../resources/shaders/shared.h"
#include "DifferenceMode.hpp"

using namespace geode;

class Renderer : public cocos2d::CCNode {
private:
    inline Renderer()
        : objectBatch(this), differenceMode(*this) {}
    ~Renderer() override;

    bool init(PlayLayer* layer);

    void terminate();

    void prepareShaderUniforms();

    void prepareDynamicRenderingBuffer();

    void generateStaticRenderingBuffer(ObjectSorter& sorter);

    void draw() override;

    void updateDebugText();

public:
    void update(float dt) override;

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
        debugTextEnabled = !debugTextEnabled;
    }

    inline bool isDifferenceModeEnabled() { return differenceModeEnabled; }
    inline void setDifferenceModeEnabled(bool enabled) {
        differenceModeEnabled = enabled;
    }

    void setEnabled(bool enabled);

    void moveGroup(i32 groupId, float deltaX, float deltaY);

    void toggleGroup(i32 groupId, bool visible);

    void resetGroups();

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

    std::unordered_map<GameObject*, usize> objectSRBIndicies;

    cocos2d::CCTexture2D* spriteSheets[(i32)SpriteSheet::COUNT] = { nullptr };

    Ref<cocos2d::CCLabelBMFont> debugText;

    std::set<i16> usedGroupIds;
    std::set<i16> disabledGroups;

    ObjectBatch objectBatch;
    Shader* shader = nullptr;

    DynamicRenderingBuffer* drb = nullptr;
    Buffer* drbBuffer = nullptr;

    Buffer* srbBuffer = nullptr;

    bool differenceModeEnabled = false;
    DifferenceMode differenceMode;

    float gameTimer = 0.0;

    i64 groupStateCount;
    i64 renderedGameObjectCount;
    i64 renderTime;
};

void storeGLStates();

void restoreGLStates();