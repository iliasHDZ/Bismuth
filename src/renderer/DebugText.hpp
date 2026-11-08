#pragma once

#include <common.hpp>

class DebugText : public cocos2d::CCNode {
public:
    CREATE_FUNC(DebugText);

    void setText(const std::string& str);

protected:
    bool init() override;

private:
    cocos2d::CCLayerColor* backRectangle;
    cocos2d::CCLabelBMFont* label;
};