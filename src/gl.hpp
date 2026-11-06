#pragma once

#include <Geode/cocos/platform/win32/CCGL.h>

/*
    This file adds the required OpenGL 4.3 functions as
    the GLEW in Cocos2d only supports up to OpenGL 4.2
*/

#define GL_BUFFER 0x82E0
#define GL_SHADER 0x82E1
#define GL_PROGRAM 0x82E2
#define GL_SHADER_BINARY_FORMAT_SPIR_V 0x9551
#define GL_COMPUTE_SHADER 0x91B9
#define GL_SHADER_STORAGE_BLOCK 0x92E6
#define GL_SHADER_STORAGE_BUFFER 0x90D2

#ifndef GLAD_API_CALL
#define GLAD_API_CALL extern
#endif

typedef void (*PFNGLMULTIDRAWELEMENTSINDIRECTPROC)(GLenum mode, GLenum type, const void * indirect, GLsizei drawcount, GLsizei stride);
typedef void (*PFNGLMULTIDRAWARRAYSINDIRECTPROC)(GLenum mode, const void * indirect, GLsizei drawcount, GLsizei stride);
typedef void (*PFNGLDISPATCHCOMPUTEPROC)(GLuint num_groups_x, GLuint num_groups_y, GLuint num_groups_z);
typedef void (*PFNGLSPECIALIZESHADERPROC)(GLuint shader, const GLchar *pEntryPoint, GLuint numSpecializationConstants, const GLuint *pConstantIndex, const GLuint *pConstantValue);
typedef GLuint (*PFNGLGETPROGRAMRESOURCEINDEXPROC)(GLuint program, GLenum programInterface, const GLchar * name);
typedef void (*PFNGLSHADERSTORAGEBLOCKBINDINGPROC)(GLuint program, GLuint storageBlockIndex, GLuint storageBlockBinding);

GLAD_API_CALL PFNGLMULTIDRAWELEMENTSINDIRECTPROC glad_glMultiDrawElementsIndirect;
#define glMultiDrawElementsIndirect glad_glMultiDrawElementsIndirect
GLAD_API_CALL PFNGLMULTIDRAWARRAYSINDIRECTPROC glad_glMultiDrawArraysIndirect;
#define glMultiDrawArraysIndirect glad_glMultiDrawArraysIndirect
GLAD_API_CALL PFNGLDISPATCHCOMPUTEPROC glad_glDispatchCompute;
#define glDispatchCompute glad_glDispatchCompute
GLAD_API_CALL PFNGLSPECIALIZESHADERPROC glad_glSpecializeShader;
#define glSpecializeShader glad_glSpecializeShader
GLAD_API_CALL PFNGLGETPROGRAMRESOURCEINDEXPROC glad_glGetProgramResourceIndex;
#define glGetProgramResourceIndex glad_glGetProgramResourceIndex
GLAD_API_CALL PFNGLSHADERSTORAGEBLOCKBINDINGPROC glad_glShaderStorageBlockBinding;
#define glShaderStorageBlockBinding glad_glShaderStorageBlockBinding

bool hasLoadedNewerOpenGLFunctions();
void loadNewerOpenGLFunctions();

void storeVAO();
void restoreVAO();