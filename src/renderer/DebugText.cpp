#include "DebugText.hpp"

using namespace geode::prelude;

void DebugText::setText(const std::string& str) {
    if (str == "") {
        setVisible(false);
        return;
    }

    setVisible(true);

    label->setString(str.c_str());
    labelOutline1->setString(str.c_str());
    labelOutline2->setString(str.c_str());
}

bool DebugText::init() {
    label = CCLabelBMFont::create("", "chatFont.fnt");
    labelOutline1 = CCLabelBMFont::create("", "chatFont.fnt");
    labelOutline2 = CCLabelBMFont::create("", "chatFont.fnt");

    labelOutline1->setColor({0, 0, 0});
    labelOutline1->setOpacity(200);
    labelOutline2->setColor({0, 0, 0});
    labelOutline2->setOpacity(200);

    setAnchorPoint({0, 1});
    setPosition({1, CCDirector::get()->getWinSize().height - 8});

    label->setAnchorPoint({0, 1});
    label->setPosition(0, 0);
    label->setScale(0.5);
    addChild(label, 1000);

    labelOutline1->setAnchorPoint({0, 1});
    labelOutline1->setPosition(ccp(-0.5, -0.5));
    labelOutline1->setScale(0.5);
    addChild(labelOutline1, 999);

    labelOutline2->setAnchorPoint({0, 1});
    labelOutline2->setPosition(ccp(0.5, 0.5));
    labelOutline2->setScale(0.5);
    addChild(labelOutline2, 999);

    return true;
}