#include <Geode/Geode.hpp>
#include <Geode/binding/PlayLayer.hpp>
#include "profiler.hpp"
#include "ObjectUtils.hpp"
#include "HotKey.hpp"

using namespace geode::prelude;

#include <Geode/modify/GJBaseGameLayer.hpp>
class $modify(MyGJBaseGameLayer, GJBaseGameLayer) {
    /*
    void processMoveActions() {
        ((decomp_GJBaseGameLayer*)this)->processMoveActions();
    }
    */
};

/*

DECLARE_HOTKEY(enumKeyCodes::KEY_A, {
    auto sfn = CCSpriteFrameCache::sharedSpriteFrameCache()->spriteFrameByName("d_wheel_01_001.png");

    log::info("offset: {}", sfn->getOffsetInPixels());
    log::info("originalSize: {}", sfn->getOriginalSizeInPixels());
    log::info("rect: {}", sfn->getRectInPixels());
    log::info("contentScaleFactor: {}", CCDirector::sharedDirector()->getContentScaleFactor());
});

DECLARE_HOTKEY(enumKeyCodes::KEY_B, {
    const char* frameName = ObjectToolbox::sharedState()->intKeyToFrame(471);
    if (!frameName)
        return;

    GameObject* object = GameObject::createWithKey(471);

    object->customSetup();
    object->addGlow(frameName);
    object->addColorSprite(frameName);
    object->setupCustomSprites(frameName);

    log::info("UnflippedOffsetPositionFromCenter: {}", object->getUnflippedOffsetPosition());
    log::info("ContentSize: {}", object->getContentSize());
    log::info("Rect: {}", CCSpriteFrameCache::get()->spriteFrameByName(frameName)->getRect());

    for (auto child : CCArrayExt<CCSprite*>(object->getChildren())) {
        log::info("- CHILD");
        log::info("- position: {}", child->getPosition());
        log::info("- scale: {}", child->getScale());
        log::info("- rotation: {}", child->getRotation());
        
        std::string frameName = "";

        auto* cachedFrames = CCSpriteFrameCache::sharedSpriteFrameCache()->m_pSpriteFrames;
        const auto rect = child->getTextureRect();
        auto texture = child->getTexture();

        for (auto [key, frame] : CCDictionaryExt<std::string, CCSpriteFrame*>(cachedFrames)) {
            if (frame->getTexture() == texture && frame->getRect() == rect) {
                frameName = key;
                break;
            }
        }

        log::info("- frameName: {}", frameName);
    }
})
*/

/*
inline float qround(float a) {
    return roundf(a * 10000) / 10000;
}

DECLARE_HOTKEY(enumKeyCodes::KEY_M, {
    matjson::Value objectsJson = matjson::makeObject({});

    for (int i = 1; i < 4540; i++) {
        const char* frameName = ObjectToolbox::sharedState()->intKeyToFrame(i);
        if (!frameName)
            continue;

        GameObject* object = GameObject::createWithKey(i);

        if (!object || i == 1615)
            continue;

        if (i != 1329)
            object->customSetup();
        object->addGlow(frameName);
        object->addColorSprite(frameName);
        object->setupCustomSprites(frameName);

        matjson::Value objectJson = matjson::makeObject({});

        objectJson["defaultZOrder"] = object->m_defaultZOrder;
        objectJson["defaultZLayer"] = (int)object->m_defaultZLayer;
        objectJson["parentMode"] = object->getParentMode();

        if (object->m_baseColor && object->m_baseColor->m_defaultColorID != 0)
            objectJson["defaultBaseColorChannel"] = object->m_baseColor->m_defaultColorID;
        if (object->m_detailColor && object->m_detailColor->m_defaultColorID != 0)
            objectJson["defaultDetailColorChannel"] = object->m_detailColor->m_defaultColorID;

        const char* spriteSheet = "";

        switch ((SpriteSheet)object->getParentMode()) {
        case SpriteSheet::GAME_1:   spriteSheet = "game1"; break;
        case SpriteSheet::GAME_2:   spriteSheet = "game2"; break;
        case SpriteSheet::TEXT:     spriteSheet = "text"; break;
        case SpriteSheet::FIRE:     spriteSheet = "fire"; break;
        case SpriteSheet::SPECIAL:  spriteSheet = "special"; break;
        case SpriteSheet::GLOW:     spriteSheet = "glow"; break;
        case SpriteSheet::PIXEL:    spriteSheet = "pixel"; break;
        case SpriteSheet::_UNK:     spriteSheet = "unknown"; break;
        case SpriteSheet::PARTICLE: spriteSheet = "particle"; break;
        default:
        }

        objectJson["spriteSheet"] = spriteSheet;

        std::vector<matjson::Value> spritesJson;

        object->setPosition({0, 0});

        ObjectUtils::unpackObjectIntoSprites(object, [&](const UnpackedSprite& sprite) {
            const CCAffineTransform& tr = sprite.transform;

            glm::vec2 unitX = { tr.a, tr.b };
            glm::vec2 unitY = { tr.c, tr.d };
            
            float scaleX = glm::length(unitX);
            float scaleY = glm::length(unitY);

            if (glm::dot(glm::vec2(-unitX.y, unitX.x), unitY) < 0.0)
                scaleX *= -1;

            float rotation = glm::degrees(atan2f(unitX.y / scaleX, unitX.x / scaleX));

            const char* colorType = "";
            switch (sprite.type) {
            case SpriteType::BASE: colorType = "base"; break;
            case SpriteType::DETAIL: colorType = "detail"; break;
            case SpriteType::BLACK: colorType = "black"; break;
            case SpriteType::GLOW: colorType = "glow"; break;
            case SpriteType::BLACK_CHILD: colorType = "blackChild"; break;
            }

            std::string frameName = "";

            auto* cachedFrames = CCSpriteFrameCache::sharedSpriteFrameCache()->m_pSpriteFrames;
            const auto rect = sprite.sprite->getTextureRect();
            auto texture = sprite.sprite->getTexture();

            for (auto [key, frame] : CCDictionaryExt<std::string, CCSpriteFrame*>(cachedFrames)) {
                if (frame->getTexture() == texture && frame->getRect() == rect) {
                    frameName = key;
                    break;
                }
            }

            auto spriteOffset = sprite.sprite->getUnflippedOffsetPosition();

            spritesJson.push_back(matjson::makeObject({
                {"texture", frameName},
                {"colorType", colorType},
                {"position", std::vector<float>({qround(tr.tx), qround(tr.ty)})},
                {"scale", std::vector<float>({qround(scaleX), qround(scaleY)})},
                {"contentSize", std::vector<float>({qround(sprite.sprite->getContentWidth()), qround(sprite.sprite->getContentHeight())})},
                {"spriteOffset", std::vector<float>({qround(spriteOffset.x), qround(spriteOffset.y)})},
                {"flipX", sprite.sprite->isFlipX()},
                {"flipY", sprite.sprite->isFlipY()},
                {"rotation", rotation}
            }));
        });

        objectJson["sprites"] = spritesJson;
        objectsJson[std::to_string(i)] = objectJson;

        object->release();
    }

    auto result = file::writeToJson("./object.json", objectsJson);
})
*/

