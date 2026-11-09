#include "ObjectUtils.hpp"
#include "Geode/cocos/cocoa/CCAffineTransform.h"

using namespace geode::prelude;

const char* zLayerAsString(ZLayer zLayer) {
    switch (zLayer) {
    case ZLayer::B5:      return "b5";
    case ZLayer::B4:      return "b4";
    case ZLayer::B3:      return "b3";
    case ZLayer::B2:      return "b2";
    case ZLayer::B1:      return "b1";
    default:
    case ZLayer::Default: return "default";
    case ZLayer::T1:      return "t1";
    case ZLayer::T2:      return "t2";
    case ZLayer::T3:      return "t3";
    case ZLayer::T4:      return "t4";
    }
}

const char* spriteSheetAsString(SpriteSheet spriteSheet) {
    switch (spriteSheet) {
    case SpriteSheet::GLOW:     return "glow";
    case SpriteSheet::PARTICLE: return "particle";
    case SpriteSheet::PIXEL:    return "pixel";
    case SpriteSheet::GAME_2:   return "game2";
    case SpriteSheet::FIRE:     return "fire";
    case SpriteSheet::TEXT:     return "text";
    case SpriteSheet::SPECIAL:  return "special";
    default:
    case SpriteSheet::_UNK:     return "unk"; 
    case SpriteSheet::GAME_1:   return "game";
    }
}

const char* spriteTypeAsString(SpriteType type) {
    switch (type) {
    case SpriteType::BASE:        return "BASE";
    case SpriteType::DETAIL:      return "DETAIL";
    case SpriteType::BLACK:       return "BLACK";
    case SpriteType::GLOW:        return "GLOW";
    case SpriteType::BLACK_CHILD: return "BLACK_CHILD";
    default:
        return "<unknown>";
    }
}

u8 LayerKey::asNumber() const {
    u8 zlayerKey = (u8)zlayer;
    switch (zlayer) {
    case ZLayer::B5:      zlayerKey = 0; break;
    case ZLayer::B4:      zlayerKey = 1; break;
    case ZLayer::B3:      zlayerKey = 2; break;
    case ZLayer::B2:      zlayerKey = 3; break;
    case ZLayer::B1:      zlayerKey = 4; break;
    default:
    case ZLayer::Default: zlayerKey = 5; break;
    case ZLayer::T1:      zlayerKey = 6; break;
    case ZLayer::T2:      zlayerKey = 7; break;
    case ZLayer::T3:      zlayerKey = 8; break;
    case ZLayer::T4:      zlayerKey = 9; break;
    }

    u8 spriteSheetKey = 0;
    switch (spriteSheet) {
    case SpriteSheet::GLOW:     spriteSheetKey = 0; break;
    case SpriteSheet::PARTICLE: spriteSheetKey = 1; break;
    case SpriteSheet::PIXEL:    spriteSheetKey = 2; break;
    case SpriteSheet::GAME_2:   spriteSheetKey = 3; break;
    case SpriteSheet::FIRE:     spriteSheetKey = 4; break;
    case SpriteSheet::TEXT:     spriteSheetKey = 5; break;
    case SpriteSheet::SPECIAL:  spriteSheetKey = 6; break;
    default:
    case SpriteSheet::_UNK:
    case SpriteSheet::GAME_1:   spriteSheetKey = 7; break;
    }
    
    return (zlayerKey << 4) | (spriteSheetKey << 1) | (blending ? 1 : 0);
}

LayerKey LayerKey::fromNumber(u8 num) {
    u8 zlayerKey = num >> 4;
    u8 spriteSheetKey = (num >> 1) & 0x7;
    bool blending = num & 1;

    ZLayer zlayer;
    switch (zlayerKey) {
    case 0: zlayer = ZLayer::B5; break;
    case 1: zlayer = ZLayer::B4; break;
    case 2: zlayer = ZLayer::B3; break;
    case 3: zlayer = ZLayer::B2; break;
    case 4: zlayer = ZLayer::B1; break;
    default:
    case 5: zlayer = ZLayer::Default; break;
    case 6: zlayer = ZLayer::T1; break;
    case 7: zlayer = ZLayer::T2; break;
    case 8: zlayer = ZLayer::T3; break;
    case 9: zlayer = ZLayer::T4; break;
    }

    SpriteSheet spriteSheet;
    switch (spriteSheetKey) {
    case 0: spriteSheet = SpriteSheet::GLOW; break;
    case 1: spriteSheet = SpriteSheet::PARTICLE; break;
    case 2: spriteSheet = SpriteSheet::PIXEL; break;
    case 3: spriteSheet = SpriteSheet::GAME_2; break;
    case 4: spriteSheet = SpriteSheet::FIRE; break;
    case 5: spriteSheet = SpriteSheet::TEXT; break;
    case 6: spriteSheet = SpriteSheet::SPECIAL; break;
    default:
    case 7: spriteSheet = SpriteSheet::GAME_1; break;
    }

    return { zlayer, spriteSheet, blending };
}

void ObjectUtils::unpackObjectIntoSprites(GameObject* object, ReceiveUnpackedSpriteFunc func) {
    CCSprite* colorSprite = object->m_colorSprite;

    bool shouldUnpackColorSprite = colorSprite && colorSprite->getParent() != object;
    bool isColorSpriteInFront    = object->m_colorZLayerRelated;

    auto transform = CCAffineTransformMakeIdentity();

    if (object->m_glowSprite)
        unpackSpriteRecursively(object, object->m_glowSprite, transform, func);

    if (shouldUnpackColorSprite && !isColorSpriteInFront)
        unpackSpriteRecursively(object, colorSprite, transform, func);

    unpackSpriteRecursively(object, object, transform, func);

    if (shouldUnpackColorSprite && isColorSpriteInFront)
        unpackSpriteRecursively(object, colorSprite, transform, func);
}

void ObjectUtils::unpackSpriteRecursively(
    GameObject* object,
    cocos2d::CCSprite* sprite,
    cocos2d::CCAffineTransform transform,
    ReceiveUnpackedSpriteFunc func,
    SpriteType type
) {
    transform = CCAffineTransformConcat(sprite->nodeToParentTransform(), transform);

    bool isSpriteBlack = false;

    if (sprite == object->m_colorSprite) type = SpriteType::DETAIL;
    if (sprite == object->m_glowSprite)  type = SpriteType::GLOW;

    auto parentType = type;

    if (sprite == object) {
        if (object->m_isObjectBlack)
            parentType = SpriteType::BLACK;
    } else if (type == SpriteType::BASE && object->m_isColorSpriteBlack && sprite->getTag() != 997)
        parentType = SpriteType::BLACK_CHILD;

    CCArrayExt<CCSprite*> spriteChildren = sprite->getChildren();

    for (auto child : spriteChildren) {
        if (child->getZOrder() < 0)
            unpackSpriteRecursively(object, child, transform, func, type);
    }
    
    if (!sprite->getDontDraw())
        func({sprite, object, parentType, transform});

    for (auto child : spriteChildren) {
        if (child->getZOrder() >= 0)
            unpackSpriteRecursively(object, child, transform, func, type);
    }
}