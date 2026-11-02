#pragma once

#include "Geode/cocos/cocoa/CCGeometry.h"
#include "gl.hpp"
#include "ccTypes.h"
#include "glm/fwd.hpp"
#include <cstring>
#include <glm/glm.hpp>
#include <Geode/Geode.hpp>
#include <chrono>
#include <memory>
#include <stdint.h>

#define MOVE_TRIGGER_ID   901
#define ALPHA_TRIGGER_ID  1007
#define TOGGLE_TRIGGER_ID 1049
#define ROTATE_TRIGGER_ID 1346
#define FOLLOW_TRIGGER_ID 1347
#define GUIDE_OBJECT_ID   2016

#ifdef __GNUC__
#define PACKED( __Declaration__ ) __Declaration__ __attribute__((__packed__))
#endif

#ifdef _MSC_VER
#define PACKED( __Declaration__ ) __pragma( pack(push, 1) ) __Declaration__ __pragma( pack(pop))
#endif

//#define DEBUG_LOG(...) log::info(__VA_ARGS__)
#define DEBUG_LOG(...)

using u8  = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using i8  = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

using vec2 = glm::vec2;
using vec3 = glm::vec3;
using vec4 = glm::vec4;

namespace fs = std::filesystem;

// #define BISMUTH_ASSERT(COND) { if (!(COND)) geode::log::error("{}:{}: Assertion failed: {}", fs::path(__FILE__).filename(), __LINE__, #COND); }
#define BISMUTH_ASSERT(COND)

using usize = uintptr_t;
using isize = intptr_t;

template <typename T>
using UPtr = std::unique_ptr<T>;

template <typename T, typename... Args>
inline std::unique_ptr<T> makeUnique(Args&& ...args) {
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

inline vec2 ccPointToGLM(const cocos2d::CCPoint& point) {
    return vec2(point.x, point.y);
}

inline vec3 ccColor3BToGLM(const cocos2d::ccColor3B& color) {
    return vec3(color.r / 255.f, color.g / 255.f, color.b / 255.f);
}

extern std::chrono::steady_clock::time_point modStartTime;

inline u64 getTime() {
    return (std::chrono::high_resolution_clock::now() - modStartTime).count();
}

inline usize alignUp(usize value, usize align) {
    usize rem = value % align;
    return rem ? value + (align - rem) : value;
}

std::string byteSizeToString(usize size);

inline bool isNVidiaGPU() {
    return strcmp((const char*)glGetString(GL_VENDOR), "NVIDIA Corporation") == 0;
}

inline bool isAMDGPU() {
    return strcmp((const char*)glGetString(GL_VENDOR), "AMD") == 0;
}

inline bool isIntelGPU() {
    return strcmp((const char*)glGetString(GL_VENDOR), "Intel") == 0;
}

bool isOpenGLAtLeastVersion(u32 major, u32 minor);

void glObjectLabel(GLenum id, GLuint name, GLsizei length, const char* label);

/*
    Draws a fullscreen quad.
    VAO is handled automatically.
*/
void drawFullscreenQuad();

std::optional<std::string> readResourceFile(const fs::path& path);

inline glm::vec2 getClockwise(const glm::vec2& v) {
    return { v.y, -v.x };
}

inline glm::vec2 getCounterClockwise(const glm::vec2& v) {
    return { -v.y, v.x };
}