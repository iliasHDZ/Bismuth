#pragma once

#include <common.hpp>
#include "Buffer.hpp"

#define MIN_LOAD_STEP_TIME (1.0 / 240.0)

class LoadingBar : public cocos2d::CCNode {
public:
    void show();

    void updateInfo(std::string text, usize countFinished, usize totalCount);

    void draw() override;

public:
    CREATE_FUNC(LoadingBar);

private:
    bool init() override;

    void layout();

    void generateMesh();

private:
    cocos2d::CCDrawNode* progressBar;
    cocos2d::CCSprite* icon;
    cocos2d::CCLabelBMFont* textLabel;

    float progress = 0.0f;

    struct Vertex {
        glm::vec2 position;
        glm::vec2 radiusVector;
    };

    std::vector<Vertex> verticies;
    std::vector<u16> indicies;

    cocos2d::CCSize lastContentSize = { -1, -1 };

    UPtr<Buffer> vertexBuffer = nullptr;
    UPtr<Buffer> indexBuffer = nullptr;
};