#include "Renderer.hpp"
#include "Geode/cocos/CCDirector.h"
#include "Geode/cocos/kazmath/include/kazmath/mat4.h"
#include "Geode/cocos/platform/win32/CCGL.h"
#include "ccTypes.h"
#include "common.hpp"
#include "decomp/PlayLayer.hpp"
#include <Geode/Enums.hpp>
#include <Geode/binding/GJBaseGameLayer.hpp>
#include <Geode/binding/RingObject.hpp>

using namespace geode::prelude;

static Renderer* currentRenderer;

static u64 totalFrameTime = 0;

Renderer::~Renderer() { terminate(); }

static std::string byteSizeToString(usize size) {
    std::string unit = "B";
    double dsize = size;
    if (dsize > 1000.0) { dsize /= 1000.0; unit = "kB"; }
    if (dsize > 1000.0) { dsize /= 1000.0; unit = "MB"; }
    return fmt::format("{:.3f}{}", dsize, unit);
}

bool Renderer::init(PlayLayer* layer) {
    this->layer = layer;
    
    auto size = CCDirector::get()->getWinSize();

    if (!Mod::get()->getSettingValue<bool>("enabled"))
        return false;

    log::info("OpenGL Version: {}", (const char*)glGetString(GL_VERSION));

    auto tcache = cocos2d::CCTextureCache::get();
    // TODO: Add text sheet
    spriteSheets[(i32)SpriteSheet::GAME_1]   = tcache->addImage("GJ_GameSheet.png", false);
    spriteSheets[(i32)SpriteSheet::GAME_2]   = tcache->addImage("GJ_GameSheet02.png", false);
    spriteSheets[(i32)SpriteSheet::PARTICLE] = tcache->addImage("GJ_ParticleSheet.png", false);
    spriteSheets[(i32)SpriteSheet::GLOW]     = tcache->addImage("GJ_GameSheetGlow.png", false);
    spriteSheets[(i32)SpriteSheet::FIRE]     = tcache->addImage("FireSheet_01.png", false);
    spriteSheets[(i32)SpriteSheet::PIXEL]    = tcache->addImage("PixelSheet_01.png", false);

    ObjectSorter sorter;

    sorter.initForGameLayer(layer);
    /*
    for (int x = 0; x < layer->m_sections.size(); x++) {
        if (layer->m_sections[x] == nullptr)
            continue;
        for (int y = 0; y < layer->m_sections[x]->size(); y++) {
            auto section = layer->m_sections[x]->at(y);

            if (section == nullptr || layer->m_sectionSizes[x] == nullptr)
                continue;

            auto size = layer->m_sectionSizes[x]->at(y);
            
            for (int i = 0; i < size; i++) {
                auto object = section->at(i);
                if (object == layer->m_anticheatSpike)
                    continue;

                sorter.addGameObject(object);
            }
        }
    }
    */

    groupStateCount = 1;

    log::info("Level contains {} object(s)", layer->m_objects->count());
    log::info("Sorting objects...");
    for (auto object : CCArrayExt<GameObject*>(layer->m_objects)) {
        if (object == layer->m_anticheatSpike) {
            DEBUG_LOG("- anti-cheat spike");
            continue;
        }

        if (object->isTrigger())
            continue;

        for (i32 i = 0; i < object->m_groupCount; i++) {
            auto id = object->m_groups->at(i);
            if ((id + 1) > groupStateCount)
                groupStateCount = id + 1;
            usedGroupIds.insert(id);
        }

        DEBUG_LOG("- {}, id: {}", (void*)object, object->m_objectID);
        DEBUG_LOG("  - zlayer: {}", (i32)object->getObjectZLayer());
        DEBUG_LOG("  - blending: {}", object->m_baseOrDetailBlending);
        DEBUG_LOG("  - spritesheet: {}", object->getParentMode());
        DEBUG_LOG("  - glowColorIsLBG: {}", object->m_glowColorIsLBG);
        DEBUG_LOG("  - customGlowColor: {}", object->m_customGlowColor);
        DEBUG_LOG("  - opacityMod: {}", object->m_opacityMod);
        if (object->m_baseColor && object->m_baseColor->m_usesHSV)
            DEBUG_LOG("  - baseHSV: {} {} {} {} {}", object->m_baseColor->m_hsv.h, object->m_baseColor->m_hsv.s, object->m_baseColor->m_hsv.v, object->m_baseColor->m_hsv.absoluteSaturation, object->m_baseColor->m_hsv.absoluteBrightness);
        if (object->m_detailColor && object->m_detailColor->m_usesHSV)
            DEBUG_LOG("  - detailHSV: {} {} {} {} {}", object->m_detailColor->m_hsv.h, object->m_detailColor->m_hsv.s, object->m_detailColor->m_hsv.v, object->m_detailColor->m_hsv.absoluteSaturation, object->m_detailColor->m_hsv.absoluteBrightness);
        if (object->getHasRotateAction())
            DEBUG_LOG("  - rotationDelta: {}", ((EnhancedGameObject*)object)->m_rotationDelta);

        sorter.addGameObject(object);
    }
    sorter.finalizeSorting();

    renderedGameObjectCount = 0;
    for (auto it = sorter.iterator(); !it.isEnd(); it.next()){
        renderedGameObjectCount++;
        objectBatch.reserveForGameObject(it.get());
    }

    log::info("Generating static rendering buffer....");

    generateStaticRenderingBuffer(sorter);

    log::info("Generating vertex buffer...");

    objectBatch.allocateReservations();
    for (auto it = sorter.iterator(); !it.isEnd(); it.next())
        objectBatch.writeGameObject(it.get());
    objectBatch.finishWriting();

    std::map<std::string, std::string> shaderMacroVariables;

    shaderMacroVariables["TOTAL_OBJECT_COUNT"] = std::to_string(renderedGameObjectCount == 0 ? 1 : renderedGameObjectCount);
    shaderMacroVariables["GROUP_ID_LIMIT"]     = std::to_string(groupStateCount);

    log::info("Compiling shaders...");

    shader = Shader::create("object.vert", "object.frag", shaderMacroVariables);
    if (!shader)
        return false;

    drbBuffer = Buffer::createDynamicDraw(sizeof(DynamicRenderingBuffer) + sizeof(GroupState) * groupStateCount);
    if (!drbBuffer)
        return false;

    uniformBuffer = Buffer::createDynamicDraw(sizeof(RendererUniformBuffer));
    if (!uniformBuffer)
        return false;

    drb = (DynamicRenderingBuffer*)malloc(drbBuffer->getSize());

    debugText = CCLabelBMFont::create("", "chatFont.fnt");

    debugText->setAnchorPoint(CCPoint(0, 1));
    debugText->setPosition(1, CCDirector::get()->getWinSize().height - 8);
    debugText->setScale(0.5);
    layer->addChild(debugText, 1000);

    setZOrder(-2);
    setEnabled(true);

    ingameEnableDisable = Mod::get()->getSettingValue<bool>("ingame_enable");

    rendererStartTime = getTime();

    log::info("Renderer initialized");
    return true;
}

