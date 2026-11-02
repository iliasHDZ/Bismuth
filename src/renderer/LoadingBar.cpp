#include "LoadingBar.hpp"
#include "Shader.hpp"

using namespace geode::prelude;

#define RADIUS 7.0f

#define PADDING_WIDTH 7.0f
#define PADDING_HEIGHT 5.0f
#define GAP 7.0f

#define PROGRESS_BAR_HEIGHT 0.7f

void LoadingBar::show() {
    CCDirector::get()->getRunningScene()->addChild(this, 100000);
    setPosition({CCDirector::get()->getWinSize().width / 2.0f, 40.0f});
    layout();
}

void LoadingBar::updateInfo(std::string text, usize countFinished, usize totalCount) {
    text = text + " (" + std::to_string(countFinished) + "/" + std::to_string(totalCount) + ")";
    textLabel->setString(text.c_str());

    progress = (float)countFinished / (float)totalCount;

    layout();
}

static UPtr<Shader> shader = nullptr;
static CCTexture2D* gradient = nullptr;
static CCTexture2D* gradientDark = nullptr;

void LoadingBar::draw() {
    if (!shader)
        return;

    shader->use();

    kmMat4 matrixP;
	kmMat4 matrixMV;
	kmMat4 matrixMVP;
	
	kmGLGetMatrix(KM_GL_PROJECTION, &matrixP);
	kmGLGetMatrix(KM_GL_MODELVIEW, &matrixMV);
	
	kmMat4Multiply(&matrixMVP, &matrixP, &matrixMV);

    ccGLBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    shader->setMatrix4("CC_MVPMatrix", matrixMVP.mat);
    shader->setVec2("u_contentSize", ccPointToGLM(getContentSize()));

    shader->setInt("u_gradient", 0);
    shader->setInt("u_gradientDark", 1);

    ccGLBindTexture2DN(0, gradient->getName());
    ccGLBindTexture2DN(1, gradientDark->getName());

    if (!vertexBuffer || lastContentSize != getContentSize()) {
        generateMesh();
        lastContentSize = getContentSize();
    }

    vertexBuffer->bindAs(GL_ARRAY_BUFFER);
    indexBuffer->bindAs(GL_ELEMENT_ARRAY_BUFFER);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)sizeof(glm::vec2));

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    shader->setInt("u_outline", 0);
    glDrawElements(GL_TRIANGLES, indicies.size(), GL_UNSIGNED_SHORT, NULL);

    shader->setInt("u_outline", 1);
    glDrawElements(GL_TRIANGLES, indicies.size(), GL_UNSIGNED_SHORT, NULL);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

bool LoadingBar::init() {
    if (!CCNode::init())
        return false;

    icon = CCSprite::create("icon.png"_spr);
    progressBar = CCDrawNode::create();

    textLabel = CCLabelBMFont::create("Loading...", "chatFont.fnt");
    if (!textLabel)
        return false;

    // setShaderProgram(shader);

    addChild(icon);
    addChild(textLabel);
    addChild(progressBar);
    return true;
}

void LoadingBar::layout() {
    icon->setAnchorPoint({0, 0});
    textLabel->setAnchorPoint({0, 0});

    float x = PADDING_WIDTH;

    float innerHeight = std::max(icon->getContentHeight(), textLabel->getContentHeight());

    float innerBottom = PROGRESS_BAR_HEIGHT + 2.0f + PADDING_HEIGHT;

    icon->setPosition({x, (innerHeight - icon->getContentHeight()) / 2 + innerBottom});
    x += icon->getContentWidth() + GAP;

    textLabel->setPosition({x, (innerHeight - textLabel->getContentHeight()) / 2 + innerBottom});
    x += textLabel->getContentWidth() + 1.0f;

    float progressBarMaxWidth = icon->getContentWidth() + GAP + textLabel->getContentWidth() + 1.0f;

    const auto progressColor = ccc4FFromccc3B(ccColor3B {0, 136, 255});

    progressBar->setPosition({PADDING_WIDTH, PADDING_HEIGHT});
    progressBar->clear();
    progressBar->drawRect(CCRect({0, 0}, {progressBarMaxWidth * progress, PROGRESS_BAR_HEIGHT}), progressColor, 0, {0});

    setContentSize({x + PADDING_WIDTH, innerHeight + PADDING_HEIGHT * 2 + innerBottom - PADDING_HEIGHT});

    setAnchorPoint({0.5f, 0.5f});
}

void LoadingBar::generateMesh() {
    verticies.clear();
    indicies.clear();

    float w = getContentWidth();
    float h = getContentHeight();
    
    float bordersX[4] = { 0, RADIUS, w - RADIUS, w };
    float bordersY[4] = { 0, RADIUS, h - RADIUS, h };
    float radiusX[4] = { 1, 0, 0, 1 };
    float radiusY[4] = { 1, 0, 0, 1 };

    for (isize x = 0; x < 3; x++)
        for (isize y = 0; y < 3; y++) {
            usize index = verticies.size();

            verticies.push_back({ {bordersX[x + 0], bordersY[y + 0]}, {radiusX[x + 0], radiusY[y + 0]} });
            verticies.push_back({ {bordersX[x + 1], bordersY[y + 0]}, {radiusX[x + 1], radiusY[y + 0]} });
            verticies.push_back({ {bordersX[x + 0], bordersY[y + 1]}, {radiusX[x + 0], radiusY[y + 1]} });
            verticies.push_back({ {bordersX[x + 1], bordersY[y + 1]}, {radiusX[x + 1], radiusY[y + 1]} });

            indicies.push_back(index + 0);
            indicies.push_back(index + 1);
            indicies.push_back(index + 2);
            indicies.push_back(index + 1);
            indicies.push_back(index + 3);
            indicies.push_back(index + 2);
        }

    if (!vertexBuffer)
        vertexBuffer = Buffer::create("VertexBuffer", verticies.size() * sizeof(Vertex), GL_STATIC_DRAW);
    if (!indexBuffer)
        indexBuffer = Buffer::create("IndexBuffer", indicies.size() * sizeof(u16), GL_STATIC_DRAW);

    vertexBuffer->write(std::span<Vertex>(verticies));
    vertexBuffer->flush(0, verticies.size() * sizeof(Vertex));
    indexBuffer->write(std::span<u16>(indicies));
    indexBuffer->flush(0, indicies.size() * sizeof(u16));
}

$on_mod(Loaded) {
    shader = Shader::create("loadingBar.vert", "loadingBar.frag");
    gradient = CCTextureCache::get()->addImage("bismuthGradient.png"_spr, true);
    gradientDark = CCTextureCache::get()->addImage("bismuthGradientDark.png"_spr, true);
    /*
    auto vertSource = readResourceFile("loadingBar.vert");
    auto fragSource = readResourceFile("loadingBar.frag");

    if (!vertSource || !fragSource)
        return;

    shader = new CCGLProgram;
    if (!shader->initWithVertexShaderByteArray(vertSource.value().c_str(), fragSource.value().c_str())) {
        log::error("Failed to compile loadingBar shader");
        delete shader;
        shader = nullptr;
        return;
    }

    shader->addAttribute("a_position", 0);
    shader->addAttribute("a_radiusVector", 1);

    CCShaderCache::sharedShaderCache()->addProgram(shader, "loadingBar");
    */
}