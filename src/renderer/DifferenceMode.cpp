#include "Geode/cocos/CCDirector.h"
#include "Geode/cocos/platform/win32/CCGL.h"
#include "Renderer.hpp"
#include <Geode/binding/Standalones.hpp>
#include <cstddef>

using namespace geode::prelude;

DifferenceMode::~DifferenceMode() {
    destroyFramebuffers();

    if (fullscreenQuadBuffer)
        Buffer::destroy(fullscreenQuadBuffer);
    if (fullscreenQuadVAO)
        glDeleteVertexArrays(1, &fullscreenQuadVAO);
    if (shader)
        Shader::destroy(shader);
}

void DifferenceMode::drawSceneHook() {
    auto currentSize = CCDirector::get()->getOpenGLView()->getWindowedSize();

    if (lastSize != currentSize) {
        prepare(currentSize.width, currentSize.height);
        lastSize = currentSize;
    }

    bool prevEnabled = renderer.isEnabled();

    // storeGLStates();
    renderer.setEnabled(false);
    glBindFramebuffer(GL_FRAMEBUFFER, vanillaFramebuffer);
    glClear(GL_COLOR_BUFFER_BIT);
    kmGLPushMatrix();
    CCDirector::get()->getRunningScene()->visit();
    kmGLPopMatrix();

    renderer.setEnabled(true);
    glBindFramebuffer(GL_FRAMEBUFFER, bismuthFramebuffer);
    glClear(GL_COLOR_BUFFER_BIT);
    kmGLPushMatrix();
    CCDirector::get()->getRunningScene()->visit();
    kmGLPopMatrix();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    storeGLStates();

    shader->use();
    shader->setTexture(shader->location("u_vanillaFrame"), 0, vanillaTexture);
    shader->setTexture(shader->location("u_bismuthFrame"), 1, bismuthTexture);
    shader->setFloat("u_intensity", intensity);
    shader->setFloat("u_backdropIntensity", backdropIntensity);

    glBindVertexArray(fullscreenQuadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    restoreGLStates();

    CCDirector::get()->getOpenGLView()->swapBuffers();
}

static u32 createFramebufferTexture(u32 width, u32 height) {
    u32 texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    return texture;
}

static u32 createColorFramebuffer(u32 colorTextureAttachment) {
    u32 fb;
    glGenFramebuffers(1, &fb);
    glBindFramebuffer(GL_FRAMEBUFFER, fb);
    glBindTexture(GL_TEXTURE_2D, colorTextureAttachment);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTextureAttachment, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return fb;
}

static float fullscreenQuad[] = {
    -1.0, -1.0,
    -1.0,  1.0,
     1.0,  1.0,
    -1.0, -1.0,
     1.0,  1.0,
     1.0, -1.0
};

void DifferenceMode::prepare(u32 width, u32 height) {
    storeGLStates();

    destroyFramebuffers();

    vanillaTexture     = createFramebufferTexture(width, height);
    vanillaFramebuffer = createColorFramebuffer(vanillaTexture);
    bismuthTexture     = createFramebufferTexture(width, height);
    bismuthFramebuffer = createColorFramebuffer(bismuthTexture);

    if (!fullscreenQuadBuffer)
        fullscreenQuadBuffer = Buffer::createStaticDraw(fullscreenQuad, sizeof(fullscreenQuad));

    if (!fullscreenQuadVAO) {
        glGenVertexArrays(1, &fullscreenQuadVAO);

        glBindVertexArray(fullscreenQuadVAO);
        fullscreenQuadBuffer->bindAs(GL_ARRAY_BUFFER);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, (void*)0);
        glEnableVertexAttribArray(0);
    }

    if (!shader)
        shader = Shader::createOld("differenceMode.vert", "differenceMode.frag");

    restoreGLStates();
}

void DifferenceMode::destroyFramebuffers() {
    if (vanillaFramebuffer) glDeleteFramebuffers(1, &vanillaFramebuffer);
    if (bismuthFramebuffer) glDeleteFramebuffers(1, &bismuthFramebuffer);
    if (vanillaTexture) glDeleteTextures(1, &vanillaTexture);
    if (bismuthTexture) glDeleteTextures(1, &bismuthTexture);
    vanillaFramebuffer = 0;
    bismuthFramebuffer = 0;
    vanillaTexture = 0;
    bismuthTexture = 0;
}

#include <Geode/modify/CCDirector.hpp>
class $modify(MyCCDirector, CCDirector) {
    void drawScene() {

        DifferenceMode* diffMode = nullptr;
        auto renderer = Renderer::get();
        if (renderer)
            diffMode = renderer->getDifferenceMode();

        if (!diffMode) {
            CCDirector::drawScene();
            return;
        }

        calculateDeltaTime();
        if (!m_bPaused)
            m_pScheduler->update(m_fDeltaTime);
        if (m_pNextScene)
            setNextScene();
        m_uTotalFrames++;

        diffMode->drawSceneHook();
    }
};