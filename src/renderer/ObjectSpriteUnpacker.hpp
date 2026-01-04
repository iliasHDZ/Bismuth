#pragma once

#include "Geode/cocos/cocoa/CCAffineTransform.h"
#include <Geode/binding/GameObject.hpp>
#include <common.hpp>

enum class SpriteType {
    BASE,
    DETAIL,
    BLACK,
    GLOW
};

enum class SpriteSheet {
    GAME_1,
    GAME_2,
    TEXT,
    FIRE,
    SPECIAL,
    GLOW,
    PIXEL,
    _UNK,
    PARTICLE,

    COUNT
};

class ObjectSpriteUnpackerDelegate {
public:
    // The sprites are recieved in draw order
    virtual void recieveUnpackedSprite(
        GameObject* parentObject,
        cocos2d::CCSprite* sprite,
        SpriteType type,
        const cocos2d::CCAffineTransform& transform
    ) = 0;
};

class ObjectSpriteUnpacker {
public:
    inline ObjectSpriteUnpacker(ObjectSpriteUnpackerDelegate& delegate)
        : delegate(delegate) {}

    void unpackObject(GameObject* object);

    inline SpriteSheet getSpritesheetOfObject(GameObject* object, SpriteType type) {
        return type == SpriteType::GLOW ? SpriteSheet::GLOW : (SpriteSheet)object->getParentMode();
    }

private:
    void unpackSpriteRecursively(
        GameObject* object,
        cocos2d::CCSprite* sprite,
        cocos2d::CCAffineTransform transform,
        SpriteType type = SpriteType::BASE
    );

private:
    ObjectSpriteUnpackerDelegate& delegate;
};