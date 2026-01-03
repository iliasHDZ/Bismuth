#pragma once

#include "ccTypes.h"
#include <cstring>
#include <glm/glm.hpp>
#include <Geode/Geode.hpp>
#include <chrono>

#ifdef __GNUC__
#define PACKED( __Declaration__ ) __Declaration__ __attribute__((__packed__))
#endif

#ifdef _MSC_VER
#define PACKED( __Declaration__ ) __pragma( pack(push, 1) ) __Declaration__ __pragma( pack(pop))
#endif

//#define DEBUG_LOG(...) log::info(__VA_ARGS__)
#define DEBUG_LOG(...)

using u8  = unsigned char;
using u16 = unsigned short;
using u32 = unsigned int;
using u64 = unsigned long long;

using i8  = signed char;
using i16 = signed short;
using i32 = signed int;
using i64 = signed long long;

using vec2 = glm::vec2;
using vec3 = glm::vec3;
using vec4 = glm::vec4;

namespace fs = std::filesystem;

// TODO: Figure out how to set usize to the pointer size of this machine's architecture
using usize = u64;
using isize = i64;

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