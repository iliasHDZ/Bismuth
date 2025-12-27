#include "ObjectSorter.hpp"

static std::array<ZLayer, 9> zlayers = {
    ZLayer::B5,
    ZLayer::B4,
    ZLayer::B3,
    ZLayer::B2,
    ZLayer::B1,
    ZLayer::T1,
    ZLayer::T2,
    ZLayer::T3,
    ZLayer::T4
};

void ObjectSorter::initForGameLayer(GJBaseGameLayer* layer) {
    layers.clear();

    for (ZLayer zlayer : zlayers) {
        for (auto sheet = (SpriteSheet)0; sheet < SpriteSheet::COUNT; sheet = (SpriteSheet)((i32)sheet + 1)) {
            tryAddLayer(layer, zlayer, false, sheet);
            tryAddLayer(layer, zlayer, true, sheet);
        }
    }

    std::sort(
        layers.begin(),
        layers.end(),
        [](const ObjectBatchLayer& a, const ObjectBatchLayer& b) {
            return a.node->getZOrder() < b.node->getZOrder();
        }
    );
}

void ObjectSorter::addGameObject(GameObject* object) {
    auto layer = getLayer(object->getObjectZLayer(), object->m_baseOrDetailBlending, (SpriteSheet)object->getParentMode());

    if (layer)
        layer->objects.push_back(object);
}

void ObjectSorter::finalizeSorting() {
    for (auto& layer : layers) {
        std::sort(
            layer.objects.begin(),
            layer.objects.end(),
            [](GameObject* a, GameObject* b) {
                return a->getObjectZOrder() < b->getObjectZOrder();
            }
        );
    }
}

void ObjectSorter::tryAddLayer(GJBaseGameLayer* layer, ZLayer zlayer, bool blending, SpriteSheet sheet) {
    // TODO: Fix UI objects
    cocos2d::CCNode* node = layer->parentForZLayer((i32)zlayer, blending, (i32)sheet, false);

    if (!node || layer->m_batchNodes->indexOfObject(node) == UINT_MAX)
        return;

    layers.push_back({
        zlayer,
        blending,
        sheet,
        (cocos2d::CCSpriteBatchNode*)node
    });
}

ObjectBatchLayer* ObjectSorter::getLayer(ZLayer zLayer, bool blending, SpriteSheet sheet) {
    if ((i32)zLayer % 2 == 0) {
        zLayer = (ZLayer)((i32)zLayer - 1);
        
        if (zLayer < ZLayer::B5)
            zLayer = ZLayer::B5;
    }

    // TODO: Probably use a faster way to find the layer
    for (auto& layer : layers) {
        if (layer.zLayer == zLayer && layer.blending == blending && layer.sheet == sheet)
            return &layer;
    }

    return nullptr;
}

ObjectSorter::Iterator::Iterator(ObjectSorter& sorter)
    : sorter(sorter)
{
    while (layerIndex < sorter.layers.size() && sorter.layers[layerIndex].objects.size() == 0)
        layerIndex++;
};

void ObjectSorter::Iterator::next() {
    objectIndex++;
    if (objectIndex >= sorter.layers[layerIndex].objects.size()) {
        objectIndex = 0;
        layerIndex++;
        while (layerIndex < sorter.layers.size() && sorter.layers[layerIndex].objects.size() == 0)
            layerIndex++;
    }
}