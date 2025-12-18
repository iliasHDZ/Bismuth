#include <Geode/Geode.hpp>
#include "profiler.hpp"
#include "decomp/PlayLayer.hpp"
#include "decomp/GJBaseGameLayer.hpp"

#include <Geode/modify/GJBaseGameLayer.hpp>
class $modify(MyGJBaseGameLayer, GJBaseGameLayer) {
    void moveObjects(cocos2d::CCArray* objects, double moveX, double moveY, bool unused) {
        const char* name = "GJBaseGameLayer::moveObjects";
        profiler::functionPush(name);

        if (objects == nullptr) {
            profiler::functionPop(name);
            return;
        }

        cocos2d::CCObject* obj;
        CCARRAY_FOREACH(objects, obj) {
            GameObject* object = (GameObject*)obj;

            /*
            if (object->m_objectType == GameObjectType::Decoration)
                continue;
            */

            if (!object->m_isDecoration2 && object->m_unk4C4 != m_gameState.m_unkUint2) {
                object->m_unk4C4 = m_gameState.m_unkUint2;

                object->m_lastPosition.x = object->m_positionX;
                object->m_lastPosition.y = object->m_positionY;
                object->dirtifyObjectRect();
            }

            if (moveX != 0.0 && !object->m_tempOffsetXRelated)
                object->m_positionX += moveX;
            if (moveY != 0.0)
                object->m_positionY += moveY;
                
            object->dirtifyObjectPos();
            updateObjectSection(object);
        }

        updateDisabledObjectsLastPos(objects);
        profiler::functionPop(name);
    }

    void processMoveActions() {
        ((decomp_GJBaseGameLayer*)this)->processMoveActions();
    }
};

int i = 0;

cocos2d::CCLabelBMFont* thing = nullptr;

#include <Geode/modify/PlayLayer.hpp>
class $modify(MyPlayLayer, PlayLayer) {
    void updateVisibility(float dt) {
        i++;
        if (i < 100) {
            if (thing) thing->setString("DECOMP");
            ((decomp_PlayLayer*)this)->virtual_updateVisibility(dt);
        } else {
            if (thing) thing->setString("ORIGINAL");
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

        addChild(thing);
        return true;
    }

    void onQuit() {
        if (thing) {
            thing->removeFromParentAndCleanup(true);
            thing = nullptr;
        }
    }
};