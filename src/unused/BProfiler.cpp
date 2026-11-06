#include "BProfiler.hpp"
#include "common.hpp"
#include <fmt/format.h>

static bool shouldUseAverages = false;
static usize divideNumber = 0;

static std::map<std::string, u64> currentTimes;
static std::map<std::string, u64> savedTimes;

static inline void addToMap(std::map<std::string, u64>& map, std::string name, u64 num) {
    auto it = map.find(name);
    if (it != map.end())
        it->second += num;
    else
        map[name] = num;
}

BProfiler::Timer BProfiler::start(const std::string& name) {
    BProfiler::Timer timer { name };
    return timer;
}

bool BProfiler::isUsingAverages() {
    return shouldUseAverages;
}

void BProfiler::useAverages(bool cond) {
    shouldUseAverages = cond;
}

std::string BProfiler::toString() {
    std::string ret = "";
    for (auto& [name, time] : savedTimes)
        ret += fmt::format("{}: {:.5f}ms\n", name, ((double)time / divideNumber) / 1000000.0);
    return ret;
}

void BProfiler::frameEnd() {
    if (!shouldUseAverages) {
        savedTimes = currentTimes;
        divideNumber = 1;
    } else {
        for (auto& [name, time] : currentTimes)
            addToMap(savedTimes, name, time);
        divideNumber++;
    }
    currentTimes.clear();
}

void BProfiler::end(std::string name, u64 time) {
    addToMap(currentTimes, name, time);
}