void Renderer::terminate() {
    if (shader)
        Shader::destroy(shader);
    shader = nullptr;

    if (drbBuffer)
        Buffer::destroy(drbBuffer);
    if (drb)
        free(drb);

    if (srbBuffer)
        Buffer::destroy(srbBuffer);

    if (uniformBuffer)
        Buffer::destroy(uniformBuffer);

    currentRenderer = nullptr;
    log::info("Renderer terminated");
}

void Renderer::prepareShaderUniforms() {
    kmMat4 matrixP;
	kmMat4 matrixMV;
	kmMat4 matrixMVP;

    glBlendFunc(CC_BLEND_SRC, CC_BLEND_DST);
	
	kmGLGetMatrix(KM_GL_PROJECTION, &matrixP);
	kmGLGetMatrix(KM_GL_MODELVIEW, &matrixMV);
	
	kmMat4Multiply(&matrixMVP, &matrixP, &matrixMV);

    shader->setTextureArray("u_spriteSheets", (i32)SpriteSheet::COUNT, spriteSheets);

    (kmMat4&)uniforms.u_mvp = matrixMVP;
    uniforms.u_timer = gameTimer;
    uniforms.u_cameraPosition = ccPointToGLM(layer->m_gameState.m_cameraPosition2);
    uniforms.u_cameraViewSize = glm::vec2(layer->m_cameraWidth, layer->m_cameraHeight);
    auto winsize = CCDirector::get()->getWinSize();
    uniforms.u_winSize = glm::vec2(winsize.width, winsize.height);
    uniforms.u_screenRight = CCDirector::get()->getScreenRight();
    uniforms.u_cameraUnzoomedX = layer->m_cameraUnzoomedX;
    ccColor3B specialLightBG = GameToolbox::transformColor(layer->m_effectManager->activeColorForIndex(COLOR_CHANNEL_BG), 0.0, -0.2, 0.2);
    uniforms.u_specialLightBGColor = ccColor3BToGLM(specialLightBG);

    uniforms.u_gameStateFlags = 0;
    if (layer->m_player1->m_isDead)
        uniforms.u_gameStateFlags |= GAME_STATE_IS_PLAYER_DEAD;

    if (layer->m_skipAudioStep)
        uniforms.u_audioScale = FMODAudioEngine::sharedEngine()->getMeteringValue();
    else
        uniforms.u_audioScale = layer->m_audioEffectsLayer->m_audioScale;
    
    if (layer->m_isSilent || (layer->m_isPracticeMode && !layer->m_practiceMusicSync))
        uniforms.u_audioScale = 0.5;

    uniformBuffer->write(&uniforms, sizeof(RendererUniformBuffer));
    uniformBuffer->bindAsUniformBuffer(RENDERER_UNIFORM_BUFFER_BINDING);
}

