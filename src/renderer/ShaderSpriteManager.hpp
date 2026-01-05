#pragma once

#include <common.hpp>
#include <unordered_map>
#include "../../resources/shaders/shared.h"
/*
#include "Buffer.hpp"
#include "../../resources/shaders/shared.h"
#include "Geode/cocos/sprite_nodes/CCSprite.h"
#include "Geode/cocos/sprite_nodes/CCSpriteFrame.h"

class Renderer;

class SpriteManager {
public:
    SpriteManager(Renderer& renderer)
        : renderer(renderer) {}

    u32 getSpriteCropAttributeForFrame(cocos2d::CCSpriteFrame* frame);

    void prepareSpriteCropBuffer();

private:
    u32 addRectAsSpriteCrop(const cocos2d::CCRect& rect);

private:
    Renderer& renderer;

    bool isBufferDirty = true;
    UPtr<Buffer> spriteCropBuffer;
    std::vector<SpriteCrop> spriteCropBufferData;

    std::unordered_map<cocos2d::CCSpriteFrame*, u32> spriteCropAttributeCache;
};
*/

class Renderer;

class ShaderSpriteManager {
public:
    inline ShaderSpriteManager(Renderer& renderer)
        : renderer(renderer)
    {
        init();
    }

    void init();

    u32 getShaderSpriteIndexOfSprite(cocos2d::CCSprite* sprite);

private:
    void registerShaderSprite(const char* frameName, u32 shaderSpriteIndex);

private:
    Renderer& renderer;

    std::unordered_map<cocos2d::CCSpriteFrame*, u32> shaderSpriteIndexPerSpriteFrame;
};