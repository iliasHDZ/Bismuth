#pragma once

#include <common.hpp>

#include "ObjectBatch.hpp"

class ObjectBatchNode : public cocos2d::CCNode {
public:
    inline ObjectBatchNode(Renderer& renderer)
        : renderer(renderer), batch(renderer) {}

    bool init(SpriteSheet spriteSheet);

    void draw() override;

    inline void addGameObject(GameObject* object) {
        batch.writeGameObject(object);
    }

    inline void generateBatch() {
        batch.finishWriting();
    }

public:
    static inline Ref<ObjectBatchNode> create(Renderer& renderer, SpriteSheet spriteSheet) {
        auto ret = new ObjectBatchNode(renderer);
        if (ret->init(spriteSheet)) {
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }

private:
    Renderer& renderer;
    ObjectBatch batch;

    SpriteSheet spriteSheet;
    cocos2d::CCTexture2D* spriteSheetTexture;
};