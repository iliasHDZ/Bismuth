#include "ObjectSpriteUnpacker.hpp"
#include "Geode/cocos/cocoa/CCAffineTransform.h"

using namespace geode::prelude;

void ObjectSpriteUnpacker::unpackObject(GameObject* object) {
    CCSprite* colorSprite = object->m_colorSprite;

    bool shouldUnpackColorSprite = colorSprite && colorSprite->getParent() != object;
    bool isColorSpriteInFront    = object->m_colorZLayerRelated;

    auto transform = CCAffineTransformMakeIdentity();

    if (object->m_glowSprite)
        unpackSpriteRecursively(object, object->m_glowSprite, transform);

    if (shouldUnpackColorSprite && !isColorSpriteInFront)
        unpackSpriteRecursively(object, colorSprite, transform);

    unpackSpriteRecursively(object, object, transform);

    if (shouldUnpackColorSprite && isColorSpriteInFront)
        unpackSpriteRecursively(object, colorSprite, transform);
}

void ObjectSpriteUnpacker::unpackSpriteRecursively(
    GameObject* object,
    cocos2d::CCSprite* sprite,
    cocos2d::CCAffineTransform transform,
    SpriteType type
) {
    transform = CCAffineTransformConcat(sprite->nodeToParentTransform(), transform);

    if (sprite == object->m_colorSprite) type = SpriteType::DETAIL;
    if (sprite == object->m_glowSprite)  type = SpriteType::GLOW;

    CCArrayExt<CCSprite*> spriteChildren = sprite->getChildren();

    for (auto child : spriteChildren) {
        if (child->getZOrder() < 0)
            unpackSpriteRecursively(object, child, transform, type);
    }
    
    if (!sprite->getDontDraw())
        delegate.receiveUnpackedSprite(object, sprite, type, transform);

    for (auto child : spriteChildren) {
        if (child->getZOrder() >= 0)
            unpackSpriteRecursively(object, child, transform, type);
    }
}