void Renderer::prepareDynamicRenderingBuffer() {
    auto prevTime = getTime();
    auto effectManager = layer->m_effectManager;

    for (usize i = 0; i < effectManager->m_colorActionSpriteVector.size(); i++) {
        auto sprite = effectManager->m_colorActionSpriteVector[i];
        if (sprite == nullptr)
            continue;

        auto id = sprite->m_colorID;

        drb->channelColors[id].r = sprite->m_color.r;
        drb->channelColors[id].g = sprite->m_color.g;
        drb->channelColors[id].b = sprite->m_color.b;
        drb->channelColors[id].a = (u8)sprite->m_opacity;

        bool shouldBlend = layer->shouldBlend(id);
        if (
            id == COLOR_CHANNEL_P1 ||
            id == COLOR_CHANNEL_P2 ||
            id == COLOR_CHANNEL_LBG
        ) {
            shouldBlend = true;
        }

        if (shouldBlend)
            drb->colorChannelBlendingBitmap[id >> 5] |= 1 << (id & 0x1f);
        else
            drb->colorChannelBlendingBitmap[id >> 5] &= ~(1 << (id & 0x1f));
    }

    drb->channelColors[COLOR_CHANNEL_BLACK] = { 0, 0, 0, 255 };

    for (auto groupId : usedGroupIds) {
        float opacity = 0.0;
        if (!disabledGroups.contains(groupId))
            opacity = layer->m_effectManager->opacityModForGroup(groupId);

        drb->groupStates[groupId].opacity = opacity;
    }

    drbBuffer->write(drb, drbBuffer->getSize());
    drbBuffer->bindAsUniformBuffer(DYNAMIC_RENDERING_BUFFER_BINDING);

    drbGenerationTime = getTime() - prevTime;
}

static u32 convertToShaderHSV(const ccHSVValue& hsv) {
    u32 hue = hsv.h + 256.f;
    u32 sat = ( hsv.s + (hsv.absoluteSaturation ? 1.0 : 0.0) ) * 127.5f;
    u32 val = ( hsv.v + (hsv.absoluteBrightness ? 1.0 : 0.0) ) * 127.5f;

    u32 ret = (hue & HSV_HUE_MASK) << HSV_HUE_BIT |
              (sat & HSV_SAT_MASK) << HSV_SAT_BIT |
              (val & HSV_VAL_MASK) << HSV_VAL_BIT;

    if (hsv.absoluteSaturation) ret |= HSV_SAT_ADD;
    if (hsv.absoluteBrightness) ret |= HSV_VAL_ADD;
    return ret;
}

