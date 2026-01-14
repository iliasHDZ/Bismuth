#pragma once

#include "Shader.hpp"
#include <common.hpp>

class Renderer;

class OverdrawView {
public:
    OverdrawView(Renderer& renderer);
    ~OverdrawView();

    inline void setEnabled(bool enabled) {
        this->enabled = enabled;
        if (!enabled)
            stencilBuffer.clear();
        else if (!initialized)
            init();
    }

    inline bool isEnabled() { return enabled; }

    void init();

    void predraw();

    void postdraw();

    void clear();

    void finish();

    std::string getDebugText();

private:
    Renderer& renderer;

    bool enabled = false;
    bool stencilEnabled = false;
    bool initialized = false;

    float overdrawRate = 0.0f;

    u32 stencilTexture = 0;

    std::vector<u8> stencilBuffer;

    Shader* shader = nullptr;
};