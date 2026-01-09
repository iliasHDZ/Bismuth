#include "OverdrawView.hpp"
#include "Geode/cocos/CCDirector.h"
#include "Geode/cocos/base_nodes/CCNode.h"
#include "Geode/cocos/platform/win32/CCEGLView.h"
#include "Geode/cocos/platform/win32/CCGL.h"
#include "Geode/cocos/sprite_nodes/CCSpriteBatchNode.h"
#include "Geode/modify/Modify.hpp"
#include "Renderer.hpp"

using namespace geode::prelude;

static OverdrawView* instance = nullptr;

class PreDrawNode : public CCNode {
public:
    void draw() override {
        if (instance) instance->predraw();
    }
};

class PostDrawNode : public CCNode {
public:
    void draw() override {
        if (instance) {
            instance->postdraw();
            instance->finish();
        }
    }
};

OverdrawView::OverdrawView(Renderer& renderer)
    : renderer(renderer)
{
    instance = this;
}

OverdrawView::~OverdrawView() {
    instance = nullptr;

    if (shader)
        Shader::destroy(shader);

    if (stencilTexture != 0)
        glDeleteTextures(1, &stencilTexture);
}

void OverdrawView::init() {
    initialized = true;
    
    Ref<PreDrawNode>  prenode  = new PreDrawNode();
    Ref<PostDrawNode> postnode = new PostDrawNode();
    
    prenode->autorelease();
    postnode->autorelease();

    renderer.getPlayLayer()->m_objectLayer->addChild(prenode, -10000);
    renderer.getPlayLayer()->m_objectLayer->addChild(postnode, 10000);
}

void OverdrawView::predraw() {
    if (!enabled) return;
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_ALWAYS, 0, 0);
    glStencilOp(GL_KEEP, GL_INCR, GL_INCR);
    stencilEnabled = true;
}

void OverdrawView::postdraw() {
    if (!enabled) return;

    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
}

void OverdrawView::clear() {
    if (!enabled) return;
    
    glClearStencil(0);
    glClear(GL_STENCIL_BUFFER_BIT);
}

void OverdrawView::finish() {
    if (!enabled) return;
    
    auto currentSize = CCDirector::get()->getOpenGLView()->getWindowedSize();
    stencilBuffer.resize((u32)currentSize.width * (u32)currentSize.height);

    storeGLStates();

    glReadPixels(0, 0, currentSize.width, currentSize.height, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, stencilBuffer.data());
    glDisable(GL_STENCIL_TEST);

    u32 totalPixelsDrawn = 0;
    for (auto b : stencilBuffer)
        totalPixelsDrawn += b;

    overdrawRate = (double)totalPixelsDrawn / ((double)currentSize.width * (double)currentSize.height);

    if (stencilTexture != 0)
        glDeleteTextures(1, &stencilTexture);

    glGenTextures(1, &stencilTexture);
    glBindTexture(GL_TEXTURE_2D, stencilTexture);
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

    drawFullscreenQuad();

    restoreGLStates();
}

std::string OverdrawView::getDebugText() {
    if (!enabled) return "";

    std::string text = "Overdraw view is enabled, press F4 to hide this screen\n";
    text += fmt::format("Overdraw rate: {:.2f}%\n\n", overdrawRate * 100.f);
    return text;
}

#include <Geode/modify/CCDirector.hpp>
class $modify(OverdrawViewCCDirector, CCDirector) {
    void drawScene() {
        if (instance)
            instance->clear();

        CCDirector::drawScene();
    }
};

#include <Geode/modify/CCKeyboardDispatcher.hpp>
class $modify(OverdrawViewCCKeyboardDispatcher, CCKeyboardDispatcher) {
    bool dispatchKeyboardMSG(enumKeyCodes key, bool keyDown, bool isKeyRepeat) {
        if (instance) {
            if (keyDown && key == KEY_F4 && !isKeyRepeat)
                instance->setEnabled(!instance->isEnabled());
        }

        return CCKeyboardDispatcher::dispatchKeyboardMSG(key, keyDown, isKeyRepeat);
    }
};