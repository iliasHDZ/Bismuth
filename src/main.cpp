#include <Geode/Geode.hpp>
#include <Geode/binding/PlayLayer.hpp>
#include "profiler.hpp"
#include "decomp/PlayLayer.hpp"
#include "decomp/GJBaseGameLayer.hpp"

using namespace geode::prelude;

#include <Geode/modify/GJBaseGameLayer.hpp>
class $modify(MyGJBaseGameLayer, GJBaseGameLayer) {
    /*
    void processMoveActions() {
        ((decomp_GJBaseGameLayer*)this)->processMoveActions();
    }
    */
};

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

/*
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