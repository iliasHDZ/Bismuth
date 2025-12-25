#include "Renderer.hpp" 
#include "ObjectSorter.hpp"

using namespace cocos2d;

static Renderer* currentRenderer;

Renderer::~Renderer() { terminate(); }

bool Renderer::init(PlayLayer* layer) {
    this->layer = layer;

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
    for (int x = 0; x < layer->m_sections.size(); x++) {
        if (layer->m_sections[x] == nullptr)
            continue;
        for (int y = 0; y < layer->m_sections[x]->size(); y++) {
            auto section = layer->m_sections[x]->at(y);

            if (section == nullptr || layer->m_sectionSizes[x] == nullptr)
                continue;

            auto size = layer->m_sectionSizes[x]->at(y);
            
            for (int i = 0; i < size; i++) {
                sorter.addGameObject(section->at(i));
            }
        }
    }
    sorter.finalizeSorting();

    for (auto it = sorter.iterator(); !it.isEnd(); it.next())
        objectBatch.reserveForGameObject(it.get());
    objectBatch.allocateReservations();
    for (auto it = sorter.iterator(); !it.isEnd(); it.next())
        objectBatch.writeGameObject(it.get());
    objectBatch.finishWriting();

    std::map<std::string, std::string> shaderMacroVariables;

    shaderMacroVariables["TOTAL_OBJECT_COUNT"] = "1";
    shaderMacroVariables["TOTAL_GROUP_COUNT"]  = "1";

    shader = Shader::create("object.vert", "object.frag", shaderMacroVariables);
    if (!shader)
        return false;

    CCObject* obj;
    CCARRAY_FOREACH(layer->m_batchNodes, obj) {
        ((CCNode*)obj)->setVisible(false);
    }

    drbBuffer = Buffer::createDynamicDraw(sizeof(DynamicRenderingBuffer));
    if (!drbBuffer)
        return false;

    debugText = CCLabelBMFont::create("", "chatFont.fnt");

    debugText->setAnchorPoint(CCPoint(0, 1));
    debugText->setPosition(1, CCDirector::get()->getWinSize().height - 8);
    debugText->setScale(0.5);
    debugText->setVisible(false);
    layer->addChild(debugText, 1000);

    log::info("Renderer initialized");
    return true;
}

void Renderer::terminate() {
    if (shader)
        Shader::destroy(shader);
    if (drbBuffer)
        Buffer::destroy(drbBuffer);
    shader = nullptr;

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

    shader->setMatrix4("u_mvp", matrixMVP.mat);

    /*
    i32 textureId = 0;
    for (i32 i = 0; i < (i32)SpriteSheet::COUNT; i++) {
        if (spriteSheets[i] == nullptr)
            continue;
        std::string name = "u_textures[" + std::to_string(i) + "]";
        shader->setTexture(name.c_str(), textureId, spriteSheets[i]);
        textureId++;
    }
    */
   shader->setTextureArray("u_spriteSheets", (i32)SpriteSheet::COUNT, spriteSheets);
}

void Renderer::prepareDynamicRenderingBuffer() {
    auto effectManager = layer->m_effectManager;

    for (usize i = 0; i < effectManager->m_colorActionSpriteVector.size(); i++) {
        auto sprite = effectManager->m_colorActionSpriteVector[i];
        if (sprite == nullptr)
            continue;

        auto id = sprite->m_colorID;

        drb.channelColors[id].r = sprite->m_color.r;
        drb.channelColors[id].g = sprite->m_color.g;
        drb.channelColors[id].b = sprite->m_color.b;
        drb.channelColors[id].a = (u8)sprite->m_opacity;

        bool shouldBlending = layer->shouldBlend(id);

        if (shouldBlending)
            log::info("{} BLENDING", id);

        if (shouldBlending)
            drb.colorChannelBlendingBitmap[id >> 5] |= 1 << (id & 0x1f);
        else
            drb.colorChannelBlendingBitmap[id >> 5] &= ~(1 << (id & 0x1f));
    }

    drb.channelColors[COLOR_CHANNEL_BLACK] = { 0, 0, 0, 255 };

    drbBuffer->write(&drb, sizeof(DynamicRenderingBuffer));
    drbBuffer->bindAsUniformBuffer(DYNAMIC_RENDERING_BUFFER_BINDING);
}

void Renderer::draw() {
    glBeginQuery(GL_TIME_ELAPSED, 50);

    storeGLStates();

    shader->use();

    prepareShaderUniforms();
    prepareDynamicRenderingBuffer();

    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    objectBatch.draw();

    restoreGLStates();
    
    glEndQuery(GL_TIME_ELAPSED);
    glGetQueryObjecti64v(50, GL_QUERY_RESULT, &renderTime);

    if (debugText->isVisible())
        updateDebugText();
}

static std::string byteSizeToString(usize size) {
    std::string unit = "B";
    double dsize = size;
    if (dsize > 1000.0) { dsize /= 1000.0; unit = "kB"; }
    if (dsize > 1000.0) { dsize /= 1000.0; unit = "MB"; }
    return fmt::format("{:.3f}{}", dsize, unit);
}

void Renderer::updateDebugText() {
    std::string text = "";

    auto screenSize = CCDirector::get()->getWinSizeInPixels();

    text += fmt::format("Bismuth {}\n", Mod::get()->getVersion().toVString());
    text += fmt::format("OpenGL {}\n", (const char*)glGetString(GL_VERSION));
    text += fmt::format("{}\n", (const char*)glGetString(GL_RENDERER));
    text += fmt::format("Window: {}x{}\n", screenSize.width, screenSize.height);
    text += fmt::format("Render time: {}ms\n", (double)renderTime / 1000000.0);
    text += fmt::format("Vertex buffer size: {}\n", byteSizeToString(objectBatch.getQuadCount() * sizeof(ObjectQuad)));
    text += "\n";
    text += "Press F3 to hide this screen";

    debugText->setString(text.c_str());
}

void Renderer::update(float dt) {
    auto parent = getParent();
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
};

#include <Geode/modify/GJBaseGameLayer.hpp>
class $modify(RendererGJBaseGameLayer, GJBaseGameLayer) {
    void update(float dt) {
        GJBaseGameLayer::update(dt);
        auto renderer = Renderer::get();
        if (renderer)
            renderer->update(dt);
    }
};

#include <Geode/modify/CCKeyboardDispatcher.hpp>
class $modify(RendererCCKeyboardDispatcher, cocos2d::CCKeyboardDispatcher) {
    bool dispatchKeyboardMSG(cocos2d::enumKeyCodes key, bool keyDown, bool isKeyRepeat) {
        if (keyDown && key == cocos2d::KEY_F3 && !isKeyRepeat && Renderer::get())
            Renderer::get()->toggleDebugText();

        return cocos2d::CCKeyboardDispatcher::dispatchKeyboardMSG(key, keyDown, isKeyRepeat);
    }
};

static i32 storedVAO, storedVBO, storedIBO, storedProgram;
static i32 storedBlendSrcAlpha, storedBlendSrcRGB;
static i32 storedBlendDstAlpha, storedBlendDstRGB;

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
}