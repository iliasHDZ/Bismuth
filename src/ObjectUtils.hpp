#pragma once

#include "Geode/cocos/cocoa/CCAffineTransform.h"
#include "Geode/cocos/sprite_nodes/CCSprite.h"
#include <Geode/binding/GameObject.hpp>
#include <common.hpp>
#include <functional>
#include "renderer/SAOBuffer.hpp"

enum class SpriteType {
    BASE        = 0,
    DETAIL      = 1,
    BLACK       = 2,
    GLOW        = 3,
    BLACK_CHILD = 4
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

#define LAYER_KEY_BLENDING 0x80

const char* zLayerAsString(ZLayer zLayer);

const char* spriteSheetAsString(SpriteSheet spriteSheet);

const char* spriteTypeAsString(SpriteType type);

template <>
class fmt::formatter<ZLayer> {
public:
    constexpr auto parse (format_parse_context& ctx) { return ctx.begin(); }
    template <typename Context>
    constexpr auto format (const ZLayer& value, Context& ctx) const {
        return format_to(ctx.out(), "{}", zLayerAsString(value));
    }
};

template <>
class fmt::formatter<SpriteSheet> {
public:
    constexpr auto parse (format_parse_context& ctx) { return ctx.begin(); }
    template <typename Context>
    constexpr auto format (const SpriteSheet& value, Context& ctx) const {
        return format_to(ctx.out(), "{}", spriteSheetAsString(value));
    }
};

template <>
class fmt::formatter<SpriteType> {
public:
    constexpr auto parse (format_parse_context& ctx) { return ctx.begin(); }
    template <typename Context>
    constexpr auto format (const SpriteType& value, Context& ctx) const {
        return format_to(ctx.out(), "{}", spriteTypeAsString(value));
    }
};

struct LayerKey {
    ZLayer zlayer;
    SpriteSheet spriteSheet;
    bool blending;

    auto operator<=>(const LayerKey&) const = default;

    u8 asNumber() const;

    static LayerKey fromNumber(u8 num);

    inline std::string toString() const {
        return fmt::format("({}, {}, {})", zLayerAsString(zlayer), spriteSheetAsString(spriteSheet), blending);
    }

    inline std::string asID() const {
        return fmt::format("bismuth-{}-{}", zLayerAsString(zlayer), spriteSheetAsString(spriteSheet)) + (blending ? "-blending" : "");
    }

    static inline ZLayer getNormalObjectZLayer(GameObject* object) {
        ZLayer zLayer = object->getObjectZLayer();
        if ((i32)zLayer % 2 == 0) {
            zLayer = (ZLayer)((i32)zLayer - 1);
            if (zLayer < ZLayer::B5)
                zLayer = ZLayer::B5;
        }
        return zLayer;
    }

    static inline LayerKey getFromObject(GameObject* object, bool blending) {
        return { getNormalObjectZLayer(object), (SpriteSheet)object->getParentMode(), blending };
    }

    static inline LayerKey getFromGlowSpriteObject(GameObject* object) {
        return { getNormalObjectZLayer(object), SpriteSheet::GLOW, true };
    }
};

template <>
class fmt::formatter<LayerKey> {
public:
    constexpr auto parse (format_parse_context& ctx) { return ctx.begin(); }
    template <typename Context>
    constexpr auto format (const LayerKey& value, Context& ctx) const {
        return format_to(ctx.out(), "({}, {}, {})", value.zlayer, value.spriteSheet, value.blending);
    }
};

template <>
struct std::hash<LayerKey> {
    std::size_t operator()(const LayerKey& u) const noexcept {
        return u.asNumber();
    }
};

struct UnpackedSprite {
    cocos2d::CCSprite* sprite;
    GameObject* parentObject;
    SpriteType type;
    const cocos2d::CCAffineTransform& transform;
};

using ReceiveUnpackedSpriteFunc = std::function<void(const UnpackedSprite&)>;

class ObjectUtils {
public:
    static inline SpriteSheet getSpritesheetOfObject(GameObject* object, SpriteType type) {
        return type == SpriteType::GLOW ? SpriteSheet::GLOW : (SpriteSheet)object->getParentMode();
    }

    static GEODE_INLINE u32 getSpriteColorChannel(SpriteType type, GameObject* object, cocos2d::CCSprite* sprite) {
        u32 colorChannel = type == SpriteType::DETAIL ? object->m_activeDetailColorID : object->m_activeMainColorID;

        /*
        bool isSpriteBlack = (sprite == object) ? object->m_isObjectBlack : object->m_isColorSpriteBlack;
        if (isSpriteBlack)
            return COLOR_CHANNEL_BLACK;
        */
        if (type == SpriteType::GLOW && object->m_glowColorIsLBG)
            return COLOR_CHANNEL_LBG;
        return colorChannel;
    }

    /*
        This function unpacks GameObject and turns them into
        individual sprites and calls func for every sprite
        in draw order.
    */
    static void unpackObjectIntoSprites(GameObject* object, ReceiveUnpackedSpriteFunc func);

private:
    static void unpackSpriteRecursively(
        GameObject* object,
        cocos2d::CCSprite* sprite,
        cocos2d::CCAffineTransform transform,
        ReceiveUnpackedSpriteFunc func,
        SpriteType type = SpriteType::BASE
    );
};