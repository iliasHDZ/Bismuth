#pragma once
#include <Geode/Geode.hpp>

class decomp_PlayLayer : public PlayLayer {
public:
    void optimized_updateVisibility(float delta);
};