void Renderer::generateStaticRenderingBuffer(ObjectSorter& sorter) {
    usize srbSize = sizeof(StaticObjectInfo) * renderedGameObjectCount;
    auto srb = (StaticRenderingBuffer*)malloc(srbSize);

    auto objects = srb->objects;
    usize index = 0;

    for (auto it = sorter.iterator(); !it.isEnd(); it.next()) {
        auto object = it.get();
        auto objectInfo = &objects[index];

        objectInfo->objectPosition = ccPointToGLM(object->getPosition());
        if (object->getHasRotateAction())
            objectInfo->rotationSpeed = ((EnhancedGameObject*)object)->m_rotationDelta; // 'rotationDelta' is incorrect naming
        else
            objectInfo->rotationSpeed = 0.0;

        objectInfo->flags = 0;
        if (object->m_usesAudioScale) {
            objectInfo->flags |= OBJECT_FLAG_USES_AUDIO_SCALE;
        
            if (object->m_customAudioScale) {
                objectInfo->flags |= OBJECT_FLAG_CUSTOM_AUDIO_SCALE;
                objectInfo->audioScaleMin = object->m_minAudioScale;
                objectInfo->audioScaleMax = object->m_maxAudioScale;
            }
            
            if (typeinfo_cast<RingObject*>(object))
                objectInfo->flags |= OBJECT_FLAG_IS_ORB;
        }

        if (object->m_baseColor && object->m_baseColor->m_usesHSV) {
            objectInfo->flags |= OBJECT_FLAG_HAS_BASE_HSV;
            objectInfo->baseHSV = convertToShaderHSV(object->m_baseColor->m_hsv);
        }

        if (object->m_detailColor && object->m_detailColor->m_usesHSV) {
            objectInfo->flags |= OBJECT_FLAG_HAS_DETAIL_HSV;
            objectInfo->detailHSV = convertToShaderHSV(object->m_detailColor->m_hsv);
        }

        memset(objectInfo->groupIds, 0, sizeof(objectInfo->groupIds));

        for (i32 i = 0; i < object->m_groupCount; i++) {
            auto dstGroupId = &objectInfo->groupIds[i >> 1];
            auto groupId = object->m_groups->at(i);
            if ((i & 1) == 0)
                *dstGroupId |= groupId;
            else
                *dstGroupId |= groupId << 16;
        }

        if (object->m_isInvisibleBlock) objectInfo->flags |= OBJECT_FLAG_IS_INVISIBLE_BLOCK;
        if (object->m_customGlowColor)  objectInfo->flags |= OBJECT_FLAG_SPECIAL_GLOW_COLOR;
        objectInfo->fadeMargin = object->m_fadeMargin;

        objectSRBIndicies[object] = index;
        index++;
    }

    srbBuffer = Buffer::createStaticDraw(srb, srbSize);
    free(srb);
};

void Renderer::draw() {
    u64 prevTime = getTime();
    
    if (debugTextEnabled) {
        // glBeginQuery(GL_TIME_ELAPSED, 50);
    }

    storeGLStates();

    shader->use();

    prepareShaderUniforms();
    prepareDynamicRenderingBuffer();

    srbBuffer->bindAsStorageBuffer(STATIC_RENDERING_BUFFER_BINDING);

    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    objectBatch.draw();

    restoreGLStates();
    
    if (debugTextEnabled) {
        renderTime = 0;
        // glEndQuery(GL_TIME_ELAPSED);
        // glGetQueryObjecti64v(50, GL_QUERY_RESULT, &renderTime);
    }
    drawFuncTime = getTime() - prevTime;

    if (debugText->isVisible())
        updateDebugText();
}

