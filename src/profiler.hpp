#pragma once

#include <Geode/Geode.hpp>
#include <Geode/modify/CCTextureAtlas.hpp>

#define PROFILER_ENABLED

#ifdef PROFILER_ENABLED
#define PROFILER_PUSH(S) profiler::functionPush(S)
#define PROFILER_POP() profiler::functionPop()
#define PROFILER_SCOPE(S) ScopedProfiler __profiler(S)
#else
#define PROFILER_PUSH(S)
#define PROFILER_POP()
#define PROFILER_SCOPE(S)
#endif

class GPUTimer;

namespace profiler {

extern bool isTakingGPUTime;

void functionPush(const char* name);

void functionPop();

void addTimer(GPUTimer* timer);

void removeTimer(GPUTimer* timer);

};

class ScopedProfiler {
public:
    inline ScopedProfiler(const char* name) {
        profiler::functionPush(name);
    }

    inline ~ScopedProfiler() {
        profiler::functionPop();
    }
};

#define CM ,

#define PROFILER_HOOK(Ret_, Class_, Name_, InArgs_, PassArgs_) \
    class GEODE_CRTP2(GEODE_CONCAT(profilerHook, __LINE__), Class_) { \
            Ret_ Name_(InArgs_) { \
            profiler::functionPush(#Class_ "::" #Name_); \
            Ret_ ret = Class_::Name_(PassArgs_); \
            profiler::functionPop(); \
            return ret; \
        } \
    };

#define PROFILER_HOOK_VOID(Class_, Name_, InArgs_, PassArgs_) \
    class GEODE_CRTP2(GEODE_CONCAT(profilerHook, __LINE__), Class_) { \
            void Name_(InArgs_) { \
            profiler::functionPush(#Class_ "::" #Name_); \
            Class_::Name_(PassArgs_); \
            profiler::functionPop(); \
        } \
    };

#define PROFILER_HOOK_3(Ret_, Class_, Name_) \
    PROFILER_HOOK(Ret_, Class_, Name_, , )
#define PROFILER_HOOK_4(Ret_, Class_, Name_, A_) \
    PROFILER_HOOK(Ret_, Class_, Name_, A_ a, a)
#define PROFILER_HOOK_5(Ret_, Class_, Name_, A_, B_) \
    PROFILER_HOOK(Ret_, Class_, Name_, A_ a CM B_ b, a CM b)
#define PROFILER_HOOK_6(Ret_, Class_, Name_, A_, B_, C_) \
    PROFILER_HOOK(Ret_, Class_, Name_, A_ a CM B_ b CM C_ c, a CM b CM c)
#define PROFILER_HOOK_7(Ret_, Class_, Name_, A_, B_, C_, D_) \
    PROFILER_HOOK(Ret_, Class_, Name_, A_ a CM B_ b CM C_ c CM D_ d, a CM b CM c CM d)
#define PROFILER_HOOK_8(Ret_, Class_, Name_, A_, B_, C_, D_, E_) \
    PROFILER_HOOK(Ret_, Class_, Name_, A_ a CM B_ b CM C_ c CM D_ d CM E_ e, a CM b CM c CM d CM e)
#define PROFILER_HOOK_9(Ret_, Class_, Name_, A_, B_, C_, D_, E_, F_) \
    PROFILER_HOOK(Ret_, Class_, Name_, A_ a CM B_ b CM C_ c CM D_ d CM E_ e CM F_ f, a CM b CM c CM d CM e CM f)

#define PROFILER_HOOK_VOID_2(Class_, Name_) \
    PROFILER_HOOK_VOID(Class_, Name_, , )
#define PROFILER_HOOK_VOID_3(Class_, Name_, A_) \
    PROFILER_HOOK_VOID(Class_, Name_, A_ a, a)
#define PROFILER_HOOK_VOID_4(Class_, Name_, A_, B_) \
    PROFILER_HOOK_VOID(Class_, Name_, A_ a CM B_ b, a CM b)
#define PROFILER_HOOK_VOID_5(Class_, Name_, A_, B_, C_) \
    PROFILER_HOOK_VOID(Class_, Name_, A_ a CM B_ b CM C_ c, a CM b CM c)
#define PROFILER_HOOK_VOID_6(Class_, Name_, A_, B_, C_, D_) \
    PROFILER_HOOK_VOID(Class_, Name_, A_ a CM B_ b CM C_ c CM D_ d, a CM b CM c CM d)
#define PROFILER_HOOK_VOID_7(Class_, Name_, A_, B_, C_, D_, E_) \
    PROFILER_HOOK_VOID(Class_, Name_, A_ a CM B_ b CM C_ c CM D_ d CM E_ e, a CM b CM c CM d CM e)
#define PROFILER_HOOK_VOID_8(Class_, Name_, A_, B_, C_, D_, E_, F_) \
    PROFILER_HOOK_VOID(Class_, Name_, A_ a CM B_ b CM C_ c CM D_ d CM E_ e CM F_ f, a CM b CM c CM d CM e CM f)

#ifdef PROFILER_ENABLED
#define _PROFILER_HOOK(...) GEODE_INVOKE(GEODE_CONCAT(PROFILER_HOOK_, GEODE_NUMBER_OF_ARGS(__VA_ARGS__)), __VA_ARGS__)
#define _PROFILER_HOOK_VOID(...) GEODE_INVOKE(GEODE_CONCAT(PROFILER_HOOK_VOID_, GEODE_NUMBER_OF_ARGS(__VA_ARGS__)), __VA_ARGS__)
#else
#define _PROFILER_HOOK(...)
#define _PROFILER_HOOK_VOID(...)
#endif
