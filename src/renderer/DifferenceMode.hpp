#include <common.hpp>
#include "Buffer.hpp"
#include "Shader.hpp"

class Renderer;

class DifferenceMode {
public:
    ~DifferenceMode();
    inline DifferenceMode(Renderer& renderer)
        : renderer(renderer) {}

    /*
    void drawSceneHook1();
    void drawSceneHook2();
    void drawSceneHook3();
    void drawSceneHook4();
    */

    void drawSceneHook();

private:
    void prepare(u32 width, u32 height);

    void destroyFramebuffers();

private:
    Renderer& renderer;

    float intensity = 5.0;
    float backdropIntensity = 0.2;

    cocos2d::CCSize lastSize = { -1.0, -1.0 };

    i32 prevFramebuffer;
    bool rendererPrevEnabled;
    bool prevPaused;

    u32 vanillaTexture = 0;
    u32 vanillaFramebuffer = 0;

    u32 bismuthTexture = 0;
    u32 bismuthFramebuffer = 0;

    u32 fullscreenQuadVAO = 0;
    Buffer* fullscreenQuadBuffer = nullptr;

    Shader* shader = nullptr;
};