void Renderer::updateDebugText() {
    std::string text = "";

    if (!enabled) {
        text += "Bismuth renderer is disabled\n";
        text += fmt::format("Total frame time: {}ms\n", (double)totalFrameTime / 1000000.0);
        text += "Press F8 to enable\n";
        debugText->setString(text.c_str());
        return;
    }

    if (debugTextEnabled) {
        auto screenSize = CCDirector::get()->getWinSizeInPixels();

        text += fmt::format("Bismuth renderer {}\n", Mod::get()->getVersion().toVString());
        text += fmt::format("OpenGL {}\n", (const char*)glGetString(GL_VERSION));
        text += fmt::format("{}\n", (const char*)glGetString(GL_RENDERER));
        text += fmt::format("Window: {}x{}\n", screenSize.width, screenSize.height);
        text += fmt::format("Render time: {}ms\n", (double)renderTime / 1000000.0);
        text += fmt::format("DRB generation time: {}ms\n", (double)drbGenerationTime / 1000000.0);
        text += fmt::format("Renderer::draw() time: {}ms\n", (double)drawFuncTime / 1000000.0);
        text += fmt::format("GJBaseGameLayer::update() time: {}ms\n", (double)gjbglUpdateTime / 1000000.0);
        text += fmt::format("Total frame time: {}ms\n", (double)totalFrameTime / 1000000.0);
        text += fmt::format("Vertex buffer size: {}\n", byteSizeToString(objectBatch.getQuadCount() * sizeof(ObjectQuad)));
        text += fmt::format("Static rendering buffer size: {}\n", byteSizeToString(srbBuffer->getSize()));
        text += fmt::format("Dynamic rendering buffer size: {}\n", byteSizeToString(drbBuffer->getSize()));
        text += "\n";
        text += "Press F3 to hide this screen";
    } else if (differenceModeEnabled)
        text += "_";

    debugText->setString(text.c_str());
}

void Renderer::update(float dt) {
    gameTimer += dt;
    
    float audioScale;
    if (layer->m_skipAudioStep)
        audioScale = FMODAudioEngine::sharedEngine()->getMeteringValue();
    else
        audioScale = layer->m_audioEffectsLayer->m_audioScale;
    
    if (layer->m_isSilent || (layer->m_isPracticeMode && !layer->m_practiceMusicSync))
        audioScale = 0.5;

    if (debugText->isVisible())
        updateDebugText();
}

Ref<Renderer> Renderer::create(PlayLayer* layer) {
    auto ren = new Renderer;

    if (ren->init(layer)) {
        ren->autorelease();
        currentRenderer = ren;
        return ren;
    }
    
    delete ren;
    return nullptr;
}

Ref<Renderer> Renderer::get() {
    return currentRenderer;
}

bool Renderer::useOptimizations() {
    if (canEnableDisableIngame())
        return false;

    return true;
}

void Renderer::setEnabled(bool enabled) {
    this->enabled = enabled;
    this->setVisible(enabled);
    for (auto batch : CCArrayExt<CCNode*>(layer->m_batchNodes))
        batch->setVisible(!enabled);
    updateDebugText();
}

void Renderer::moveGroup(i32 groupId, float deltaX, float deltaY) {
    drb->groupStates[groupId].offset += glm::vec2(deltaX, deltaY);
}

void Renderer::toggleGroup(i32 groupId, bool visible) {
    if (visible)
        disabledGroups.erase(groupId);
    else
        disabledGroups.insert(groupId);
}

void Renderer::resetGroups() {
    for (i32 i = 0; i < groupStateCount; i++)
        drb->groupStates[i].offset = glm::vec2(0, 0);
    disabledGroups.clear();
}

#include <Geode/modify/PlayLayer.hpp>
class $modify(RendererPlayLayer, PlayLayer) {
    void setupHasCompleted() {
        PlayLayer::setupHasCompleted();
        
        auto batchLayer = this->m_objectLayer;
        if (!batchLayer) {
            log::error("failed to attach renderer: batch layer not found");
            return;
        }

        auto renderer = Renderer::create(this);
        if (renderer)
            batchLayer->addChild(renderer);
    }

    void resetLevel() {
        PlayLayer::resetLevel();

        auto renderer = Renderer::get();
        if (renderer)
            renderer->resetGroups();
    }

