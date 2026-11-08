#include "Geode/cocos/CCDirector.h"
#include "Renderer.hpp"
#include <decomp/PlayLayer.hpp>
#include <decomp/GJBaseGameLayer.hpp>
#include "../cl.hpp"
#include "OverdrawView.hpp"

using namespace geode::prelude;

static bool newPlayLayer = false;

#include <Geode/modify/PlayLayer.hpp>
class $modify(MyPlayLayer, PlayLayer) {
    void setupHasCompleted() {
        newPlayLayer = true;
        PlayLayer::setupHasCompleted();
        newPlayLayer = false;
    }

    bool init(GJGameLevel *level, bool useReplay, bool dontCreateObjects) {
        bool ret = PlayLayer::init(level, useReplay, dontCreateObjects);
        if (ret) {
            auto view = OverdrawView::create();
            if (view) {
                m_objectLayer->addChild(view->getPreDrawNode(), -10000);
                m_objectLayer->addChild(view, 10000);
                addChild(view->getText(), 10000);
            }
        }
        return ret;
    }

    // This gets called from inside PlayLayer::setupHasCompleted()
    void resetLevel() {
        if (newPlayLayer) {
            auto batchLayer = this->m_objectLayer;
            if (!batchLayer) {
                log::error("failed to attach renderer: batch layer not found");
                return;
            }

            auto renderer = Renderer::create(this, m_objects);
            if (renderer)
                batchLayer->addChild(renderer, -100000);
        }

        auto renderer = Renderer::get();
        if (renderer)
            renderer->reset();

        PlayLayer::resetLevel();
    }

    void updateVisibility(float dt) {
        auto renderer = Renderer::get();
        if (renderer && renderer->useOptimizations()) {
            ((decomp_PlayLayer*)this)->optimized_updateVisibility(dt);
            return;
        }

        PlayLayer::updateVisibility(dt);
    }

    CheckpointObject* createCheckpoint() {
        CheckpointObject* ret = PlayLayer::createCheckpoint();

        auto renderer = Renderer::get();
        if (renderer)
            renderer->getGroupManager().storeCheckpoint(ret);
        
        return ret;
    }

    void loadFromCheckpoint(CheckpointObject* object) {
        PlayLayer::loadFromCheckpoint(object);

        auto renderer = Renderer::get();
        if (renderer)
            renderer->getGroupManager().restoreCheckpoint(object);
    }

    void checkForEnd() {
        auto renderer = Renderer::get();
        if (renderer && renderer->useOptimizations() && m_endPortal)
            m_endPortal->updateEndPos(false);

        PlayLayer::checkForEnd();
    }
};

#include <Geode/modify/CheckpointObject.hpp>
class $modify(MyCheckpointObject, CheckpointObject) {
    struct Fields {
        CheckpointObject* checkpoint;
        ~Fields() {
            auto renderer = Renderer::get();
            if (renderer)
                renderer->getGroupManager().removeCheckpoint(checkpoint);
        }
    };

    bool init() {
        if (!CheckpointObject::init()) return false;
        m_fields.self()->checkpoint = this;
        return true;
    }
};

static void removeDecoObjects(CCArray* array) {
    for (u32 i = 0; i < array->count();) {
        auto object = (GameObject*)array->objectAtIndex(i);
        if (object->m_objectType == GameObjectType::Decoration)
            array->removeObjectAtIndex(i);
        else
            i++;
    }
}

#include <Geode/modify/GJBaseGameLayer.hpp>
class $modify(RendererGJBaseGameLayer, GJBaseGameLayer) {
    void update(float dt) {
        GJBaseGameLayer::update(dt);

        auto renderer = Renderer::get();
        if (renderer)
            renderer->update(dt);
    }

