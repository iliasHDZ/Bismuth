#include "GJBaseGameLayer.hpp"
#include "../renderer/Renderer.hpp"
#include "../profiler.hpp"

using namespace geode::prelude;

void decomp_GJBaseGameLayer::moveObjects(cocos2d::CCArray* objects, double moveX, double moveY, bool unused) {
    if (objects == nullptr)
        return;

    for (auto object : CCArrayExt<GameObject*>(objects)) {
        if (!object->m_isDecoration2 && object->m_unk4C4 != m_gameState.m_commandIndex) {
            object->m_unk4C4 = m_gameState.m_commandIndex;

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
}

void decomp_GJBaseGameLayer::processMoveActions() {
    m_effectManager->processMoveCalculations();
    for (auto node : m_effectManager->m_unkVector6c0) {
        if (node->m_unk0d0)
            continue;

        int groupId = node->getTag();
        CCArray* objects = getStaticGroup(groupId);
        if (objects)
            moveObjects(objects, node->m_unk038, node->m_unk040, node->m_uIndexInArray == 13); // index in array thing is unused

        objects = getOptimizedGroup(groupId);
        if (objects)
            moveObjects(objects, node->m_unk090, node->m_unk098, node->m_uIndexInArray == 13); // index in array thing is unused
    }
}

void decomp_GJBaseGameLayer::processRotationActions() {
    float objLayerScaleX = m_objectLayer->getScaleX();
    float objLayerScaleY = m_objectLayer->getScaleY();
    CCPoint objLayerPos = m_objectLayer->getPosition();

    m_objectLayer->setScale(1.0f);
    m_objectLayer->setPosition(CCPointZero);

    for (auto& [_, gcos] : m_effectManager->m_unkMap5c8) {
        for (auto gco : gcos)
            gco->m_unkInt204 = m_gameState.m_commandIndex;
    }

    for (auto gco : m_effectManager->m_unkVector5b0) {
        if (gco->m_unkInt204 != m_gameState.m_commandIndex)
            continue;

        if (gco->m_someInterpValue1RelatedFalse)
            continue;

        GameObject* mainObject = tryGetMainObject(gco->m_centerGroupID);
        CCArray* staticGroup = getStaticGroup(gco->m_targetGroupID);

        CCArray* objectsToRotate = getOptimizedGroup(gco->m_targetGroupID);

        float rotation;
        bool ignoreStaticGroups;

        if (staticGroup->count() != 0) {
            objectsToRotate = getGroup(gco->m_targetGroupID);
            rotation = gco->m_someInterpValue1RelatedOne - gco->m_someInterpValue1RelatedZero;
            ignoreStaticGroups = false;
        } else {
            rotation = gco->m_someInterpValue2RelatedOne - gco->m_someInterpValue2RelatedZero;
            ignoreStaticGroups = true;
        }

        if (rotation == 0.0f && !gco->m_finishRelated)
            continue;

        gco->m_someInterpValue1RelatedFalse = true;

        float localRotation = rotation;
        if (gco->m_lockObjectRotation)
            localRotation = 0.0f;

        claimRotationAction(
            gco->m_targetGroupID,
            gco->m_centerGroupID,
            rotation,
            localRotation,
            ignoreStaticGroups,
            true
        );

        if (mainObject) {
            CCPoint centerPoint = mainObject->getUnmodifiedPosition();
            CCPoint claimedMove = claimMoveAction(gco->m_targetGroupID, ignoreStaticGroups);

            m_areaTransformNode->setPosition(centerPoint);
            m_areaTransformNode->setScaleX(1.0f);
            m_areaTransformNode->setScaleY(1.0f);
            m_areaScaleNode->setScaleX(1.0f);
            m_areaScaleNode->setScaleY(1.0f);
            m_areaTransformNode->setSkewX(0.0f);
            m_areaTransformNode->setSkewY(0.0f);
            m_areaSkewNode->setSkewX(0.0f);
            m_areaSkewNode->setSkewY(0.0f);
            m_areaTransformNode->setRotation(rotation);
            prepareTransformParent(false);

            CCAffineTransform transform1 = m_areaTransformNode->nodeToWorldTransform();
            CCAffineTransform transform2 = m_areaSkewNode->nodeToWorldTransform();
            // Huh? Was was the line above for?
            // I've just noticed it doesn't get used as well lmao
            transform2 = m_areaScaleNode->nodeToWorldTransform();

            m_areaTransformNode2->setScaleX(1.0f);
            m_areaTransformNode2->setScaleY(1.0f);
            m_areaTransformNode2->setSkewX(0.0f);
            m_areaTransformNode2->setSkewY(0.0f);
            m_areaTransformNode2->setRotation(0.0f);

            for (auto object : CCArrayExt<GameObject*, false>(objectsToRotate)) {
                if (!object) continue;

                object->m_unk4fb = gco->m_finishRelated;
                if (!object->m_isDecoration2 && object->m_unk4C4 != m_gameState.m_commandIndex) {
                    object->m_lastPosition.x = object->m_positionX;
                    object->m_lastPosition.y = object->m_positionY;
                    object->m_unk4C4 = m_gameState.m_commandIndex;
                    object->dirtifyObjectRect();
                }

                object->m_isDirty = true;
                object->m_isUnmodifiedPosDirty = true;
            
                CCPoint centerToObject = ccp(
                    object->m_positionX - object->m_positionXOffset - centerPoint.x,
                    object->m_positionY - object->m_positionYOffset - centerPoint.y
                );

                CCPoint rotatedCenterToObject = CCPointApplyAffineTransform(centerToObject, transform1);

                object->m_positionX += rotatedCenterToObject.x + claimedMove.x - object->m_positionX + object->m_positionXOffset;
                object->m_positionY += rotatedCenterToObject.y + claimedMove.y - object->m_positionY + object->m_positionYOffset;

                if (localRotation != 0.0f && object->m_canRotateFree) {
                    object->m_rotationXOffset += localRotation;
                    object->m_rotationYOffset += localRotation;
                    object->addRotation(localRotation);
                    if (object->m_objectType != GameObjectType::Decoration && !object->m_shouldUseOuterOb)
                        object->calculateOrientedBox();
                }
                this->updateObjectSection(object);
            }
        } else if (localRotation != 0.0f) {
            for (auto object : CCArrayExt<GameObject*, false>(objectsToRotate)) {
                if (!object || !object->m_canRotateFree) continue;

                object->m_rotationXOffset += localRotation;
                object->m_rotationYOffset += localRotation;
                object->addRotation(localRotation);
                if (object->m_objectType != GameObjectType::Decoration) {
                    object->m_isObjectRectDirty = true;
                    object->m_isOrientedBoxDirty = true;
                    if (!object->m_shouldUseOuterOb)
                        object->calculateOrientedBox();
                }
            }
        }
    }
    m_objectLayer->setScaleX(objLayerScaleX);
    m_objectLayer->setScaleY(objLayerScaleY);
    m_objectLayer->setPosition(objLayerPos);
}

void decomp_GJBaseGameLayer::optimized_processRotationActions() {
    PROFILER_SCOPE("optimized_processRotationActions");

    float objLayerScaleX = m_objectLayer->getScaleX();
    float objLayerScaleY = m_objectLayer->getScaleY();
    CCPoint objLayerPos = m_objectLayer->getPosition();

    m_objectLayer->setScale(1.0f);
    m_objectLayer->setPosition(CCPointZero);

    for (auto& [_, gcos] : m_effectManager->m_unkMap5c8) {
        for (auto gco : gcos)
            gco->m_unkInt204 = m_gameState.m_commandIndex;
    }

    i32 objectsRotated = 0;

    for (auto gco : m_effectManager->m_unkVector5b0) {
        if (gco->m_unkInt204 != m_gameState.m_commandIndex)
            continue;

        if (gco->m_someInterpValue1RelatedFalse)
            continue;

        GameObject* mainObject = tryGetMainObject(gco->m_centerGroupID);

        GroupManager& groupManager = Renderer::get()->getGroupManager();

        auto objectsToRotate = groupManager.getVanillaRotateGroup(gco->m_targetGroupID);
        // auto objectsToRotateArray = getOptimizedGroup(gco->m_targetGroupID);

        objectsRotated += objectsToRotate.size();

        CCArray* staticGroup = getStaticGroup(gco->m_targetGroupID);

        float rotation;
        bool ignoreStaticGroups;

        if (staticGroup->count() != 0) {
            // objectsToRotateArray = getGroup(gco->m_targetGroupID);
            rotation = gco->m_someInterpValue1RelatedOne - gco->m_someInterpValue1RelatedZero;
            ignoreStaticGroups = false;
        } else {
            rotation = gco->m_someInterpValue2RelatedOne - gco->m_someInterpValue2RelatedZero;
            ignoreStaticGroups = true;
        }

        // CCArrayExt<GameObject*, false> objectsToRotate = objectsToRotateArray;

        if (rotation == 0.0f && !gco->m_finishRelated)
            continue;

        float localRotation = rotation;
        if (gco->m_lockObjectRotation)
            localRotation = 0.0f;

        int targetId = gco->m_targetGroupID;
        int centerId = gco->m_centerGroupID;

        gco->m_someInterpValue1RelatedFalse = true;

        claimRotationAction(
            gco->m_targetGroupID,
            gco->m_centerGroupID,
            rotation,
            localRotation,
            ignoreStaticGroups,
            true
        );

        if (!mainObject)
            groupManager.rotateGroup(gco->m_targetGroupID, rotation, gco->m_lockObjectRotation);
        else {
            auto pos = mainObject->getUnmodifiedPosition();
            groupManager.rotateGroup(gco->m_targetGroupID, rotation, gco->m_lockObjectRotation, ccPointToGLM(pos));
        }

        if (objectsToRotate.empty())
            continue;

        if (mainObject) {
            CCPoint centerPoint = mainObject->getUnmodifiedPosition();
            CCPoint claimedMove = claimMoveAction(gco->m_targetGroupID, ignoreStaticGroups);

            if (claimedMove != ccp(0, 0))
                groupManager.moveGroup(gco->m_targetGroupID, claimedMove.x, claimedMove.y);

            m_areaTransformNode->setPosition(centerPoint);
            m_areaTransformNode->setScaleX(1.0f);
            m_areaTransformNode->setScaleY(1.0f);
            m_areaScaleNode->setScaleX(1.0f);
            m_areaScaleNode->setScaleY(1.0f);
            m_areaTransformNode->setSkewX(0.0f);
            m_areaTransformNode->setSkewY(0.0f);
            m_areaSkewNode->setSkewX(0.0f);
            m_areaSkewNode->setSkewY(0.0f);
            m_areaTransformNode->setRotation(rotation);
            prepareTransformParent(false);

            CCAffineTransform transform1 = m_areaTransformNode->nodeToWorldTransform();
            CCAffineTransform transform2 = m_areaSkewNode->nodeToWorldTransform();
            // Huh? Was was the line above for?
            // I've just noticed it doesn't get used as well lmao
            transform2 = m_areaScaleNode->nodeToWorldTransform();

            m_areaTransformNode2->setScaleX(1.0f);
            m_areaTransformNode2->setScaleY(1.0f);
            m_areaTransformNode2->setSkewX(0.0f);
            m_areaTransformNode2->setSkewY(0.0f);
            m_areaTransformNode2->setRotation(0.0f);

            for (auto object : objectsToRotate) {
                if (!object) continue;

                object->m_unk4fb = gco->m_finishRelated;
                if (!object->m_isDecoration2 && object->m_unk4C4 != m_gameState.m_commandIndex) {
                    object->m_lastPosition.x = object->m_positionX;
                    object->m_lastPosition.y = object->m_positionY;
                    object->m_unk4C4 = m_gameState.m_commandIndex;
                    object->dirtifyObjectRect();
                }

                object->m_isDirty = true;
                object->m_isUnmodifiedPosDirty = true;
            
                CCPoint centerToObject = ccp(
                    object->m_positionX - object->m_positionXOffset - centerPoint.x,
                    object->m_positionY - object->m_positionYOffset - centerPoint.y
                );

                CCPoint rotatedCenterToObject = CCPointApplyAffineTransform(centerToObject, transform1);

                object->m_positionX += rotatedCenterToObject.x + claimedMove.x - object->m_positionX + object->m_positionXOffset;
                object->m_positionY += rotatedCenterToObject.y + claimedMove.y - object->m_positionY + object->m_positionYOffset;

                if (localRotation != 0.0f && object->m_canRotateFree) {
                    object->m_rotationXOffset += localRotation;
                    object->m_rotationYOffset += localRotation;
                    object->addRotation(localRotation);
                    if (object->m_objectType != GameObjectType::Decoration && !object->m_shouldUseOuterOb)
                        object->calculateOrientedBox();
                }
                this->updateObjectSection(object);
            }
        } else if (localRotation != 0.0f) {
            for (auto object : objectsToRotate) {
                if (!object || !object->m_canRotateFree) continue;

                object->m_rotationXOffset += localRotation;
                object->m_rotationYOffset += localRotation;
                object->addRotation(localRotation);
                if (object->m_objectType != GameObjectType::Decoration) {
                    object->m_isObjectRectDirty = true;
                    object->m_isOrientedBoxDirty = true;
                    if (!object->m_shouldUseOuterOb)
                        object->calculateOrientedBox();
                }
            }
        }
    }
    m_objectLayer->setScaleX(objLayerScaleX);
    m_objectLayer->setScaleY(objLayerScaleY);
    m_objectLayer->setPosition(objLayerPos);

    Renderer::get()->setDebugTextEntry("Physics objects rotated", std::to_string(objectsRotated));
}

void decomp_GJBaseGameLayer::processFollowActions() {
    for (auto node : m_effectManager->m_unkVector6d8) {
        auto targetObjects = getGroup(node->getTag());
        auto followObject = tryGetMainObject(node->m_unk074);

        if (!followObject)
            continue;

        float lastPosX = followObject->m_lastPosition.x;
        float lastPosY = followObject->m_lastPosition.y;

        if (followObject->m_unk4C4 != m_gameState.m_commandIndex) {
            lastPosX = followObject->m_positionX;
            lastPosY = followObject->m_positionY;
            followObject->m_unk4C4 = m_gameState.m_commandIndex;
            followObject->dirtifyObjectRect();
        }

        node->m_unk038 = (followObject->m_positionX - lastPosX) * node->m_unk080;
        node->m_unk040 = (followObject->m_positionY - lastPosY) * node->m_unk088;

        bool isMoveX = node->m_unk038 != 0.0f;
        bool isMoveY = node->m_unk040 != 0.0f;

        if (
            isMoveX ||
            isMoveY ||
            node->m_unk078 ||
            followObject->m_unk4fb &&
            m_effectManager->wasFollowing(node->getTag(), node->m_unk074)
        ) {
            GroupCommandObject2* gco = node->m_groupObject;
            if (gco) {
                gco->m_currentXOffset += node->m_unk038;
                gco->m_currentYOffset += node->m_unk040;
            }

            m_effectManager->setFollowing(node->getTag(), node->m_unk074, isMoveX || isMoveY);

            m_followedCount += targetObjects->count();

            for (auto object : CCArrayExt<GameObject*, false>(targetObjects)) {
                if (!object) break;

                if (!object->m_isDecoration2 && object->m_unk4C4 != m_gameState.m_commandIndex) {
                    object->m_lastPosition.x = object->m_positionX;
                    object->m_lastPosition.y = object->m_positionY;
                    object->m_unk4C4 = m_gameState.m_commandIndex;
                    object->dirtifyObjectRect();
                }

                if (isMoveY)
                    object->m_positionY += node->m_unk040;
                object->dirtifyObjectPos();
                if (isMoveX)
                    object->m_positionX += node->m_unk038;

                updateObjectSection(object);
            }

            updateDisabledObjectsLastPos(targetObjects);
        }
    }
}

void decomp_GJBaseGameLayer::optimized_processFollowActions() {
    for (auto node : m_effectManager->m_unkVector6d8) {
        auto targetObjects = getGroup(node->getTag());
        auto followObject = tryGetMainObject(node->m_unk074);

        if (!followObject)
            continue;

        float lastPosX = followObject->m_lastPosition.x;
        float lastPosY = followObject->m_lastPosition.y;

        if (followObject->m_unk4C4 != m_gameState.m_commandIndex) {
            lastPosX = followObject->m_positionX;
            lastPosY = followObject->m_positionY;
            followObject->m_unk4C4 = m_gameState.m_commandIndex;
            followObject->dirtifyObjectRect();
        }

        node->m_unk038 = (followObject->m_positionX - lastPosX) * node->m_unk080;
        node->m_unk040 = (followObject->m_positionY - lastPosY) * node->m_unk088;

        bool isMoveX = node->m_unk038 != 0.0f;
        bool isMoveY = node->m_unk040 != 0.0f;

        if (
            isMoveX ||
            isMoveY ||
            node->m_unk078 ||
            followObject->m_unk4fb &&
            m_effectManager->wasFollowing(node->getTag(), node->m_unk074)
        ) {
            GroupCommandObject2* gco = node->m_groupObject;
            if (gco) {
                gco->m_currentXOffset += node->m_unk038;
                gco->m_currentYOffset += node->m_unk040;
            }

            m_effectManager->setFollowing(node->getTag(), node->m_unk074, isMoveX || isMoveY);

            m_followedCount += targetObjects->count();

            auto renderer = Renderer::get();

            renderer->getGroupManager().moveGroup(node->getTag(), node->m_unk038, node->m_unk040);

            auto reducedTargetObjects = renderer->getGroupManager().getVanillaRotateGroup(node->getTag());

            for (auto object : reducedTargetObjects) {
                if (!object) break;

                if (!object->m_isDecoration2 && object->m_unk4C4 != m_gameState.m_commandIndex) {
                    object->m_lastPosition.x = object->m_positionX;
                    object->m_lastPosition.y = object->m_positionY;
                    object->m_unk4C4 = m_gameState.m_commandIndex;
                    object->dirtifyObjectRect();
                }

                if (isMoveY)
                    object->m_positionY += node->m_unk040;
                object->dirtifyObjectPos();
                if (isMoveX)
                    object->m_positionX += node->m_unk038;

                updateObjectSection(object);
            }

            updateDisabledObjectsLastPos(targetObjects);
        }
    }
}

void decomp_GJBaseGameLayer::processPlayerFollowActions(float dt) {
    
}

// UNTESTED
void decomp_GJBaseGameLayer::processMoveActionsStep(float dt, bool visibleFrame) {
    m_unked0 = 0;
    m_disabledObjectsCount = 0;
    m_unked8 = 0;
    m_areaObjectsCount = 0;

    for (auto& action : m_gameState.m_dynamicMoveActions) {
        auto node = m_effectManager->tryGetMoveCommandNode(action.m_gameObject1->m_targetGroupID);
        if (node) node->m_unk0d1 = false;
    }

    processDynamicObjectActions(1, dt);
    processTransformActions(visibleFrame);
    processRotationActions();
    processDynamicObjectActions(0, dt);
    processMoveActions();
    processPlayerFollowActions(dt);
    processAdvancedFollowActions(dt);
    processFollowActions();
    processAreaActions(dt, visibleFrame);
    
    if (m_isEditor) {
        for (int i = 0; i < m_disabledObjectsCount; ++i)
            m_disabledObjects[i]->quickUpdatePosition();
    }
}

#define kMaxCameraBounds 10000000.0f

void decomp_GJBaseGameLayer::optimized_preUpdateVisibility(float dt) {
    CCSize winSize = CCDirector::sharedDirector()->getWinSize();

    CCPoint camPos = m_gameState.m_cameraLastPosition;
    float zoom = m_gameState.m_cameraZoom;

    if (m_isEditor && !m_started) {
        zoom = m_objectLayer->getScale();
        camPos = -m_objectLayer->getPosition() * (1.f / zoom);

        if (m_gameState.m_cameraShakeEnabled) {
            camPos -= m_gameState.m_cameraShakeOffset;
        }
    }

    float scaledScreenWidth = winSize.width / zoom;
    float scaledScreenHeight = winSize.height / zoom;

    m_cameraWidth = scaledScreenWidth;
    m_cameraHeight = scaledScreenHeight;
    m_halfCameraWidth = scaledScreenWidth / 2;
    m_cameraUnzoomedX = m_gameState.m_cameraLastPosition.x + winSize.width / 2;
    m_activeObjectsCount = 0;

    CCRect viewRect = CCRectMake(camPos.x, camPos.y, scaledScreenWidth, scaledScreenHeight);

    // calculate difference when camera is rotated
    if (m_gameState.m_cameraAngle == 0.f) {
        m_cameraWidthOffset = 0.f;
        m_cameraHeightOffset = 0.f;
        m_cameraUnzoomedHeightOffset = 0.f;
        m_targetCameraHeightOffset = 0.f;
    }
    else {
        if (m_calculateTargetHeightOffset) {
            m_cameraObb2->calculateWithCenter(
                ccp(winSize.width / 2, winSize.height / 2),
                viewRect.size.width,
                viewRect.size.height,
                CC_DEGREES_TO_RADIANS(m_gameState.m_targetCameraAngle)
            );

            m_targetCameraHeightOffset = m_cameraObb2->getBoundingRect().size.height - viewRect.size.height;
            m_calculateTargetHeightOffset = false;
        }
        
        m_cameraObb2->calculateWithCenter(
            ccp(winSize.width / 2, winSize.height / 2),
            viewRect.size.width,
            viewRect.size.height,
            CC_DEGREES_TO_RADIANS(m_gameState.m_cameraAngle)
        );

        CCRect camBounds = m_cameraObb2->getBoundingRect();

        m_cameraUnzoomedHeightOffset = camBounds.size.height - viewRect.size.height;
        m_cameraWidthOffset = (camBounds.size.width - viewRect.size.width) * zoom;
        m_cameraHeightOffset = (camBounds.size.height - viewRect.size.height) * zoom;
    }

    bool rotated = m_gameState.m_cameraAngle != 0.f;
    if (m_groundLayer->m_cameraRotated != rotated) {

        m_groundLayer->m_cameraRotated = rotated;
        m_groundLayer2->m_cameraRotated = rotated;
        m_groundLayer->scaleGround(m_groundLayer->getScaleX());
        m_groundLayer2->scaleGround(m_groundLayer2->getScaleX());
    }

    // scale view rect and update ground shadows if camera is tilted
    if (m_gameState.m_cameraAngle != 0.f && m_cameraWidthOffset != 0.f) {
        updateGroundShadows();
        float scaledCameraWidthOffset = m_cameraWidthOffset / zoom;
        float scaledCameraHeightOffset = m_cameraHeightOffset / zoom;

        viewRect.origin.x -= scaledCameraWidthOffset / 2;
        viewRect.origin.y -= scaledCameraHeightOffset / 2;
        viewRect.size.width += scaledCameraWidthOffset;
        viewRect.size.height += scaledCameraHeightOffset;

        m_updateGroundShadows = true; // applies next frame if camera is straight
    }
    else if (m_updateGroundShadows) {
        m_groundLayer->updateShadowXPos(0.f, 0.f);
        m_groundLayer->updateShadows();
        m_groundLayer2->updateShadowXPos(0.f, 0.f);
        m_groundLayer2->updateShadows();
        m_updateGroundShadows = false;
    }

    float camRight = viewRect.getMaxX();
    float camLeft = viewRect.getMinX();
    float camTop = viewRect.getMaxY();
    float camBottom = viewRect.getMinY();

    // camera bounds cap at 10,000,000 (kMaxCameraBounds)
    m_leftSectionIndex = std::max(floorf(std::min(camLeft, kMaxCameraBounds) * m_sectionXFactor) - 1.f, 0.0f);
    m_rightSectionIndex = std::max(ceilf(std::min(camRight, kMaxCameraBounds) * m_sectionXFactor) + 1.f, 0.0f);
    m_bottomSectionIndex = std::max(floorf(std::min(camBottom, kMaxCameraBounds) * m_sectionYFactor) - 1.f, 0.0f);
    m_topSectionIndex = std::max(ceilf(std::min(camTop, kMaxCameraBounds) * m_sectionYFactor) + 1.f, 0.0f);
}