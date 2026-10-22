#include "ObjectBatchNode.hpp"
#include "Geode/cocos/platform/win32/CCGL.h"
#include "Renderer.hpp"
#include <profiler.hpp>

using namespace geode::prelude;

bool ObjectBatchNode::init() {
    auto renderer = Renderer::get();
    spriteSheetTexture = renderer->getSpriteSheetTexture(layerId.spriteSheet);
    return true;
}

void ObjectBatchNode::draw() {
    if (!spriteSheetTexture)
        return;

    PROFILER_PUSH("ObjectBatchNode::draw");
    auto renderer = Renderer::get();

    auto shader = renderer->prepareDraw();
    ccGLBindTexture2D(spriteSheetTexture->getName());

    glEnable(GL_BLEND);
    if (layerId.blending)
        ccGLBlendFunc(GL_SRC_ALPHA, GL_ONE);
    else
        ccGLBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    renderer->getObjectBatch().draw(layerId);

    renderer->finishDraw();
    PROFILER_POP();
}