#include <Geode/Geode.hpp>
#include <Geode/binding/EffectGameObject.hpp>
#include "PlayLayer.hpp"

#include <algorithm>

#define COLOR_BG  1000
#define COLOR_P1  1005
#define COLOR_P2  1006
#define COLOR_LBG 1007

#define OBJECT_BREAKABLE_BLOCK 143

using namespace cocos2d;

/*
class changed_DynamicBitset {
public:
    // As far as I can tell (in the android version at least). The type in the vector is 64-bit
    gd::vector<unsigned long long> m_bits;
};
*/

#define DYNAMIC_BITSET_GET(D, B) ( ( (D).m_bits[B >> 5] >> (B & 0x1f) ) & 1 )
#define DYNAMIC_BITSET_SET(D, B) ( (D).m_bits[B >> 5] |= ( 1 << (B & 0x1f) ) )

void decomp_PlayLayer::virtual_updateVisibility(float delta) {
    float someValue = m_gameState.m_cameraPosition2.x + m_cameraWidth;

    // Windows only
    *(float*)((char*)GetModuleHandle(NULL) + 0x6a304c) = someValue;

    preUpdateVisibility(delta);
    m_effectManager->processColors();
    m_effectManager->calculateLightBGColor(m_effectManager->activeColorForIndex(COLOR_P1));

    ccColor3B brightBGColor = GameToolbox::transformColor(m_background->getColor(), 0.0, -0.3, 0.4);

    ccColor3B colorLBG = m_effectManager->activeColorForIndex(COLOR_LBG);
    ccColor3B colorBG  = m_effectManager->activeColorForIndex(COLOR_BG);

    m_lightBGColor = GameToolbox::transformColor(colorBG, 0.0, -0.2, 0.2);

    for (int i = 0; i < m_keyPulses.m_bits.size(); i++)
        m_keyPulses.m_bits[i] = 0;

    CCSize winSize = CCDirector::sharedDirector()->getWinSize();

    float centerLeftX  = winSize.width * 0.5 - 75.0;
    float centerRightX = centerLeftX + 110.0;

    float someScreenLeft = CCDirector::sharedDirector()->getScreenRight() - centerRightX - 90.0;

    ccColor3B someLBGColor;
    if ((float)(colorBG.r + colorBG.g + colorBG.b) >= 150.0f)
        someLBGColor = ccColor3B { 255, 255, 255 };
    else
        someLBGColor = colorLBG;

    bool isPlayerDead = m_player1->m_isDead;
    bool isLevelFlipping = isFlipping();

    float audioScale;
    if (m_skipAudioStep)
        audioScale = FMODAudioEngine::sharedEngine()->getMeteringValue();
    else
        audioScale = m_audioEffectsLayer->m_audioScale;
    
    if (m_isSilent || (m_isPracticeMode && !m_practiceMusicSync))
        audioScale = 0.5;

    m_player1->m_audioScale = audioScale;
    m_player2->m_audioScale = audioScale;

    for (int index = 0; index < m_activeObjectsCount; index++) {
        GameObject* object = m_activeObjects[index];

        if (object->m_mainColorKeyIndex < 1) {
            object->updateMainColor();
            object->updateSecondaryColor();
        } else {
            if (!DYNAMIC_BITSET_GET(m_keyPulses, object->m_mainColorKeyIndex)) {
                m_keyColors[object->m_mainColorKeyIndex] = object->colorForMode(object->m_activeMainColorID, true);
                m_keyOpacities[object->m_mainColorKeyIndex] = object->opacityModForMode(object->m_activeMainColorID, true);

                DYNAMIC_BITSET_SET(m_keyPulses, object->m_mainColorKeyIndex);
            }

            object->updateMainColor(m_keyColors[object->m_mainColorKeyIndex]);
            object->m_baseColor->m_opacity = m_keyOpacities[object->m_mainColorKeyIndex];

            if (object->hasSecondaryColor()) {
                if (!DYNAMIC_BITSET_GET(m_keyPulses, object->m_detailColorKeyIndex)) {
                    m_keyColors[object->m_detailColorKeyIndex] = object->colorForMode(object->m_activeDetailColorID, false);
                    m_keyOpacities[object->m_detailColorKeyIndex] = object->opacityModForMode(object->m_activeDetailColorID, false);

                    DYNAMIC_BITSET_SET(m_keyPulses, object->m_detailColorKeyIndex);
                }

                object->updateSecondaryColor(m_keyColors[object->m_detailColorKeyIndex]);
                object->m_detailColor->m_opacity = m_keyOpacities[object->m_detailColorKeyIndex];
            }
        }

        if (object->m_isActivated) { // It also checks something about m_blendingColors but I don't think it matters
            if (
                (object->hasSecondaryColor() && m_blendingColors.count(object->m_activeDetailColorID)) ||
                m_blendingColors.count(object->m_activeMainColorID)
            ) {
                object->addMainSpriteToParent(0);
                object->addColorSpriteToParent(false);
            }
        }

        object->activateObject();

        CCPoint position = object->getRealPosition();
        m_enterEffectPosition = position;

        int objectEnterEffect, enterEffectType;
        bool isEnterEffectRight;

        if (object->m_enterType == -1) {
            if (object->m_exitType == -1 && m_enterEffectPosition.x > m_gameState.m_cameraPosition2.x + m_halfCameraWidth)
                goto thing; // it hurts my heart to use a goto
            isEnterEffectRight = false;
            enterEffectType = m_gameState.m_exitChannelMap[object->m_enterChannel];
            objectEnterEffect = object->m_exitType;
        } else {
            thing:
            isEnterEffectRight = true;
            enterEffectType = m_gameState.m_enterChannelMap[object->m_enterChannel];
            objectEnterEffect = object->m_enterType;
        }

        if (object->m_isUIObject) {
            objectEnterEffect = -14;
            enterEffectType = -14;
        }

        if (object->getHasSyncedAnimation())
            ((EnhancedGameObject*)object)->updateSyncedAnimation(m_gameState.m_totalTime, -1);

        if (object->getHasRotateAction())
            ((EnhancedGameObject*)object)->updateRotateAction(delta);

        if (object->m_unk367)
            ((AnimatedGameObject*)object)->updateChildSpriteColor(brightBGColor);
        
        if (object->getType() == GameObjectType::Collectible)
            ((EffectGameObject*)object)->updateInteractiveHover(m_hoverNode->getPosition().y);

        if (object->m_objectID == OBJECT_BREAKABLE_BLOCK)
            object->setGlowColor(colorLBG);

        if (object->m_unk3F8)
            continue;

        if (object->m_usesAudioScale && !object->m_hasNoAudioScale) {
            if (!object->m_customAudioScale) {
                object->setRScale(audioScale);
            } else {
                float customAudioScale = (audioScale - 0.1) * (object->m_maxAudioScale - object->m_minAudioScale) + object->m_minAudioScale;
                object->setRScale(customAudioScale);
            }
        }

        if (object->m_glowColorIsLBG) {
            if (!object->m_customGlowColor)
                object->setGlowColor(m_lightBGColor);
            else
                object->setGlowColor(colorLBG);
        }

        // Invisible block is one of these objects that become more transparent
        // when they're closer to the center of the screen

        if (!object->m_isInvisibleBlock) {
            /*
            if (
                !object->ignoreFade() && enterEffectType != 14 &&
                (
                    !object->m_intrinsicDontFade ||
                    (object->m_isSolidColorBlock && object->m_baseOrDetailBlending) ||
                    !(objectEnterEffect >= -2 && objectEnterEffect < 0) ||
                    enterEffectType != -2 
                )
            ) {
                if (enterEffectType != -15) {
                    CCPoint fadePos = position;

                    if (isEnterEffectRight)
                        fadePos.x -= object->m_fadeMargin;
                    else
                        fadePos.y += object->m_fadeMargin;

                    float relMod = getRelativeModNew(fadePos, 70.0, 0.0, false, isEnterEffectRight);
                    object->setOpacity(relMod * 255.0);
                }
            } else {
                object->setOpacity(255);
            }
            */
            if (
                object->ignoreFade() || enterEffectType == -14 ||
                (
                    object->m_intrinsicDontFade &&
                    !(object->m_isSolidColorBlock && object->m_baseOrDetailBlending) &&
                    !(objectEnterEffect >= -2 && objectEnterEffect < 0) &&
                    enterEffectType == -2
                )
            ) {
                object->setOpacity(255);
            } else if (enterEffectType != -15) {
                CCPoint fadePos = position;

                if (isEnterEffectRight)
                    fadePos.x -= object->m_fadeMargin;
                else
                    fadePos.x += object->m_fadeMargin;

                float relMod = getRelativeModNew(fadePos, 70.0, 0.0, false, isEnterEffectRight);
                object->setOpacity(relMod * 255.0);
            }
        } else {
            if (!isPlayerDead) {
                CCPoint fadePos = object->getRealPosition();
                if (fadePos.x <= m_cameraUnzoomedX)
                    fadePos.x += object->m_fadeMargin;
                else
                    fadePos.x -= object->m_fadeMargin;
                
                float relMod = getRelativeMod(fadePos, 0.02, 0.014285714, 0.0) * 255.0;

                float someWidth1;
                if (fadePos.x <= centerRightX + m_gameState.m_cameraPosition2.x)
                    someWidth1 = (centerLeftX + m_gameState.m_cameraPosition2.x - fadePos.x) / std::max(centerLeftX - 30.f, 1.f);
                else
                    someWidth1 = (fadePos.x - m_gameState.m_cameraPosition2.x - centerRightX) / std::max(someScreenLeft, 1.f);

                someWidth1 = std::clamp(someWidth1, 0.0f, 1.0f);

                object->setOpacity(    std::min((someWidth1 * 0.95f + 0.05f) * 255.f, relMod));
                object->setGlowOpacity(std::min((someWidth1 * 0.85f + 0.15f) * 255.f, relMod));

                float opacity = (float)object->getOpacity() / 255.0;
                if (opacity <= 0.8)
                    object->setGlowColor(m_lightBGColor);
                else {
                    ccColor3B color = GJEffectManager::getMixedColor(
                        m_lightBGColor,
                        someLBGColor,
                        (1.f - (opacity - 0.8f) / 0.2f) * 0.3f + 0.7f
                    );
                    object->setGlowColor(color);
                }
            } else {
                object->setGlowColor(m_lightBGColor);
                object->setOpacity(255);
                object->setGlowOpacity(255);
            }
        }

        if (enterEffectType == -15 && !object->ignoreEnter()) {
            // applyCustomEnterEffect(object, isEnterEffectRight);
        } else {
            // geode::log::info("enter: {}", objectEnterEffect);
            applyEnterEffect(object, enterEffectType, isEnterEffectRight);
        }

        if (!isLevelFlipping) {
            if (m_resetActiveObjects) {
                object->setFlipX(object->m_startFlipX);
                object->setFlipY(object->m_startFlipY);
                object->setRotation(object->m_startRotationX);
                object->setPosition(object->getPosition());
            }
            continue;
        }

        float flip = m_gameState.m_levelFlipping;
        if (m_cameraFlip == -1.0)
            flip = 1.0 - flip;

        CCSize winSize = CCDirector::sharedDirector()->getWinSize();
        CCPoint objectPos = object->getPosition();

        float newObjectX = winSize.width / m_gameState.m_cameraZoom
                            - (objectPos.x - m_gameState.m_cameraPosition.x) * 2.0
                            + flip * objectPos.x;
        
        object->setPosition(CCPoint(newObjectX, objectPos.x));

        float rotation = abs(object->getRotation());

        bool is270      = rotation == 270.0;
        bool is90       = rotation == 90.0;
        bool is90Divide = (int)rotation % 90 == 0;

        if (m_gameState.m_levelFlipping >= 0.5) {
            if (!m_gameState.m_unkBool11) 
                return;
            
            int someFlip = m_gameState.m_levelFlipping <= 0.5 ? 1 : -1;

            if (is270 || is90)
                object->setFlipY((someFlip * (int)object->m_startFlipY) != 0);
            else
                object->setFlipX((someFlip * (int)object->m_startFlipX) != 0);

            if (is90Divide)
                object->setRotation((float)someFlip * object->m_startRotationX);
        } else {
            if (is270 || is90)
                object->setFlipY(!object->m_startFlipY);
            else
                object->setFlipX(!object->m_startFlipX);

            if (!is90Divide)
                object->setRotation(-object->m_startRotationX);
        }
    }

    updateEnterEffects(delta);
    processAreaVisualActions(delta);
    updateParticles(delta);

    m_resetActiveObjects = false;
    m_blendingColors.clear(); // This might be wrong, idk. But it makes sense to me.
}
    
void decomp_PlayLayer::optimized_updateVisibility(float delta) {
    // preUpdateVisibility(delta);
    m_effectManager->processColors();
    m_effectManager->calculateLightBGColor(m_effectManager->activeColorForIndex(COLOR_P1));
}