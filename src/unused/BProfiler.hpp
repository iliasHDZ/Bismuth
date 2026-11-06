#pragma once

#include "common.hpp"

class BProfiler {
public:
    class Timer;

public:
    static Timer start(const std::string& name);

    static bool isUsingAverages();

    static void useAverages(bool cond);

    static std::string toString();

    static void frameEnd();

private:
    static void end(std::string name, u64 time);

public:
    struct Timer {
    private:
        std::string name;
        u64 lastTime;

    public:
        inline Timer(const std::string& name)
            : name(name), lastTime(getTime()) {}

        inline void end() {
            BProfiler::end(name, getTime() - lastTime);
        }
    };
};