#include "OverdrawView.hpp"
#include "Geode/cocos/CCDirector.h"
#include "Geode/cocos/base_nodes/CCNode.h"
#include "Geode/cocos/platform/win32/CCEGLView.h"
#include "Geode/cocos/platform/win32/CCGL.h"
#include "Geode/modify/Modify.hpp"
#include "HotKey.hpp"
#include "Renderer.hpp"

using namespace geode::prelude;

class PreDrawNode : public CCNode {
public:
    inline PreDrawNode(OverdrawView* view)
        : view(view) {}

    void draw() override {
        if (view) view->predraw();
    }

private:
    OverdrawView* view;
};

static OverdrawView* instance = nullptr;

OverdrawView::~OverdrawView() {
    if (preDrawNode)
        preDrawNode->release();
    if (label)
        label->retain();
    instance = nullptr;
}

bool OverdrawView::init() {
    if (instance)
        return false;
    
    preDrawNode = new PreDrawNode(this);
    preDrawNode->autorelease();
    preDrawNode->retain();

    label = CCLabelBMFont::create("", "chatFont.fnt");
    label->retain();

    // label->setAnchorPoint({0, 1});
    // label->setPosition({5.0f, CCDirector::get()->getWinSize().height - 5.0f});

    label->setAnchorPoint({0, 0});                                              
    label->setPosition({5.0f, 5.0f});
    label->setScale(0.7f);

    instance = this;
    return true;
}

void OverdrawView::predraw() {
    if (!enabled) return;
    
    glClearStencil(0);
    glClear(GL_STENCIL_BUFFER_BIT);

    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_ALWAYS, 0, 0);
    glStencilOp(GL_KEEP, GL_INCR, GL_INCR);
    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    stencilEnabled = true;
}

void OverdrawView::draw() {
    label->setVisible(false);
    if (!enabled) return;
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    // glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    auto currentSize = CCDirector::get()->getOpenGLView()->getWindowedSize();
    stencilBuffer.resize((usize)currentSize.width * (usize)currentSize.height);

    storeVAO();

    glReadPixels(0, 0, currentSize.width, currentSize.height, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, stencilBuffer.data());
    glDisable(GL_STENCIL_TEST);

    u32 totalPixelsDrawn = 0;
    for (auto b : stencilBuffer)
        totalPixelsDrawn += b;

    overdrawRate = (double)totalPixelsDrawn / ((double)currentSize.width * (double)currentSize.height);

    label->setString(fmt::format("Overdraw rate: {:.2f}%\n\nF4 to disable overdraw view", overdrawRate * 100.0f).c_str());
    label->setVisible(true);

    if (stencilTexture != 0)
        glDeleteTextures(1, &stencilTexture);

    glGenTextures(1, &stencilTexture);
    ccGLBindTexture2D(stencilTexture);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RED,
        currentSize.width,
        currentSize.height,
        0,
        GL_RED,
        GL_UNSIGNED_BYTE,
        stencilBuffer.data()
    );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    if (!shader)
        shader = Shader::create("fullscreen.vert", "overdrawView.frag");

    if (!shader)
        return;

    shader->use();
    shader->setTexture(shader->location("u_texture"), 0, stencilTexture);

    ccGLBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    drawFullscreenQuad();

    restoreVAO();
}

OverdrawView* OverdrawView::get() {
    return instance;
}

DECLARE_HOTKEY(KEY_F4, {
    if (instance)
        instance->setEnabled(!instance->isEnabled());
})