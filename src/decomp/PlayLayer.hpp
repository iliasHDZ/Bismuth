#pragma once
#include <Geode/Geode.hpp>

class decomp_PlayLayer : public PlayLayer {
public:
    void virtual_updateVisibility(float delta);
    
    void optimized_updateVisibility(float delta);
};