/*
static bool decompEnabled = false;

#include <Geode/modify/PlayLayer.hpp>
class $modify(MyPlayLayer, PlayLayer) {
    void updateVisibility(float dt) {
        if (decompEnabled)
            ((decomp_PlayLayer*)this)->virtual_updateVisibility(dt);
        else
            PlayLayer::updateVisibility(dt);
    }
};

#include <Geode/modify/CCKeyboardDispatcher.hpp>
class $modify(OCCKeyboardDispatcher, cocos2d::CCKeyboardDispatcher) {
    bool dispatchKeyboardMSG(cocos2d::enumKeyCodes key, bool keyDown, bool isKeyRepeat) {
        if (keyDown && key == cocos2d::KEY_F1 && !isKeyRepeat) {
            decompEnabled = !decompEnabled;
            if (decompEnabled)
                log::info("DECOMP ENABLED");
            else
                log::info("DECOMP DISABLED");
        }

        return cocos2d::CCKeyboardDispatcher::dispatchKeyboardMSG(key, keyDown, isKeyRepeat);
    }
};
static int* a = nullptr;

#include <Geode/modify/GameObject.hpp>
class $modify(MyGameObject, GameObject) {
    void setOpacity(unsigned char op) {
        if (m_objectID == 669)
            a = 0;

        GameObject::setOpacity(op);
    }
};

int* a = nullptr;
PlayLayer* pl = nullptr;

#include <Geode/modify/GameObject.hpp>
class $modify(MyGameObject, GameObject) {
    void activateObject() {
        GameObject::activateObject();

        for (int i = 0; i < pl->m_activeObjectsCount; i++)
            if (pl->m_activeObjects[i] == this)
                return;
        
        *a = 6;
    }
};

#include <Geode/modify/PlayLayer.hpp>
class $modify(MyPlayLayer, PlayLayer) {
    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        pl = this;

        if (!PlayLayer::init(level, useReplay, dontCreateObjects))
            return false;

        return true;
    }
};

int i = 0;

cocos2d::CCLabelBMFont* thing = nullptr;

PlayLayer* pl = nullptr;

#include <Geode/modify/PlayLayer.hpp>
class $modify(MyPlayLayer, PlayLayer) {
    void updateVisibility(float dt) {
        i++;
        if (i < 100) {
            // if (thing) thing->setString("DECOMP");
            ((decomp_PlayLayer*)this)->virtual_updateVisibility(dt);
        } else {
            // if (thing) thing->setString("ORIGINAL");
            PlayLayer::updateVisibility(dt);
            if (i >= 200)
                i = 0;
        }
    }

    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects))
            return false;
        
        thing = cocos2d::CCLabelBMFont::create("", "bigFont.fnt");
        thing->setAnchorPoint(cocos2d::CCPoint(0, 1));
        thing->setPosition(cocos2d::CCPoint(5, cocos2d::CCDirector::get()->getWinSize().height));

        pl = this;

        // addChild(thing);
        return true;
    }

    void onQuit() {
        if (thing) {
            thing->removeFromParentAndCleanup(true);
            thing = nullptr;
        }
        PlayLayer::onQuit();
    }
};
*/

/*
#include <Geode/modify/CCKeyboardDispatcher.hpp>
class $modify(FnCCKeyboardDispatcher, cocos2d::CCKeyboardDispatcher) {
    bool dispatchKeyboardMSG(cocos2d::enumKeyCodes key, bool keyDown, bool p3) {
        if (keyDown && key == cocos2d::KEY_N && pl) {
            CCObject* o;
            CCARRAY_FOREACH(pl->m_objects, o) {
                GameObject* obj = (GameObject*)o;

                geode::log::info("OBJECT {}, {}", obj->m_objectID, obj->getAtlasIndex());

                CCObject* o2;
                CCARRAY_FOREACH(obj->getChildren(), o2) {
                    cocos2d::CCSprite* sprite = (cocos2d::CCSprite*)o2;

                    geode::log::info("SUBOBJECT {}", sprite->getAtlasIndex());
                }
            }
        }

        return cocos2d::CCKeyboardDispatcher::dispatchKeyboardMSG(key, keyDown, p3);
    }
};
*/