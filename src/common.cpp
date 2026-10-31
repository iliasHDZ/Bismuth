#include "common.hpp"
#include "Geode/cocos/CCDirector.h"
#include "renderer/Buffer.hpp"

using namespace geode::prelude;

std::chrono::steady_clock::time_point modStartTime;

$on_mod(Loaded) {
    modStartTime = std::chrono::high_resolution_clock::now();
}

std::string byteSizeToString(usize size) {
    std::string unit = "B";
    double dsize = size;
    if (dsize > 1000.0) { dsize /= 1000.0; unit = "kB"; }
    if (dsize > 1000.0) { dsize /= 1000.0; unit = "MB"; }
    return fmt::format("{:.3f}{}", dsize, unit);
}

bool isOpenGLAtLeastVersion(u32 major, u32 minor) {
    std::string version = (const char*)glGetString(GL_VERSION);
    if (version.starts_with("OpenGL ES"))
        return false;

    auto rawVersion = version.substr(0, version.find_first_of(' '));

    usize periodCount = 0;
    for (auto ch : rawVersion)
        if (ch == '.') periodCount++;

    if (periodCount < 2)
        rawVersion += ".0";

    auto ver = VersionInfo::parse(rawVersion);
    if (!ver)
        return false;

    return VersionInfo(major, minor, 0) <= ver.unwrap();
}

using ProcGlObjectLabel = void(*)(GLenum, GLuint, GLsizei, const char*);

ProcGlObjectLabel glObjectLabelProc = nullptr;

void glObjectLabel(GLenum id, GLuint name, GLsizei length, const char* label) {
    #ifdef GEODE_WINDOWS
    if (!glObjectLabelProc)
        glObjectLabelProc = (ProcGlObjectLabel)wglGetProcAddress("glObjectLabel");
    if (glObjectLabelProc)
        glObjectLabelProc(id, name, length, label);
    #endif
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
UPtr<Buffer> fullscreenQuadVBO = nullptr;

void drawFullscreenQuad() {
    if (!fullscreenQuadVAO) {
        if (!fullscreenQuadVBO)
            fullscreenQuadVBO = Buffer::createStaticDraw("Fullscreen quad buffer", fullscreenQuad, sizeof(fullscreenQuad));

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