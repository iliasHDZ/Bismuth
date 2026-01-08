#include "ShaderSpriteManager.hpp"
#include "Geode/cocos/sprite_nodes/CCSpriteFrame.h"

using namespace geode::prelude;

/*
#include "Geode/cocos/textures/CCTexture2D.h"

u32 SpriteManager::getSpriteCropAttributeForFrame(CCSpriteFrame* frame) {
    auto it = spriteCropAttributeCache.find(frame);
    if (it != spriteCropAttributeCache.end())
        return it->second;

    u32 index = addRectAsSpriteCrop(frame->getRectInPixels());

    spriteCropAttributeCache[frame] = index;
    return index;
}

void SpriteManager::prepareSpriteCropBuffer() {
    if (isBufferDirty) {
        spriteCropBuffer = nullptr;
        spriteCropBuffer = UPtr<Buffer>(Buffer::createStaticDraw(
            spriteCropBufferData.data(),
            spriteCropBufferData.size() * sizeof(SpriteCrop)
        ));
    }

    spriteCropBuffer->bindAsStorageBuffer(SPRITE_CROP_BUFFER_BINDING);
}

u32 SpriteManager::addRectAsSpriteCrop(const CCRect& rect) {
    u32 index = spriteCropBufferData.size();
    spriteCropBufferData.resize(index + 1);
    SpriteCrop& crop = spriteCropBufferData[index];

    crop.xCrop = ((u32)rect.getMaxX() & 0xffff) << 16 | ((u32)rect.getMinX() & 0xffff);
    crop.yCrop = ((u32)rect.getMaxY() & 0xffff) << 16 | ((u32)rect.getMinY() & 0xffff);

    isBufferDirty = true;
    return index;
}
*/

void ShaderSpriteManager::init() {
    // registerShaderSprite("block005b_05_001.png", SHADER_SPRITE_SOLID_BLOCK);
    // registerShaderSprite("block009c_base_001.png", SHADER_SPRITE_SOLID_BLOCK);
    // registerShaderSprite("lightsquare_01_01_color_001.png", SHADER_SPRITE_SOLID_BLOCK);
    // registerShaderSprite("lightsquare_01_02_color_001.png", SHADER_SPRITE_SOLID_BLOCK);
    // registerShaderSprite("lightsquare_01_03_color_001.png", SHADER_SPRITE_SOLID_BLOCK);
    // registerShaderSprite("lightsquare_01_04_color_001.png", SHADER_SPRITE_SOLID_BLOCK);
    // registerShaderSprite("lightsquare_01_05_color_001.png", SHADER_SPRITE_SOLID_BLOCK);
    // registerShaderSprite("lightsquare_01_06_color_001.png", SHADER_SPRITE_SOLID_BLOCK);
    // registerShaderSprite("lightsquare_01_07_color_001.png", SHADER_SPRITE_SOLID_BLOCK);
    // registerShaderSprite("lightsquare_01_08_color_001.png", SHADER_SPRITE_SOLID_BLOCK);
    // registerShaderSprite("whiteSquare20_001.png", SHADER_SPRITE_SOLID_BLOCK);
    // registerShaderSprite("whiteSquare40_001.png", SHADER_SPRITE_SOLID_BLOCK);
    // registerShaderSprite("whiteSquare60_001.png", SHADER_SPRITE_SOLID_BLOCK);

    // registerShaderSprite("lighttriangle_01_02_color_001.png", SHADER_SPRITE_SOLID_SLOPE);
    // registerShaderSprite("lighttriangle_01_04_color_001.png", SHADER_SPRITE_SOLID_SLOPE);
}

u32 ShaderSpriteManager::getShaderSpriteIndexOfSprite(cocos2d::CCSprite* sprite) {
    CCRect       rect    = sprite->getTextureRect();
    CCTexture2D* texture = sprite->getTexture();

    CCDictionary* cachedFrames = CCSpriteFrameCache::sharedSpriteFrameCache()->m_pSpriteFrames;
    CCSpriteFrame* frame = nullptr;

    for (auto [key, value] : CCDictionaryExt<std::string, CCSpriteFrame*>(cachedFrames)) {
        if (value->getTexture() == texture && value->getRect() == rect) {
            frame = value;
            break;
        }
    }

    if (frame == nullptr)
        return 0;

    auto it = shaderSpriteIndexPerSpriteFrame.find(frame);
    if (it == shaderSpriteIndexPerSpriteFrame.end())
        return 0;
    return it->second;
}

void ShaderSpriteManager::registerShaderSprite(const char* frameName, u32 shaderSpriteIndex) {
    CCSpriteFrame* frame = CCSpriteFrameCache::sharedSpriteFrameCache()->spriteFrameByName(frameName);
    if (frame == nullptr) return;
    shaderSpriteIndexPerSpriteFrame[frame] = shaderSpriteIndex;
}