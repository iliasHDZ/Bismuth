#include "Renderer.hpp"
#include "Geode/cocos/CCDirector.h"
#include "Geode/cocos/kazmath/include/kazmath/mat4.h"
#include "Geode/cocos/platform/win32/CCGL.h"
#include "Geode/cocos/robtop/keyboard_dispatcher/CCKeyboardDelegate.h"
#include "Geode/cocos/sprite_nodes/CCSpriteBatchNode.h"
#include "GroupManager.hpp"
#include "HotKey.hpp"
#include "ObjectBatchNode.hpp"
#include "SpriteMeshDictionary.hpp"
#include "ccTypes.h"
#include "common.hpp"
#include "glm/fwd.hpp"
#include <Geode/Enums.hpp>
#include <Geode/binding/GJBaseGameLayer.hpp>
#include <Geode/binding/GameObject.hpp>
#include <Geode/binding/RingObject.hpp>
#include "profiler.hpp"
#include "ObjectSorter.hpp"

using namespace geode::prelude;

static Renderer* currentRenderer;

PackedHSV packHSV(const ccHSVValue& hsv) {
    PackedHSV ret;

    ret.hueAndFlags = glm::packHalf2x16({hsv.h, 0}) & 0xffff;

    if (hsv.absoluteSaturation) ret.hueAndFlags |= HSV_SAT_ADD;
    if (hsv.absoluteBrightness) ret.hueAndFlags |= HSV_VAL_ADD;

    ret.satAndVal = glm::packHalf2x16({hsv.s, hsv.v});

    return ret;
}

Renderer::~Renderer() { terminate(); }

class FinishNode : public CCNode {
public:
    void draw() {
        if (currentRenderer)
            currentRenderer->finish();        
    }
};

