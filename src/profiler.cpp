#include <Geode/Geode.hpp>
#include <unordered_map>
#include <vector>
#include "Geode/cocos/CCDirector.h"
#include "common.hpp"
#include "profiler.hpp"
#include "HotKey.hpp"
#include "utils/GPUTimer.hpp"
#include "renderer/Renderer.hpp"

#include <Geode/modify/CCKeyboardDispatcher.hpp>
#include <Geode/modify/CCDisplayLinkDirector.hpp>
#include <Geode/modify/CCDirector.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/LevelEditorLayer.hpp>
#include <Geode/modify/CCSprite.hpp>

using namespace geode::prelude;

bool takeSnapshotNextFrame = false;
bool isTakingSnapshot = false;
bool profiler::isTakingGPUTime = false;

static std::unordered_map<void*, i64> timeSpentInFunction;

static i64 lastFunctionTime;
static std::vector<void*> functionCallStack;

static std::set<GPUTimer*> gpuTimers;

// TODO: Make sure to not use queries if the drivers don't support it
static bool supportsQueries = true;

DECLARE_HOTKEY(KEY_P, {
    takeSnapshotNextFrame = true;
});

class $modify(MyCCDisplayLinkDirector, cocos2d::CCDisplayLinkDirector) {
    void mainLoop() {
        u64 processTime = 0;

        if (takeSnapshotNextFrame) {
            isTakingSnapshot = true;
            profiler::isTakingGPUTime = Renderer::get() != nullptr;
            takeSnapshotNextFrame = false;
            timeSpentInFunction.clear();
            if (!profiler::isTakingGPUTime)
                glBeginQuery(GL_TIME_ELAPSED, 50);
            processTime = getTime();
        }

        CCDisplayLinkDirector::mainLoop();
        
        if (isTakingSnapshot) {
            isTakingSnapshot = false;

            processTime = getTime() - processTime;

            i64 gpuTime = 0;
            if (!profiler::isTakingGPUTime) {
                glEndQuery(GL_TIME_ELAPSED);
                glGetQueryObjecti64v(50, GL_QUERY_RESULT, &gpuTime);
            }
            profiler::isTakingGPUTime = false;

            std::vector<std::pair<void*, i64>> functionsTime;
            for (auto pair : timeSpentInFunction)
                functionsTime.push_back(pair);
                
            std::sort(
                functionsTime.begin(),
                functionsTime.end(),
                [&](const std::pair<void*, i64>& a, const std::pair<void*, i64>& b) {
                    return a.second < b.second;
                }
            );
        
            i32 functionSumTime = 0;

            for (auto [name, time] : functionsTime) {
                log::info("- {} : {}ms", (const char*)name, (double)time / 1000000.0);
                functionSumTime += time;
            }

            log::info("Functions time sum: {}ms", (double)functionSumTime / 1000000.0);
            log::info("CPU time: {}ms", (double)processTime / 1000000.0);
            log::info("Total GPU time: {}ms", (double)gpuTime / 1000000.0);

            for (auto timer : gpuTimers) {
                log::info("{}: {}ms", timer->getName(), (double)timer->getTime() / 1000000.0);
            }
        }
    }
};

static void addToFunctionTime(void* name, u64 time) {
    if (timeSpentInFunction.find(name) == timeSpentInFunction.end())
        timeSpentInFunction[name] = time;
    else
        timeSpentInFunction[name] += time;
}

namespace profiler {

void functionPush(const char* name) {
    auto time = getTime();
    if (!isTakingSnapshot)
        return;
    
    if (!functionCallStack.empty()) 
        addToFunctionTime(functionCallStack.back(), time - lastFunctionTime);
    
    functionCallStack.push_back((void*)name);
    lastFunctionTime = getTime();
}

void functionPop() {
    auto time = getTime();
    if (!isTakingSnapshot)
        return;
    
    if (!functionCallStack.empty()) 
        addToFunctionTime(functionCallStack.back(), time - lastFunctionTime);
    
    functionCallStack.pop_back();
    lastFunctionTime = getTime();
}

void addTimer(GPUTimer* timer) {
    gpuTimers.insert(timer);
}

void removeTimer(GPUTimer* timer) {
    gpuTimers.erase(timer);
}

}

#include <Geode/modify/CCSprite.hpp>
// _PROFILER_HOOK_VOID(cocos2d::CCSprite, updateTransform);
// _PROFILER_HOOK_VOID(cocos2d::CCSprite, setColor, const cocos2d::ccColor3B&);
// _PROFILER_HOOK_VOID(cocos2d::CCSprite, setOpacity, unsigned char);

#include <Geode/modify/CCNodeRGBA.hpp>
// _PROFILER_HOOK_VOID(cocos2d::CCNodeRGBA, setColor, const cocos2d::ccColor3B&);
// _PROFILER_HOOK_VOID(cocos2d::CCNodeRGBA, updateDisplayedColor, const cocos2d::ccColor3B&);

#include <Geode/modify/CCDisplayLinkDirector.hpp>
_PROFILER_HOOK_VOID(cocos2d::CCDisplayLinkDirector, mainLoop);

#include <Geode/modify/CCDirector.hpp>
_PROFILER_HOOK_VOID(cocos2d::CCDirector, drawScene);

#include <Geode/modify/CCScheduler.hpp>
_PROFILER_HOOK_VOID(cocos2d::CCScheduler, update, float);

#include <Geode/modify/CCScene.hpp>
_PROFILER_HOOK_VOID(cocos2d::CCScene, visit);

#include <Geode/modify/CCEGLView.hpp>
_PROFILER_HOOK_VOID(cocos2d::CCEGLView, swapBuffers);

#include <Geode/modify/PlayLayer.hpp>
_PROFILER_HOOK_VOID(PlayLayer, updateVisibility, float);

#include <Geode/modify/CCSpriteBatchNode.hpp>
// _PROFILER_HOOK_VOID(cocos2d::CCSpriteBatchNode, draw);

#include <Geode/modify/GJBaseGameLayer.hpp>
_PROFILER_HOOK_VOID(GJBaseGameLayer, update, float);
_PROFILER_HOOK_VOID(GJBaseGameLayer, preUpdateVisibility, float);
_PROFILER_HOOK_VOID(GJBaseGameLayer, visit);
_PROFILER_HOOK_VOID(GJBaseGameLayer, processDynamicObjectActions, int, float);
_PROFILER_HOOK_VOID(GJBaseGameLayer, processTransformActions, bool);
_PROFILER_HOOK_VOID(GJBaseGameLayer, processRotationActions);
_PROFILER_HOOK_VOID(GJBaseGameLayer, processMoveActions);
_PROFILER_HOOK_VOID(GJBaseGameLayer, processPlayerFollowActions, float);
_PROFILER_HOOK_VOID(GJBaseGameLayer, processAdvancedFollowActions, float);
_PROFILER_HOOK_VOID(GJBaseGameLayer, processFollowActions);
_PROFILER_HOOK_VOID(GJBaseGameLayer, processMoveActionsStep, float, bool);