    void processMoveActions() {
        auto renderer = Renderer::get();
        if (renderer == nullptr) {
            GJBaseGameLayer::processMoveActions();
            return;
        }

        m_effectManager->processMoveCalculations();
        for (auto node : m_effectManager->m_unkVector6c0) {
            if (node->m_unk0d0)
                continue;

            int groupId = node->getTag();

            renderer->getGroupManager().moveGroup(groupId, node->m_unk038, node->m_unk040);

            CCArray* objects = getStaticGroup(groupId);
            if (objects)
                moveObjects(objects, node->m_unk038, node->m_unk040, 0);

            if (!renderer->useOptimizations()) {
                objects = getOptimizedGroup(groupId);
                if (objects)
                    moveObjects(objects, node->m_unk090, node->m_unk098, 0);
            }
        }
    }

    void siphon_processRotationActions() {
        auto renderer = Renderer::get();
        if (renderer == nullptr) {
            GJBaseGameLayer::processRotationActions();
            return;
        }

        auto eman = m_effectManager;
        for (auto cmdObj : eman->m_unkVector5b0) {
            if (/* cmdObj->m_unkInt204 != m_gameState.m_unkUint2 ||*/ cmdObj->m_someInterpValue1RelatedFalse)
                continue;

            i32 targetId = cmdObj->m_targetGroupID;
            i32 centerId = cmdObj->m_centerGroupID;

            auto centerObject = tryGetMainObject(centerId);
            auto staticGroup = getStaticGroup(targetId);

            float rotation;
            if (staticGroup->count() != 0)
                rotation = cmdObj->m_someInterpValue1RelatedOne - cmdObj->m_someInterpValue1RelatedZero;
            else
                rotation = cmdObj->m_someInterpValue2RelatedOne - cmdObj->m_someInterpValue2RelatedZero;

            if (rotation == 0.0 && !cmdObj->m_finishRelated)
                continue;

            if (centerObject == nullptr)
                renderer->getGroupManager().rotateGroup(targetId, rotation, cmdObj->m_lockObjectRotation);
            else {
                auto pos = centerObject->getUnmodifiedPosition();
                renderer->getGroupManager().rotateGroup(targetId, rotation, cmdObj->m_lockObjectRotation, ccPointToGLM(pos));

                auto it = m_effectManager->m_unkMap618.find(targetId);
                CCPoint extraMove = {0, 0};

                if (it != m_effectManager->m_unkMap618.end()) {
                    if (staticGroup->count() != 0)
                        extraMove = ccp(it->second->m_unk090, it->second->m_unk098);
                    else
                        extraMove = ccp(it->second->m_unk038, it->second->m_unk040);
                }

                if (extraMove != ccp(0, 0))
                    renderer->getGroupManager().moveGroup(targetId, extraMove.x, extraMove.y);
            }
        }

        ((decomp_GJBaseGameLayer*)this)->processRotationActions();
    }

    void processRotationActions() {
        auto renderer = Renderer::get();
        if (renderer == nullptr) {
            GJBaseGameLayer::processRotationActions();
            return;
        }

        if (renderer->useOptimizations())
            ((decomp_GJBaseGameLayer*)this)->optimized_processRotationActions();
        else
            siphon_processRotationActions();
    }

    void siphon_processFollowActions() {
        auto renderer = Renderer::get();

        for (auto node : m_effectManager->m_unkVector6d8) {
            i32 targetGroupId = node->getTag();
            i32 followGroupId = node->m_unk074;

            auto mainObject = tryGetMainObject(followGroupId);
            if (mainObject == nullptr)
                continue;

            double moveX = 0.0, moveY = 0.0;
            if (mainObject->m_unk4C4 == m_gameState.m_commandIndex) {
                moveX = (mainObject->m_positionX - mainObject->m_lastPosition.x) * node->m_unk080; /* followXMod */
                moveY = (mainObject->m_positionY - mainObject->m_lastPosition.y) * node->m_unk088; /* followYMod */
            }

            renderer->getGroupManager().moveGroup(targetGroupId, moveX, moveY);
        }

        GJBaseGameLayer::processFollowActions();
    }

    void processFollowActions() {
        auto renderer = Renderer::get();
        if (!renderer) {
            GJBaseGameLayer::processFollowActions();
            return;
        }

        if (renderer->useOptimizations())
            ((decomp_GJBaseGameLayer*)this)->optimized_processFollowActions();
        else
            siphon_processFollowActions();
    }