bool Renderer::init(PlayLayer* layer, CCArray* levelObjects) {
    if (currentRenderer)
        return false;
    currentRenderer = this;
    
    if (!isOpenGLAtLeastVersion(4, 3)) {
        Notification::create(
            "Your device doesn't support the required OpenGL 4.3\nPlease update your drivers",
            NotificationIcon::Error,
            5.0f
        )->show();
        log::error("Device doesn't support OpenGL 4.3. OpenGL Version: {}", (const char*)glGetString(GL_VERSION));
        return false;
    }

    if (!hasLoadedNewerOpenGLFunctions())
        loadNewerOpenGLFunctions();

    this->layer = layer;
    
    auto size = CCDirector::get()->getWinSize();

    if (!Mod::get()->getSettingValue<bool>("enabled"))
        return false;

    debugToolsEnabled = Mod::get()->getSettingValue<bool>("debug_tools");

    log::info("OpenGL Version: {}", (const char*)glGetString(GL_VERSION));

    const char* fontTexture = GameManager::get()->getFontTexture(GameManager::get()->m_loadedFont);

    auto tcache = cocos2d::CCTextureCache::get();
    // TODO: Add text sheet
    spriteSheets[(i32)SpriteSheet::GAME_1]   = tcache->addImage("GJ_GameSheet.png", false);
    spriteSheets[(i32)SpriteSheet::GAME_2]   = tcache->addImage("GJ_GameSheet02.png", false);
    spriteSheets[(i32)SpriteSheet::PARTICLE] = tcache->addImage("GJ_ParticleSheet.png", false);
    spriteSheets[(i32)SpriteSheet::GLOW]     = tcache->addImage("GJ_GameSheetGlow.png", false);
    spriteSheets[(i32)SpriteSheet::FIRE]     = tcache->addImage("FireSheet_01.png", false);
    spriteSheets[(i32)SpriteSheet::PIXEL]    = tcache->addImage("PixelSheet_01.png", false);
    spriteSheets[(i32)SpriteSheet::TEXT]     = tcache->addImage(fontTexture, false);

    SpriteMeshDictionary::loadFromFile("spriteMeshes.json");

    groupManager.initWithObjects(levelObjects);

    ObjectSorter sorter;
    sorter.initForGameLayer(layer);

    std::vector<GameObject*> objects;

    log::info("Level contains {} object(s)", levelObjects->count());
    log::info("Sorting objects...");
    for (auto object : CCArrayExt<GameObject*>(levelObjects)) {
        if (object == layer->m_anticheatSpike) {
            DEBUG_LOG("- anti-cheat spike");
            continue;
        }

        if (object->isTrigger() || object->m_isHide || object->m_objectID == GUIDE_OBJECT_ID)
            continue;

        if (object == layer->m_endPortal)
            continue;

        DEBUG_LOG("- {}, id: {}", (void*)object, object->m_objectID);
        DEBUG_LOG("  - rotation: {}, {}", object->m_startRotationX, object->m_startRotationY);
        DEBUG_LOG("  - scale: {}, {}", object->m_startScaleX, object->m_startScaleY);
        DEBUG_LOG("  - flip: {}, {}", object->m_startFlipX, object->m_startFlipX);
        DEBUG_LOG("  - zlayer: {}", zLayerAsString(object->getObjectZLayer()));
        DEBUG_LOG("  - zorder: {}", object->getObjectZOrder());
        DEBUG_LOG("  - blending: {}", object->m_baseOrDetailBlending);
        DEBUG_LOG("  - spritesheet: {}", object->getParentMode());
        DEBUG_LOG("  - glowColorIsLBG: {}", object->m_glowColorIsLBG);
        DEBUG_LOG("  - customGlowColor: {}", object->m_customGlowColor);
        DEBUG_LOG("  - opacityMod: {}", object->m_opacityMod);
        DEBUG_LOG("  - isDecoration2: {}", object->m_isDecoration2);
        DEBUG_LOG("  - colorSprite: {}", object->m_colorSprite);
        DEBUG_LOG("  - glowSprite: {}", object->m_glowSprite);
        DEBUG_LOG("  - isObjectBlack: {}", object->m_isObjectBlack);
        DEBUG_LOG("  - isObjectChildrenBlack: {}", object->m_isColorSpriteBlack);
        DEBUG_LOG("  - isObjectChildrenBlack: {}", object->m_isColorSpriteBlack);
        DEBUG_LOG("  - blackChildOpacity: {}", object->m_blackChildOpacity);
        DEBUG_LOG("  - blackChildOpacityLocked: {}", object->m_blackChildOpacityLocked);
        DEBUG_LOG("  - activeDetailColorID: {}", object->m_activeDetailColorID);
        DEBUG_LOG("  - childCount: {}", object->getChildrenCount());
        if (object->m_baseColor && object->m_baseColor->m_usesHSV)
            DEBUG_LOG("  - baseHSV: {} {} {} {} {}", object->m_baseColor->m_hsv.h, object->m_baseColor->m_hsv.s, object->m_baseColor->m_hsv.v, object->m_baseColor->m_hsv.absoluteSaturation, object->m_baseColor->m_hsv.absoluteBrightness);
        if (object->m_detailColor && object->m_detailColor->m_usesHSV)
            DEBUG_LOG("  - detailHSV: {} {} {} {} {}", object->m_detailColor->m_hsv.h, object->m_detailColor->m_hsv.s, object->m_detailColor->m_hsv.v, object->m_detailColor->m_hsv.absoluteSaturation, object->m_detailColor->m_hsv.absoluteBrightness);
        if (object->getHasRotateAction())
            DEBUG_LOG("  - rotationDelta: {}", ((EnhancedGameObject*)object)->m_rotationDelta);

        sorter.addGameObject(object);
    }

    for (auto it = sorter.iterator(); !it.isEnd(); it.next())
        objects.push_back(it.get());

    log::info("Generating static rendering buffer....");

    renderedGameObjectCount = objects.size();
    
    generateStaticRenderingBuffer(objects);

    u32 groupCombCount = groupManager.getGroupCombinationCount();
    log::info("{} group combinations detected", groupCombCount);

    log::info("Generating vertex buffer...");

    for (auto& value : colorChannelsIsBlending)
        value = false;

    fetchBlendingStates();

    for (auto object : objects)
        objectBatch.writeGameObject(object);
    objectBatch.finishWriting();
    generateBatchNodes();

    log::info("Sprite count: {}", objectBatch.getSpriteCount());

    std::map<std::string, std::string> shaderMacroVariables;

    log::info("Compiling shaders...");
    shader = Shader::create("objectDrawing.vert", "objectDrawing.frag", shaderMacroVariables);
    if (!shader)
        return false;

    log::info("Compiled objectDrawing shader");

    cullingComputeShader = Shader::createCompute("objectCulling.comp", shaderMacroVariables);
    if (!cullingComputeShader)
        return false;

    log::info("Compiled objectCulling shader");

    colorChannelBufferObject = Buffer::createDynamicDraw("ColorChannelBuffer", sizeof(ColorChannelBuffer));
    if (!colorChannelBufferObject)
        return false;
    colorChannelBuffer = (ColorChannelBuffer*)colorChannelBufferObject->mapWriteOnly();

    gameStateBuffer = Buffer::createDynamicDraw("GameStateBuffer", sizeof(GameStateBuffer));
    if (!gameStateBuffer)
        return false;

    debugText = DebugText::create();
    layer->addChild(debugText, 1000);

    if (useOptimizations()) {
        for (auto object : CCArrayExt<GameObject*, false>(layer->m_objects)) {
            if (object != layer->m_endPortal)
                object->deactivateObject(true);
        }
    }

    setZOrder(-99999);
    setEnabled(true);

    rendererStartTime = getTime();

    layer->m_objectLayer->addChild(new FinishNode, 99999);

    reset();

    if (debugToolsEnabled) {
        Notification::create(
            "Debug tools removes ALL optimizations!\nDisable it for better performance.",
            NotificationIcon::Warning,
            5.0f
        )->show();
    }

    log::info("Renderer initialized");
    return true;
}

