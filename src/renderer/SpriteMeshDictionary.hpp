#pragma once

#include "math/ConvexList.hpp"
#include <common.hpp>

class SpriteMeshDictionary {
public:
    static ConvexList* getSpriteMeshForSprite(cocos2d::CCSprite* sprite);

    static void loadFromFile(const fs::path& path);
};