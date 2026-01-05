#pragma once

#include <common.hpp>
#include "ObjectSpriteUnpacker.hpp"

struct ObjectBatchLayer {
    ZLayer zLayer;
    bool blending;
    SpriteSheet sheet; // Also known as the 'parent mode' in the game

    cocos2d::CCSpriteBatchNode* node;
    std::vector<GameObject*> objects;
};

class ObjectSorter {
public:
    void initForGameLayer(GJBaseGameLayer* layer);

    void addGameObject(GameObject* object);

    void finalizeSorting();

private:
    void tryAddLayer(GJBaseGameLayer* layer, ZLayer zlayer, bool blending, SpriteSheet sheet);

    ObjectBatchLayer* getLayer(ZLayer zLayer, bool blending, SpriteSheet sheet);

public:
    class Iterator {
    public:
        Iterator(ObjectSorter& sorter);

        inline bool isEnd() const { return layerIndex >= sorter.layers.size(); }

        // Maybe make this inline instead?
        void next();

        inline GameObject* get() const { return sorter.layers[layerIndex].objects[objectIndex]; }

        inline ObjectBatchLayer& getLayer() const { return sorter.layers[layerIndex]; }

    private:
        ObjectSorter& sorter;
        u32 layerIndex = 0;
        u32 objectIndex = 0;
    };

    inline Iterator iterator() {
        return Iterator(*this);
    }

private:
    std::vector<ObjectBatchLayer> layers;
};