void Renderer::generateBatchNodes() {
    for (const auto& id : objectBatch.getUsedLayerIds()) {
        auto batchNode = ObjectBatchNode::create(id);
        if (batchNode) {
            auto node = getSpriteBatchNodeWithLayerId(id);

            if (!node)
                continue;

            vanillaBatchNodes.insert(node);

            layer->m_objectLayer->addChild(batchNode, node->getZOrder());
            batchNodes.push_back(batchNode);
        }
    }
}

void Renderer::terminate() {
    currentRenderer = nullptr;
    log::info("Renderer terminated");
}

void Renderer::prepareShaderUniforms() {
    kmMat4 matrixP;
	kmMat4 matrixMV;
	kmMat4 matrixMVP;
	
	kmGLGetMatrix(KM_GL_PROJECTION, &matrixP);
	kmGLGetMatrix(KM_GL_MODELVIEW, &matrixMV);
	
	kmMat4Multiply(&matrixMVP, &matrixP, &matrixMV);

    (kmMat4&)gameState.u_mvp = matrixMVP;
    gameState.u_timer = gameTimer;
    gameState.u_cameraViewSize = glm::vec2(layer->m_cameraWidth, layer->m_cameraHeight);
    auto winsize = CCDirector::get()->getWinSize();
    gameState.u_winSize = glm::vec2(winsize.width, winsize.height);
    gameState.u_screenRight = CCDirector::get()->getScreenRight();
    gameState.u_cameraUnzoomedX = layer->m_cameraUnzoomedX;
    ccColor3B specialLightBG = GameToolbox::transformColor(layer->m_effectManager->activeColorForIndex(COLOR_CHANNEL_BG), 0.0, -0.2, 0.2);
    gameState.u_specialLightBGColor = ccColor3BToGLM(specialLightBG);

    auto winSize = CCDirector::get()->getWinSize();

    vec2 cameraBottomLeft  = ccPointToGLM(layer->m_objectLayer->convertToNodeSpace({0, 0}));
    vec2 cameraBottomRight = ccPointToGLM(layer->m_objectLayer->convertToNodeSpace({winSize.width, 0}));
    vec2 cameraTopLeft     = ccPointToGLM(layer->m_objectLayer->convertToNodeSpace({0, winSize.height}));

    vec2 cameraRight = cameraBottomRight - cameraBottomLeft;
    vec2 cameraUp    = cameraTopLeft     - cameraBottomLeft;

    gameState.u_cameraPosition = cameraBottomLeft;

    gameState.u_cameraRight = glm::normalize(cameraRight);
    gameState.u_cameraUp    = glm::normalize(cameraUp);

    gameState.u_cameraSize = glm::vec2 { glm::length(cameraRight), glm::length(cameraUp) };

    gameState.u_gameStateFlags = 0;
    if (layer->m_player1->m_isDead)
        gameState.u_gameStateFlags |= GAME_STATE_IS_PLAYER_DEAD;

    if (layer->m_skipAudioStep)
        gameState.u_audioScale = FMODAudioEngine::sharedEngine()->getMeteringValue();
    else
        gameState.u_audioScale = layer->m_audioEffectsLayer->m_audioScale;
    
    if (layer->m_isSilent || (layer->m_isPracticeMode && !layer->m_practiceMusicSync))
        gameState.u_audioScale = 0.5;

    gameStateBuffer->write(&gameState, sizeof(gameState));
}

