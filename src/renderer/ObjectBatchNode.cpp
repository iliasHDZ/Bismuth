#include "ObjectBatchNode.hpp"
#include "Renderer.hpp"
#include <profiler.hpp>

using namespace geode::prelude;

void ObjectBatchNode::draw() {
    Renderer::get()->getObjectBatch().draw(layerId);
}