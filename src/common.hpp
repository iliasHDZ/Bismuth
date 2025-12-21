#pragma once

#include <glm/glm.hpp>
#include <Geode/Geode.hpp>
#include <filesystem>

using u8  = unsigned char;
using u16 = unsigned short;
using u32 = unsigned int;
using u64 = unsigned long long;

using i8  = signed char;
using i16 = signed short;
using i32 = signed int;
using i64 = signed long long;

namespace fs = std::filesystem;

// TODO: Figure out how to set usize to the pointer size of this machine's architecture
using usize = u64;
using isize = i64;

inline glm::vec2 ccPointToGLM(const cocos2d::CCPoint& point) {
    return glm::vec2(point.x, point.y);
}