void Renderer::prepareColorChannelBuffer() {
    PROFILER_SCOPE("Renderer::prepareColorChannelBuffer");

    auto effectManager = layer->m_effectManager;

    for (usize i = 0; i < effectManager->m_colorActionSpriteVector.size(); i++) {
        auto sprite = effectManager->m_colorActionSpriteVector[i];
        if (sprite == nullptr)
            continue;

        auto id = sprite->m_colorID;

        auto& channelColor = colorChannelBuffer->u_channelColors[id];
        channelColor.r = sprite->m_copyColor.r;
        channelColor.g = sprite->m_copyColor.g;
        channelColor.b = sprite->m_copyColor.b;
        channelColor.a = (u8)sprite->m_opacity;
    }

    colorChannelBuffer->u_channelColors[COLOR_CHANNEL_BLACK] = { 0, 0, 0, 255 };
    colorChannelBufferObject->flush(0, colorChannelBufferObject->getSize());
}

void Renderer::generateStaticRenderingBuffer(std::vector<GameObject*>& objects) {
    std::vector<ObjectInfo> objectInfos;
    objectInfos.resize(renderedGameObjectCount);

    usize index = 0;

    for (auto object : objects) {
        auto objectInfo = &objectInfos[index];

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

        objectInfo->baseColorChannelId = object->m_activeMainColorID;
        objectInfo->detailColorChannelId = object->m_activeDetailColorID;

        if (object->m_baseColor && object->m_baseColor->m_usesHSV) {
            objectInfo->flags |= OBJECT_FLAG_HAS_BASE_HSV;
            objectInfo->baseHSV = packHSV(object->m_baseColor->m_hsv);
        }

        if (object->m_detailColor && object->m_detailColor->m_usesHSV) {
            objectInfo->flags |= OBJECT_FLAG_HAS_DETAIL_HSV;
            objectInfo->detailHSV = packHSV(object->m_detailColor->m_hsv);
        }

        float opacity = (object->m_opacityMod2 > 0.0) ? object->m_opacityMod2 : 1.0;
        float blackChildOpacity = 1.0f;

        if (object->m_hasCustomChild && !object->m_blackChildOpacityLocked && object->m_blackChildOpacity > 0.0f)
            blackChildOpacity = object->m_blackChildOpacity;

        objectInfo->opacityValues = glm::packUnorm4x8({opacity, blackChildOpacity, 0, 0});

        if (object->m_isInvisibleBlock) objectInfo->flags |= OBJECT_FLAG_IS_INVISIBLE_BLOCK;
        if (object->m_customGlowColor)  objectInfo->flags |= OBJECT_FLAG_SPECIAL_GLOW_COLOR;
        if (object->m_objectType == GameObjectType::Solid)
            objectInfo->flags |= OBJECT_FLAG_IS_STATIC_OBJECT;
        objectInfo->fadeMargin = object->m_fadeMargin;

        objectIndicies[object] = index;
        index++;
    }

    objectInfoBuffer = Buffer::createStaticDraw("ObjectInfoBuffer", objectInfos.data(), objectInfos.size() * sizeof(ObjectInfo));
};

void Renderer::draw() {
    PROFILER_SCOPE("Renderer::draw");
    fetchBlendingStates();

    u64 prevTime = getTime();
    prepareShaderUniforms();
    if (!isPaused()) {
        prepareColorChannelBuffer();
        groupManager.updateGroupStateBuffer();

        objectBatch.predraw();
    }

    renderingTimer.start();
}

