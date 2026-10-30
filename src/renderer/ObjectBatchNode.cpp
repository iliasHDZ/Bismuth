#include "ObjectBatchNode.hpp"
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
    auto renderer = Renderer::get();

    if (!renderer->getObjectBatch().shouldDraw(layerId))
        return;

    PROFILER_PUSH("ObjectBatchNode::draw");

    auto shader = renderer->prepareDraw();
    ccGLBindTexture2D(spriteSheetTexture->getName());

    glEnable(GL_BLEND);
    if (layerId.blending || layerId.spriteSheet == SpriteSheet::GLOW)
        ccGLBlendFunc(GL_SRC_ALPHA, GL_ONE);
    else
        ccGLBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    renderer->getObjectBatch().draw(layerId);

    renderer->finishDraw();
    PROFILER_POP();
}