#include "ObjectBatchNode.hpp"
#include "Renderer.hpp"

using namespace geode::prelude;

bool ObjectBatchNode::init(SpriteSheet spriteSheet) {
    this->spriteSheet = spriteSheet;
    batch.setSpriteSheetFilter(spriteSheet);
    spriteSheetTexture = renderer.getSpriteSheetTexture(spriteSheet);
    return spriteSheetTexture != nullptr;
}

void ObjectBatchNode::draw() {
    auto shader = renderer.prepareDraw();

    shader->setTexture("u_spriteSheet", 0, spriteSheetTexture);

    batch.draw();

    renderer.finishDraw();
}