#include "Renderer.hpp" 
#include "ObjectSorter.hpp"

Renderer::~Renderer() { terminate(); }

bool Renderer::init(PlayLayer* layer) {
    this->layer = layer;

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
    CCObject* obj;
    CCARRAY_FOREACH(layer->m_objects, obj) {
        sorter.addGameObject((GameObject*)obj);
    }
    sorter.finalizeSorting();

    for (auto it = sorter.iterator(); !it.isEnd(); it.next())
        objectBatch.reserveForGameObject(it.get());
    objectBatch.allocateReservations();
    for (auto it = sorter.iterator(); !it.isEnd(); it.next())
        objectBatch.writeGameObject(it.get());
    objectBatch.finishWriting();

    shader = Shader::create("object.vert", "object.frag");
    if (!shader)
        return false;

    CCARRAY_FOREACH(layer->m_batchNodes, obj) {
        ((CCNode*)obj)->setVisible(false);
    }

    log::info("OpenGL Version: {}", (const char*)glGetString(GL_VERSION));
    return true;
}

void Renderer::terminate() {
    if (shader)
        Shader::destroy(shader);
    shader = nullptr;
}

void Renderer::draw() {
    glBeginQuery(GL_TIME_ELAPSED, 50);

    storeGLStates();

    shader->use();
    objectBatch.bind();

    kmMat4 matrixP;
	kmMat4 matrixMV;
	kmMat4 matrixMVP;

    glBlendFunc(CC_BLEND_SRC, CC_BLEND_DST);
	
	kmGLGetMatrix(KM_GL_PROJECTION, &matrixP);
	kmGLGetMatrix(KM_GL_MODELVIEW, &matrixMV);
	
	kmMat4Multiply(&matrixMVP, &matrixP, &matrixMV);

    shader->setMatrix4("u_mvp", matrixMVP.mat);

    i32 textureId = 0;
    for (i32 i = 0; i < (i32)SpriteSheet::COUNT; i++) {
        if (spriteSheets[i] == nullptr)
            continue;
        std::string name = "u_textures[" + std::to_string(i) + "]";
        shader->setTexture(name.c_str(), textureId, spriteSheets[i]);
        textureId++;
    }

    glDrawElements(GL_TRIANGLES, objectBatch.indexCount(), GL_UNSIGNED_INT, 0);

    restoreGLStates();
    
    i64 gpuTime = 0;
    glEndQuery(GL_TIME_ELAPSED);
    glGetQueryObjecti64v(50, GL_QUERY_RESULT, &gpuTime);

    // log::info("Rendering time: {}ms", (float)(gpuTime) / 1000000.0);
}

void Renderer::update(float dt) {
    auto parent = getParent();
}

static WeakRef<Renderer> currentRenderer;

Ref<Renderer> Renderer::create(PlayLayer* layer) {
    Ref<Renderer> ren = new Renderer();
    if (ren && ren->init(layer))
        ren->autorelease();
    else
        CC_SAFE_DELETE(ren);
    
    currentRenderer = ren;
    return ren;
}

Ref<Renderer> Renderer::get() {
    return currentRenderer.lock();
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
        if (!renderer)
            return;
        batchLayer->addChild(renderer);

        return;
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