    void processPlayerFollowActions(float dt) {
        ((decomp_GJBaseGameLayer*)this)->processPlayerFollowActions(dt);
    }

    void toggleGroup(int id, bool activate) {
        GJBaseGameLayer::toggleGroup(id, activate);
        auto renderer = Renderer::get();
        if (renderer)
            renderer->getGroupManager().toggleGroup(id, activate);
    }

    void optimizeMoveGroups() {
        GJBaseGameLayer::optimizeMoveGroups();

        auto renderer = Renderer::get();
        if (!renderer || !renderer->useOptimizations())
            return;

        for (auto array : m_optimizedGroups) {
            if (array)
                removeDecoObjects(array);
        }
        for (auto array : m_staticGroups) {
            if (array)
                removeDecoObjects(array);
        }
    }
};

#include <Geode/modify/CCDisplayLinkDirector.hpp>
class $modify(RendererCCDisplayLinkDirector, CCDisplayLinkDirector) {
    void mainLoop() {
        auto ren = Renderer::get();
        if (!ren) {
            CCDisplayLinkDirector::mainLoop();
            return;
        }

        CCDisplayLinkDirector::mainLoop();
    }
};

class OnMenuLoad;

OnMenuLoad* menuLoad = nullptr;
static bool hasAlreadyDoneMenuLoad = false;

class OnMenuLoad : public CCNode {
public:
    void showLoadAlerts() {
        if (!isOpenGLAtLeastVersion(4, 3)) {
            std::string version = (const char*)glGetString(GL_VERSION);
            std::transform(version.begin(), version.end(), version.begin(), [](char ch) { return std::tolower(ch); });

            if (version.find("reviangle") != std::string::npos) {
                FLAlertLayer::create(
                    "Bismuth error!",
                    "You are using ReviANGLE which does not supply the required OpenGL version 4.3\n"
                    "Disable ReviANGLE and try again.",
                    "Ok"
                )->show();
                return;
            }

            FLAlertLayer::create(
                "Bismuth error!",
                "Your GPU does not support the required OpenGL 4.3!\n"
                "Make sure your GPU drivers are updated.",
                "Ok"
            )->show();
            return;
        }

        if (getGPUCoreCount() == -1) {
            FLAlertLayer::create(
                "No core count!",
                "Bismuth could not find the core count of your GPU!\n"
                "This is likely due to your GPU being AMD which is a hassle to get the core count of.\n"
                "Make sure you search up your *GPU* core count, go to Bismuth settings, and set it there.\n",
                "Ok"
            )->show();
        }
    }

    void onLoad() {
        showLoadAlerts();
        
        this->release();
        menuLoad = nullptr;
    }
};

#include <Geode/modify/LoadingLayer.hpp>
class $modify(MyLoadingLayer, LoadingLayer) {
    void loadAssets() {
        LoadingLayer::loadAssets();
        if (m_loadStep == 14 && !m_fromRefresh && !hasAlreadyDoneMenuLoad) {
            hasAlreadyDoneMenuLoad = true;
            menuLoad = new OnMenuLoad;
            menuLoad->retain();
            CCDirector::get()->getActionManager()->addAction(
                CCSequence::create(
                    CCDelayTime::create(0.1f),
                    CCCallFunc::create(menuLoad, callfunc_selector(OnMenuLoad::onLoad)),
                    NULL
                ),
                menuLoad,
                false
            );
        }
    }
};

#include <Geode/modify/LevelEditorLayer.hpp>
class $modify(MyLevelEditorLayer, LevelEditorLayer) {
    bool init(GJGameLevel* level, bool noUI) {
        bool ret = LevelEditorLayer::init(level, noUI);
        if (ret) {
            auto view = OverdrawView::create();
            if (view) {
                m_objectLayer->addChild(view->getPreDrawNode(), -10000);
                m_objectLayer->addChild(view, 10000);

                m_editorUI->addChild(view->getText(), 10000);
                view->getText()->setPosition({44, 116});
            }
        }
        return ret;
    }
};