#pragma once

#include "ccTypes.h"
#include <cstring>
#include <glm/glm.hpp>
#include <Geode/Geode.hpp>
#include <chrono>
#include <memory>
#include <stdint.h>

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

// TODO: Figure out how to set usize to the pointer size of this machine's architecture
using usize = u64;
using isize = i64;

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

inline bool isNVidiaGPU() {
    return strcmp((const char*)glGetString(GL_VENDOR), "NVIDIA Corporation") == 0;
}

inline bool isAMDGPU() {
    return strcmp((const char*)glGetString(GL_VENDOR), "AMD") == 0;
}

inline bool isIntelGPU() {
    return strcmp((const char*)glGetString(GL_VENDOR), "Intel") == 0;
}

/*
    Draws a fullscreen quad.
    VAO is handled automatically.
*/
void drawFullscreenQuad();