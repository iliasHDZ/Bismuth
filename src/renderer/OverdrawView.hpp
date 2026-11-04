#pragma once

#include "Shader.hpp"
#include <common.hpp>

class Renderer;

class OverdrawView : public cocos2d::CCNode {
public:
    ~OverdrawView();

    inline void setEnabled(bool enabled) {
        this->enabled = enabled;
        if (!enabled)
            stencilBuffer.clear();
        else if (!initialized)
            init();
    }

    inline bool isEnabled() { return enabled; }

    inline cocos2d::CCNode* getPreDrawNode() const { return preDrawNode; }

    inline cocos2d::CCLabelBMFont* getText() const { return label; }

    bool init() override;

public:
    CREATE_FUNC(OverdrawView);

    static OverdrawView* get();

private:
    void predraw();

    void draw() override;

    friend class PreDrawNode;

private:
    bool enabled = false;
    bool stencilEnabled = false;
    bool initialized = false;

    cocos2d::CCNode* preDrawNode;
    cocos2d::CCLabelBMFont* label;

    float overdrawRate = 0.0f;

    u32 stencilTexture = 0;

    std::vector<u8> stencilBuffer;

    UPtr<Shader> shader = nullptr;
};