#define GLAD_API_CALL
#include "gl.hpp"

struct CastableFuncPtr {
    void* ptr;

    template <typename T>
    operator T() {
        return (T)ptr;
    }
};

static bool loaded = false;

inline void* glGetProcAddressRaw(const char* name) {
    return (void*)wglGetProcAddress(name);
}

inline CastableFuncPtr glGetProcAddress(const char* name) {
    return (CastableFuncPtr)glGetProcAddressRaw(name);
}

bool hasLoadedNewerOpenGLFunctions() {
    return loaded;
}

void loadNewerOpenGLFunctions() {
    glMultiDrawElementsIndirect = glGetProcAddress("glMultiDrawElementsIndirect");
    glMultiDrawArraysIndirect = glGetProcAddress("glMultiDrawArraysIndirect");
    glDispatchCompute = glGetProcAddress("glDispatchCompute");
    glSpecializeShader = glGetProcAddress("glSpecializeShader");
    glGetProgramResourceIndex = glGetProcAddress("glGetProgramResourceIndex");
    glShaderStorageBlockBinding = glGetProcAddress("glShaderStorageBlockBinding");

    loaded = true;
}

static int storedVAO;

void storeVAO() {
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &storedVAO);
}

void restoreVAO() {
    glBindVertexArray(storedVAO);
}