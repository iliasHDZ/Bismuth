#include <Geode/Geode.hpp>
#include <Geode/modify/CCTextureAtlas.hpp>

namespace profiler {

void functionPush(const char* name);

void functionPop(const char* name);

};

#define CM ,

#define PROFILER_HOOK(Ret_, Class_, Name_, InArgs_, PassArgs_) \
    class GEODE_CRTP2(GEODE_CONCAT(profilerHook, __LINE__), Class_) { \
            Ret_ Name_(InArgs_) { \
            const char* name = #Class_ "::" #Name_; \
            profiler::functionPush(name); \
            Ret_ ret = Class_::Name_(PassArgs_); \
            profiler::functionPop(name); \
            return ret; \
        } \
    };

#define PROFILER_HOOK_VOID(Class_, Name_, InArgs_, PassArgs_) \
    class GEODE_CRTP2(GEODE_CONCAT(profilerHook, __LINE__), Class_) { \
            void Name_(InArgs_) { \
            const char* name = #Class_ "::" #Name_; \
            profiler::functionPush(name); \
            Class_::Name_(PassArgs_); \
            profiler::functionPop(name); \
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

#define _PROFILER_HOOK(...) GEODE_INVOKE(GEODE_CONCAT(PROFILER_HOOK_, GEODE_NUMBER_OF_ARGS(__VA_ARGS__)), __VA_ARGS__)

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

#define _PROFILER_HOOK_VOID(...) GEODE_INVOKE(GEODE_CONCAT(PROFILER_HOOK_VOID_, GEODE_NUMBER_OF_ARGS(__VA_ARGS__)), __VA_ARGS__)