void Renderer::finish() {
    renderingTimer.stop();

    if (debugText->isVisible())
        updateDebugText();
}

void Renderer::updateDebugText() {
    std::string text = "";

    if (!enabled) {
        if (differenceModeEnabled) {
            text += "Difference mode is enabled (F9 to disable)\n";
        } else
            text += "Bismuth renderer is disabled (F8 to enable)\n";
    } else {
        if (debugTextEnabled) {
            auto screenSize = CCDirector::get()->getWinSizeInPixels();

            text += fmt::format("Bismuth renderer {}\n", Mod::get()->getVersion().toVString());
            text += fmt::format("OpenGL {}\n", (const char*)glGetString(GL_VERSION));
            text += fmt::format("{}\n", (const char*)glGetString(GL_RENDERER));
            text += "\n";
            text += "Press F3 to hide this screen\n";
        } else if (differenceModeEnabled)
            text += "_\n";
    }

    debugText->setText(text);
}

void Renderer::bindCullingComputeShaderBuffers() {
    gameStateBuffer->bindAsUniformBuffer(GAME_STATE_BUFFER_BINDING);
    objectInfoBuffer->bindAsStorageBuffer(OBJECT_INFO_BUFFER_BINDING);
    colorChannelBufferObject->bindAsStorageBuffer(COLOR_CHANNEL_BUFFER_BINDING);
    groupManager.bindGroupStateBuffers();
}

void Renderer::fetchBlendingStates() {
    for (int i = 0; i < colorChannelsIsBlending.size(); i++) {
        auto vec = layer->m_effectManager->m_colorActionVector[i];
        if (vec)
            colorChannelsIsBlending[i] = vec->m_blending;
    }

    colorChannelsIsBlending[COLOR_CHANNEL_P1]  = true;
    colorChannelsIsBlending[COLOR_CHANNEL_P2]  = true;
    colorChannelsIsBlending[COLOR_CHANNEL_LBG] = true;
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

    auto& gstate = layer->m_gameState;
    cameraCenterPos = ccPointToGLM(gstate.m_cameraPosition2 + CCPoint(layer->m_cameraWidth * 0.5, layer->m_cameraHeight * 0.5));
}

CCSpriteBatchNode* Renderer::getSpriteBatchNodeWithLayerId(LayerKey id) {
    CCNode* node = layer->parentForZLayer((i32)id.zlayer, id.blending, (i32)id.spriteSheet, false);
    
    if (!node || id.spriteSheet == SpriteSheet::SPECIAL || id.spriteSheet == SpriteSheet::_UNK)
        return nullptr;

    return (CCSpriteBatchNode*)node;
}

Ref<Renderer> Renderer::create(PlayLayer* layer, CCArray* objects) {
    auto ren = new Renderer;

    if (ren->init(layer, objects)) {
        ren->autorelease();
        return ren;
    }
    
    log::error("Failed to initialize renderer");
    delete ren;
    return nullptr;
}

Ref<Renderer> Renderer::get() {
    return currentRenderer;
}

bool Renderer::useOptimizations() {
    return !isDebugToolsEnabled();
}

void Renderer::setEnabled(bool enabled) {
    if (useOptimizations() && !enabled)
        return;

    this->enabled = enabled;
    this->setVisible(enabled);
    for (auto node : batchNodes)
        node->setVisible(enabled);
    for (auto batch : vanillaBatchNodes)
        batch->setVisible(!enabled);

    if (useOptimizations()) {
        // For checkpoints
        layer->m_game2LayerT1->setVisible(true);
        // For end portal
        layer->m_gameBlendingLayerT1->setVisible(true);
        layer->m_gameLayerT1->setVisible(true);
    }

    updateDebugText();
}

void Renderer::reset() {
    groupManager.resetGroupStates();
}

DECLARE_HOTKEY(KEY_F3, {
    if (currentRenderer)
        currentRenderer->toggleDebugText();
})

DECLARE_HOTKEY(KEY_F8, {
    if (currentRenderer)
        currentRenderer->setEnabled(!currentRenderer->isEnabled());
})

DECLARE_HOTKEY(KEY_F9, {
    if (currentRenderer)
        currentRenderer->setDifferenceModeEnabled(!currentRenderer->isDifferenceModeEnabled());
})