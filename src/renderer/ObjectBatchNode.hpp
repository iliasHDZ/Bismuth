#pragma once

#include <common.hpp>

#include "ObjectBatch.hpp"
#include "ObjectUtils.hpp"

class ObjectBatchNode : public cocos2d::CCNode {
public:
    inline ObjectBatchNode(const LayerKey& id)
        : layerId(id) {}

    void draw() override;

public:
    static inline Ref<ObjectBatchNode> create(const LayerKey& id) {
        auto ret = new ObjectBatchNode(id);
        ret->autorelease();
        return ret;
    }

private:
    LayerKey layerId;
};