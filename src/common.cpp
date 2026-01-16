#include "common.hpp"
#include "Geode/cocos/CCDirector.h"
#include "Geode/cocos/platform/win32/CCGL.h"
#include "renderer/Buffer.hpp"

using namespace geode::prelude;

std::chrono::steady_clock::time_point modStartTime;

$on_mod(Loaded) {
    modStartTime = std::chrono::high_resolution_clock::now();
}

static float fullscreenQuad[] = {
    -1.0, -1.0,
    -1.0,  1.0,
     1.0,  1.0,
    -1.0, -1.0,
     1.0,  1.0,
     1.0, -1.0
};

u32 fullscreenQuadVAO = 0;
Buffer* fullscreenQuadVBO = nullptr;

void drawFullscreenQuad() {
    if (!fullscreenQuadVAO) {
        if (!fullscreenQuadVBO)
            fullscreenQuadVBO = Buffer::createStaticDraw(fullscreenQuad, sizeof(fullscreenQuad));

        glGenVertexArrays(1, &fullscreenQuadVAO);
        glBindVertexArray(fullscreenQuadVAO);
        fullscreenQuadVBO->bindAs(GL_ARRAY_BUFFER);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, (void*)0);
        glEnableVertexAttribArray(0);
    }

    glBindVertexArray(fullscreenQuadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

#include <geode/modify/CCDirector.hpp>
class $modify(CommonCCDirector, CCDirector) {
    void purgeDirector() {
        if (fullscreenQuadVAO)
            glDeleteVertexArrays(1, &fullscreenQuadVAO);
        if (fullscreenQuadVBO)
            Buffer::destroy(fullscreenQuadVBO);
        fullscreenQuadVAO = 0;
        fullscreenQuadVBO = nullptr;

        CCDirector::purgeDirector();
    }
};

std::optional<std::string> readResourceFile(const fs::path& path) {
    std::ifstream t(Mod::get()->getResourcesDir() / path);
    if (!t.is_open())
        return std::nullopt;

    t.seekg(0, std::ios::end);
    size_t size = t.tellg();
    std::string buffer(size, ' ');
    t.seekg(0);
    t.read(buffer.data(), size);
    t.close();

    return buffer;
}