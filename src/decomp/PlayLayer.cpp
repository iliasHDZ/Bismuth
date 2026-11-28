#include <Geode/Enums.hpp>
#include <Geode/Geode.hpp>
#include <Geode/binding/EffectGameObject.hpp>
#include "PlayLayer.hpp"
#include <ObjectUtils.hpp>
#include "GJBaseGameLayer.hpp"
#include <renderer/Renderer.hpp>

#define COLOR_BG  1000
#define COLOR_P1  1005
#define COLOR_P2  1006
#define COLOR_LBG 1007

#define OBJECT_BREAKABLE_BLOCK 143

using namespace geode::prelude;

void decomp_PlayLayer::optimized_updateVisibility(float dt) {
    if (shouldDebugDraw())
        preUpdateVisibility(dt);
    else
        ((decomp_GJBaseGameLayer*)this)->optimized_preUpdateVisibility(dt);

    m_effectManager->processColors();
    m_effectManager->calculateLightBGColor(m_effectManager->activeColorForIndex(COLOR_P1));

    float audioScale;
    if (m_skipAudioStep)
        audioScale = FMODAudioEngine::sharedEngine()->getMeteringValue();
    else
        audioScale = m_audioEffectsLayer->m_audioScale;
    
    if (m_isSilent || (m_isPracticeMode && !m_practiceMusicSync))
        audioScale = 0.5;

    Renderer::get()->audioScale = audioScale;

    m_player1->m_audioScale = audioScale;
    m_player2->m_audioScale = audioScale;

    // TODO: Cull checkpoints and end portal!

    for (auto checkpoint : CCArrayExt<CheckpointObject*, false>(m_checkpointArray))
        checkpoint->getObject()->activateObject();

    if (m_endPortal)
        m_endPortal->activateObject();

    if (shouldDebugDraw()) {
        for (int index = 0; index < m_activeObjectsCount; index++) {
            GameObject* object = m_activeObjects[index];
            if (object->m_objectType != GameObjectType::Decoration) {
                object->activateObject();
                object->setOpacity(0);
            }
        }
        updateDebugDraw();
    }
}
