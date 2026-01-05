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
        Iterator(ObjectSorter& sorter, bool includeGlow = false);

        inline bool isEnd() const { return layerIndex >= sorter.layers.size(); }

        // Maybe make this inline instead?
        void next();

        inline GameObject* get() const { return sorter.layers[layerIndex].objects[objectIndex]; }

        inline ObjectBatchLayer& getLayer() const { return sorter.layers[layerIndex]; }

    private:
        void skipLayers();

        inline bool isValidLayer() {
            if (sorter.layers[layerIndex].objects.size() == 0)
                return false;
            if (!includeGlow && sorter.layers[layerIndex].sheet == SpriteSheet::GLOW)
                return false;
            return true;
        }

    private:
        bool includeGlow;
        ObjectSorter& sorter;
        u32 layerIndex = 0;
        u32 objectIndex = 0;
    };

    inline Iterator iterator(bool includeGlow = false) {
        return Iterator(*this, includeGlow);
    }

private:
    std::vector<ObjectBatchLayer> layers;
};