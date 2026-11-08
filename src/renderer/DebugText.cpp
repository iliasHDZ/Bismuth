#include "DebugText.hpp"

using namespace geode::prelude;

static constexpr CCPoint padding = {3, 3};

void DebugText::setText(const std::string& str) {
    if (str == "") {
        setVisible(false);
        return;
    }

    setVisible(true);

    label->setString(str.c_str());
    backRectangle->setContentSize(label->getScaledContentSize() + padding * 2);
}

bool DebugText::init() {
    backRectangle = CCLayerColor::create();
    label = CCLabelBMFont::create("", "chatFont.fnt");

    setAnchorPoint({0, 1});
    setPosition({0, CCDirector::get()->getWinSize().height});

    backRectangle->ignoreAnchorPointForPosition(false);
    backRectangle->setAnchorPoint({0, 1});
    backRectangle->setColor({0, 0, 0});
    backRectangle->setOpacity(128);
    addChild(backRectangle, 500);

    label->setAnchorPoint({0, 1});
    label->setPosition(padding * ccp(1, -1));
    label->setScale(0.5);
    addChild(label, 1000);

    return true;
}