    void updateVisibility(float dt) {
        auto renderer = Renderer::get();
        if (renderer && renderer->useOptimizations()) {
            ((decomp_PlayLayer*)this)->optimized_updateVisibility(dt);
            return;
        }

        PlayLayer::updateVisibility(dt);
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
        u64 time = getTime();
        GJBaseGameLayer::update(dt);
        auto renderer = Renderer::get();
        if (renderer) {
            renderer->update(dt);
            renderer->setGJBGLUpdateTime(getTime() - time);
        }
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

            renderer->moveGroup(groupId, node->m_unk038, node->m_unk040);

            CCArray* objects = getStaticGroup(groupId);
            if (objects)
                moveObjects(objects, node->m_unk038, node->m_unk040, 0);

            objects = getOptimizedGroup(groupId);
            if (objects)
                moveObjects(objects, node->m_unk090, node->m_unk098, 0);
        }
    }

    void toggleGroup(int id, bool activate) {
        GJBaseGameLayer::toggleGroup(id, activate);
        auto renderer = Renderer::get();
        if (renderer)
            renderer->toggleGroup(id, activate);
    }

    void optimizeMoveGroups() {
        GJBaseGameLayer::optimizeMoveGroups();

        auto renderer = Renderer::get();
        if (!renderer || !renderer->useOptimizations())
            return;

        for (auto array : m_optimizedGroups)
            removeDecoObjects(array);
        for (auto array : m_staticGroups)
            removeDecoObjects(array);
    }
};

#include <Geode/modify/CCKeyboardDispatcher.hpp>
class $modify(RendererCCKeyboardDispatcher, cocos2d::CCKeyboardDispatcher) {
    bool dispatchKeyboardMSG(cocos2d::enumKeyCodes key, bool keyDown, bool isKeyRepeat) {
        auto renderer = Renderer::get();
        if (renderer) {
            if (keyDown && key == cocos2d::KEY_F3 && !isKeyRepeat)
                renderer->toggleDebugText();
            if (renderer->canEnableDisableIngame()) {
                if (keyDown && key == cocos2d::KEY_F8 && !isKeyRepeat)
                    renderer->setEnabled(!renderer->isEnabled());
                if (keyDown && key == cocos2d::KEY_F9 && !isKeyRepeat)
                    renderer->setDifferenceModeEnabled(!renderer->isDifferenceModeEnabled());
            }
        }

        return cocos2d::CCKeyboardDispatcher::dispatchKeyboardMSG(key, keyDown, isKeyRepeat);
    }
};

#include <Geode/modify/CCDisplayLinkDirector.hpp>
class $modify(RendererCCDisplayLinkDirector, CCDisplayLinkDirector) {
    void mainLoop() {
        u64 prevTime = getTime();
        CCDisplayLinkDirector::mainLoop();
        totalFrameTime = getTime() - prevTime;
    }
};

static i32 storedVAO, storedVBO, storedIBO, storedProgram;
static i32 storedBlendSrcAlpha, storedBlendSrcRGB;
static i32 storedBlendDstAlpha, storedBlendDstRGB;

static i32 storedTextures[9];

void storeGLStates() {
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &storedVAO);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &storedVBO);
    glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &storedIBO);
    glGetIntegerv(GL_CURRENT_PROGRAM, &storedProgram);
    glGetIntegerv(GL_CURRENT_PROGRAM, &storedProgram);
    
    glGetIntegerv(GL_BLEND_SRC_ALPHA, &storedBlendSrcAlpha);
    glGetIntegerv(GL_BLEND_SRC_RGB,   &storedBlendSrcRGB);
    glGetIntegerv(GL_BLEND_DST_ALPHA, &storedBlendDstAlpha);
    glGetIntegerv(GL_BLEND_DST_RGB,   &storedBlendDstRGB);

    for (int i = 0; i < 9; i++) {
        glActiveTexture(GL_TEXTURE0 + i);
        glGetIntegerv(GL_TEXTURE_BINDING_2D, storedTextures + i);
    }
    glActiveTexture(GL_TEXTURE0);
}

void restoreGLStates() {
    glBindVertexArray(storedVAO);
    glBindBuffer(GL_ARRAY_BUFFER, storedVBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, storedIBO);
    glUseProgram(storedProgram);

    glBlendFuncSeparate(
        storedBlendSrcRGB,
        storedBlendDstRGB,
        storedBlendSrcAlpha,
        storedBlendDstAlpha
    );

    for (int i = 0; i < 9; i++) {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, storedTextures[i]);
    }
    glActiveTexture(GL_TEXTURE0);
}