#pragma once

#include <common.hpp>
#include "ObjectBatch.hpp"
#include "Shader.hpp"

#include "../../resources/shaders/shared.h"

using namespace geode;

class Renderer : public cocos2d::CCNode {
private:
    inline Renderer()
        : objectBatch(this) {}
    ~Renderer() override;

    bool init(PlayLayer* layer);

    void terminate();

    void prepareShaderUniforms();

    void prepareDynamicRenderingBuffer();

    void draw() override;

    void updateDebugText();

public:
    void update(float dt) override;

    inline void toggleDebugText() {
        debugText->setVisible(!debugText->isVisible());
    }

    inline cocos2d::CCTexture2D* getSpriteSheetTexture(SpriteSheet sheet) {
        if ((i32)sheet < 0 || (i32)sheet >= (i32)SpriteSheet::COUNT)
            return nullptr;
        return spriteSheets[(i32)sheet];
    }

    inline PlayLayer* getPlayLayer() {
        return layer;
    }

public:
    static Ref<Renderer> create(PlayLayer* layer);

    /**
     * Returns the current renderer. If the game
     * is not in a PlayLayer. This returns nullptr.
     */
    static Ref<Renderer> get();

private:
    PlayLayer* layer;

    cocos2d::CCTexture2D* spriteSheets[(i32)SpriteSheet::COUNT] = { nullptr };

    Ref<cocos2d::CCLabelBMFont> debugText;

    ObjectBatch objectBatch;
    Shader* shader = nullptr;

    DynamicRenderingBuffer drb = { 0 };
    Buffer* drbBuffer = nullptr;

    i64 renderTime;
};

void storeGLStates();

void restoreGLStates();