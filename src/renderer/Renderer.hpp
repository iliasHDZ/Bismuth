#pragma once

#include <common.hpp>
#include "ObjectBatch.hpp"
#include "Shader.hpp"

using namespace geode;

class Renderer : public cocos2d::CCNode {
private:
    inline Renderer()
        : objectBatch(this) {}
    ~Renderer() override;

    bool init(PlayLayer* layer);

    void terminate();

    void draw() override;

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

    ObjectBatch objectBatch;
    Shader* shader;
};

void storeGLStates();

void restoreGLStates();