#include <Geode/Geode.hpp>
#include <chrono>
#include <iostream>
#include <unordered_map>
#include <vector>
#include "common.hpp"
#include "profiler.hpp"

#include <Geode/modify/CCKeyboardDispatcher.hpp>
#include <Geode/modify/CCDisplayLinkDirector.hpp>
#include <Geode/modify/CCDirector.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/LevelEditorLayer.hpp>
#include <Geode/modify/CCSprite.hpp>

bool takeSnapshotNextFrame = false;
bool isTakingSnapshot      = false;

std::unordered_map<void*, i64> timeSpentInFunction;

i64 lastFunctionTime;
std::vector<void*> functionCallStack;

std::chrono::steady_clock::time_point modStartTime;

$on_mod(Loaded) {
    modStartTime = std::chrono::high_resolution_clock::now();
}

class $modify(MyCCKeyboardDispatcher, cocos2d::CCKeyboardDispatcher) {
    bool dispatchKeyboardMSG(cocos2d::enumKeyCodes key, bool keyDown, bool p3) {
        if (keyDown && key == cocos2d::KEY_P)
            takeSnapshotNextFrame = true;

        return cocos2d::CCKeyboardDispatcher::dispatchKeyboardMSG(key, keyDown, p3);
    }
};

class $modify(MyCCDisplayLinkDirector, cocos2d::CCDisplayLinkDirector) {
    void mainLoop() {
        if (takeSnapshotNextFrame) {
            isTakingSnapshot = true;
            takeSnapshotNextFrame = false;
            timeSpentInFunction.clear();
        }
        
        CCDisplayLinkDirector::mainLoop();
        
        if (isTakingSnapshot) {
            isTakingSnapshot = false;

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
        
            for (auto [name, time] : functionsTime)
                geode::log::info("- {} : {}ms", (const char*)name, (double)time / 1000000.0);
        }
    }
};

static u64 getTime() {
    return (std::chrono::high_resolution_clock::now() - modStartTime).count();
}

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

void functionPop(const char* name) {
    auto time = getTime();
    if (!isTakingSnapshot)
        return;
    
    if (!functionCallStack.empty()) 
        addToFunctionTime(functionCallStack.back(), time - lastFunctionTime);
    
    functionCallStack.pop_back();
    lastFunctionTime = getTime();
}

}

#include <Geode/modify/CCSprite.hpp>
_PROFILER_HOOK_VOID(cocos2d::CCSprite, updateTransform);
_PROFILER_HOOK_VOID(cocos2d::CCSprite, setColor, const cocos2d::ccColor3B&);
_PROFILER_HOOK_VOID(cocos2d::CCSprite, setOpacity, unsigned char);

#include <Geode/modify/CCNodeRGBA.hpp>
_PROFILER_HOOK_VOID(cocos2d::CCNodeRGBA, setColor, const cocos2d::ccColor3B&);
_PROFILER_HOOK_VOID(cocos2d::CCNodeRGBA, updateDisplayedColor, const cocos2d::ccColor3B&);

#include <Geode/modify/CCSpriteBatchNode.hpp>
_PROFILER_HOOK_VOID(cocos2d::CCSpriteBatchNode, draw);