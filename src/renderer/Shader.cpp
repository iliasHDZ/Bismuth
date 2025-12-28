#include "Shader.hpp"
#include "Geode/cocos/platform/win32/CCGL.h"

#include <Geode/Geode.hpp>
#include <vector>

#include <glslang/Public/ShaderLang.h>
#include <SPIRV/GlslangToSpv.h>
#include <optional>
#include <memory>

using namespace geode;

Shader::~Shader() {
    if (program)
        glDeleteProgram(program);
}

void printErrorLog(std::string str) {
    char* log = str.data();
    char* start = log;

    while (*log) {
        if (*log == '\n') {
            *log = 0;
            log::error("    {}", start);
            start = log + 1;
        }
        log++;
    }

    if (*start != 0)
        log::error("    {}", start);
}

std::optional<std::string> readResourceFile(const fs::path& path) {
    std::ifstream t(Mod::get()->getResourcesDir() / path);
    if (!t.is_open())
        return std::nullopt;

    t.seekg(0, std::ios::end);
    size_t size = t.tellg();
    std::string buffer(size, ' ');
    t.seekg(0);
    t.read(buffer.data(), size);
    t.close();

    return buffer;
}

/*
    This is the main reason to compile with glslang. It is
    to use the #include pre-processor.
*/
class CustomIncluder : public glslang::TShader::Includer {
public:
    IncludeResult* includeLocal(
        const char* headerName,
        const char* includerName,
        size_t inclusionDepth
    ) override {
        auto res = readResourceFile(headerName);
        if (!res) {
            geode::log::error("could not include header file at: {}", headerName);
            return nullptr;
        }

        headerSources.push_back(res.value());
        results.push_back(IncludeResult (headerName, headerSources.back().c_str(), headerSources.back().size(), nullptr));

        return &results.back();
    }

    void releaseInclude(IncludeResult*) override {}

private:
    std::vector<IncludeResult> results;
    std::vector<std::string> headerSources;
};

static TBuiltInResource getDefaultResources();

// Confusingly, the enum for shader stages in glslang is called EShLanguage
static std::unique_ptr<glslang::TShader> compileShader(EShLanguage stage, const std::string& code, const std::string& name) {
    auto shader = std::make_unique<glslang::TShader>(stage);

    const char* str   = code.c_str();
    const char* sname = name.c_str();
    int length = code.size();

    shader->setStringsWithLengthsAndNames(&str, &length, &sname, 1);
    shader->setEnvInput(glslang::EShSourceGlsl, stage, glslang::EShClientOpenGL, 450);
    shader->setEnvClient(glslang::EShClientOpenGL, glslang::EShTargetOpenGL_450);
    shader->setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_3);
    shader->setOverrideVersion(450);
    shader->setEntryPoint("main");
    shader->setAutoMapLocations(true);
    shader->setAutoMapBindings(true);

    CustomIncluder includer;

    auto res = getDefaultResources();
    if (!shader->parse(&res, 110, ENoProfile, false, false, EShMsgDefault, includer)) {
        const char* shaderType = (stage == EShLangFragment) ? "fragment" : "vertex";
        geode::log::error("failed to compile {} shader (glslang):", shaderType);
        printErrorLog(shader->getInfoLog());
        return nullptr;
    }

    return shader;
}

struct ShaderBinaries {
    bool success;
    std::vector<u32> vertexShader;
    std::vector<u32> fragmentShader;
};

static ShaderBinaries compileProgram(const fs::path& vertexPath, const fs::path& fragmentPath, std::string preCode) {
    auto vertexSource   = readResourceFile(vertexPath);
    auto fragmentSource = readResourceFile(fragmentPath);

    if (!vertexSource)
        geode::log::error("could not find vertex shader at path: {}", vertexPath);
    if (!fragmentSource)
        geode::log::error("could not find fragment shader at path: {}", fragmentPath);
    if (!vertexSource || !fragmentSource)
        return { false };

    auto vertexShader = compileShader(EShLangVertex, preCode + vertexSource.value(), vertexPath.string());
    if (!vertexShader) return { false };

    auto fragmentShader = compileShader(EShLangFragment, preCode + fragmentSource.value(), fragmentPath.string());
    if (!fragmentShader) return { false };
    
    auto program = std::make_unique<glslang::TProgram>();

    program->addShader(vertexShader.get());
    program->addShader(fragmentShader.get());

    if (!program->link(EShMsgDefault)) {
        geode::log::error("failed to link shader program (glslang):");
        printErrorLog(program->getInfoLog());
        return { false };
    }

    ShaderBinaries binaries = { true };
    glslang::TIntermediate* interm;

    interm = program->getIntermediate(EShLangVertex);
    glslang::GlslangToSpv(*interm, binaries.vertexShader);

    interm = program->getIntermediate(EShLangFragment);
    glslang::GlslangToSpv(*interm, binaries.fragmentShader);

    return binaries;
}

static u32 createShaderOld(i32 type, const char* source) {
    u32 shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    i32 success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[512];
        glGetShaderInfoLog(shader, 512, NULL, log);
        const char* shaderType = (type == GL_FRAGMENT_SHADER) ? "fragment" : "vertex";
        geode::log::error("failed to compile {} shader:", shaderType);
        printErrorLog(log);
        return 0;
    };

    return shader;
}

Shader* Shader::createOld(const ShaderSources& sources) {
    u32 vertexShader   = createShaderOld(GL_VERTEX_SHADER, sources.vertexSource);
    u32 fragmentShader = createShaderOld(GL_FRAGMENT_SHADER, sources.fragmentSource);

    if (!vertexShader || !fragmentShader) {
        if (vertexShader)   glDeleteShader(vertexShader);
        if (fragmentShader) glDeleteShader(fragmentShader);
        return nullptr;
    }

    u32 program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    int success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char log[512];
        glGetProgramInfoLog(program, 512, NULL, log);
        geode::log::error("Failed to link shader program:");
        printErrorLog(log);
        glDeleteProgram(program);
        return nullptr;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    Shader* shader = new Shader();
    shader->program = program;
    return shader;
}

Shader* Shader::createOld(
    const fs::path& vertexPath,
    const fs::path& fragmentPath
) {
    auto vertexSource   = readResourceFile(vertexPath);
    auto fragmentSource = readResourceFile(fragmentPath);
    return createOld({vertexSource.value().c_str(), fragmentSource.value().c_str()});
}

#define GL_SHADER_BINARY_FORMAT_SPIR_V 0x9551

typedef void* (*PFNWGLGETPROCADDRESSPROC_PRIVATE)(const char*);

typedef void(*glSpecializeShaderProc)(
    GLuint shader,
    const GLchar* pEntryPoint,
    GLuint numSpecializationConstants,
    const GLuint* pConstantIndex,
    const GLuint* pConstantValue
);

static u32 createShader(i32 type, const std::vector<u32>& code) {
    u32 shader = glCreateShader(type);
    glShaderBinary(1, &shader, GL_SHADER_BINARY_FORMAT_SPIR_V, code.data(), code.size() * sizeof(u32));

    auto wglGetProcAddress = (PFNWGLGETPROCADDRESSPROC_PRIVATE)GetProcAddress(LoadLibraryA("opengl32.dll"), "wglGetProcAddress");
    auto glSpecializeShader = (glSpecializeShaderProc)wglGetProcAddress("glSpecializeShader");

    glSpecializeShader(shader, "main", 0, NULL, NULL);
    glCompileShader(shader);

    i32 success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[512];
        glGetShaderInfoLog(shader, 512, NULL, log);
        const char* shaderType = (type == GL_FRAGMENT_SHADER) ? "fragment" : "vertex";
        geode::log::error("failed to compile {} shader (opengl):", shaderType);
        printErrorLog(log);
        glDeleteShader(shader);
        return 0;
    };

    return shader;
}

Shader* Shader::create(
    const fs::path& vertexPath,
    const fs::path& fragmentPath,
    std::map<std::string, std::string> macroVariables
) {
    std::string preCode = "#define GLSL\n";
    for (auto [k, v] : macroVariables)
        preCode += "#define " + k + " " + v + "\n";

    auto binaries = compileProgram(vertexPath, fragmentPath, preCode);
    if (!binaries.success)
        return nullptr;
    
    u32 vertexShader   = createShader(GL_VERTEX_SHADER,   binaries.vertexShader);
    u32 fragmentShader = createShader(GL_FRAGMENT_SHADER, binaries.fragmentShader);

    if (!vertexShader || !fragmentShader) {
        if (vertexShader)   glDeleteShader(vertexShader);
        if (fragmentShader) glDeleteShader(fragmentShader);
        return nullptr;
    }

    u32 program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    int success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char log[512];
        glGetProgramInfoLog(program, 512, NULL, log);
        geode::log::error("failed to link shader program (opengl):");
        printErrorLog(log);
        glDeleteProgram(program);
        return nullptr;
    }

    Shader* shader = new Shader();
    shader->program = program;
    return shader;
}

void Shader::setMatrix4(u32 location, const float* data) {
    use();
    glUniformMatrix4fv(location, 1, GL_FALSE, data);
}

void Shader::setInt(u32 location, i32 value) {
    use();
    glUniform1i(location, value);
}

void Shader::setUInt(u32 location, u32 value) {
    use();
    glUniform1ui(location, value);
}

void Shader::setFloat(u32 location, float value) {
    use();
    glUniform1f(location, value);
}

void Shader::setVec2(u32 location, glm::vec2 value) {
    use();
    glUniform2f(location, value.x, value.y);
}

void Shader::setVec3(u32 location, glm::vec3 value) {
    use();
    glUniform3f(location, value.x, value.y, value.z);
}

void Shader::setTexture(u32 location, i32 id, u32 texture) {
    use();
    glActiveTexture(GL_TEXTURE0 + id);
    glBindTexture(GL_TEXTURE_2D, texture);
    setInt(location, id);
}

void Shader::setTextureArray(u32 location, i32 count, u32* textures) {
    use();
    glUniform1iv(location, count, (i32*)textures);

    for (i32 i = 0; i < count; i++) {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, textures[i]);
    }
}

void Shader::setTextureArray(u32 location, i32 count, cocos2d::CCTexture2D** textures) {
    use();
    std::vector<i32> integers;
    integers.resize(count);

    for (i32 i = 0; i < count; i++)
        integers[i] = i;

    glUniform1iv(location, count, integers.data());

    for (i32 i = 0; i < count; i++) {
        if (textures[i] == nullptr)
            continue;

        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, textures[i]->getName());
    }
}

// A bunch of bs required to compile with glslang, idk why
static TBuiltInResource getDefaultResources() {
    TBuiltInResource resources;

    resources.maxLights                                 = 32;
    resources.maxClipPlanes                             = 6;
    resources.maxTextureUnits                           = 32;
    resources.maxTextureCoords                          = 32;
    resources.maxVertexAttribs                          = 64;
    resources.maxVertexUniformComponents                = 4096;
    resources.maxVaryingFloats                          = 64;
    resources.maxVertexTextureImageUnits                = 32;
    resources.maxCombinedTextureImageUnits              = 80;
    resources.maxTextureImageUnits                      = 32;
    resources.maxFragmentUniformComponents              = 4096;
    resources.maxDrawBuffers                            = 32;
    resources.maxVertexUniformVectors                   = 128;
    resources.maxVaryingVectors                         = 8;
    resources.maxFragmentUniformVectors                 = 16;
    resources.maxVertexOutputVectors                    = 16;
    resources.maxFragmentInputVectors                   = 15;
    resources.minProgramTexelOffset                     = -8;
    resources.maxProgramTexelOffset                     = 7;
    resources.maxClipDistances                          = 8;
    resources.maxComputeWorkGroupCountX                 = 65535;
    resources.maxComputeWorkGroupCountY                 = 65535;
    resources.maxComputeWorkGroupCountZ                 = 65535;
    resources.maxComputeWorkGroupSizeX                  = 1024;
    resources.maxComputeWorkGroupSizeY                  = 1024;
    resources.maxComputeWorkGroupSizeZ                  = 64;
    resources.maxComputeUniformComponents               = 1024;
    resources.maxComputeTextureImageUnits               = 16;
    resources.maxComputeImageUniforms                   = 8;
    resources.maxComputeAtomicCounters                  = 8;
    resources.maxComputeAtomicCounterBuffers            = 1;
    resources.maxVaryingComponents                      = 60;
    resources.maxVertexOutputComponents                 = 64;
    resources.maxGeometryInputComponents                = 64;
    resources.maxGeometryOutputComponents               = 128;
    resources.maxFragmentInputComponents                = 128;
    resources.maxImageUnits                             = 8;
    resources.maxCombinedImageUnitsAndFragmentOutputs   = 8;
    resources.maxCombinedShaderOutputResources          = 8;
    resources.maxImageSamples                           = 0;
    resources.maxVertexImageUniforms                    = 0;
    resources.maxTessControlImageUniforms               = 0;
    resources.maxTessEvaluationImageUniforms            = 0;
    resources.maxGeometryImageUniforms                  = 0;
    resources.maxFragmentImageUniforms                  = 8;
    resources.maxCombinedImageUniforms                  = 8;
    resources.maxGeometryTextureImageUnits              = 16;
    resources.maxGeometryOutputVertices                 = 256;
    resources.maxGeometryTotalOutputComponents          = 1024;
    resources.maxGeometryUniformComponents              = 1024;
    resources.maxGeometryVaryingComponents              = 64;
    resources.maxTessControlInputComponents             = 128;
    resources.maxTessControlOutputComponents            = 128;
    resources.maxTessControlTextureImageUnits           = 16;
    resources.maxTessControlUniformComponents           = 1024;
    resources.maxTessControlTotalOutputComponents       = 4096;
    resources.maxTessEvaluationInputComponents          = 128;
    resources.maxTessEvaluationOutputComponents         = 128;
    resources.maxTessEvaluationTextureImageUnits        = 16;
    resources.maxTessEvaluationUniformComponents        = 1024;
    resources.maxTessPatchComponents                    = 120;
    resources.maxPatchVertices                          = 32;
    resources.maxTessGenLevel                           = 64;
    resources.maxViewports                              = 16;
    resources.maxVertexAtomicCounters                   = 0;
    resources.maxTessControlAtomicCounters              = 0;
    resources.maxTessEvaluationAtomicCounters           = 0;
    resources.maxGeometryAtomicCounters                 = 0;
    resources.maxFragmentAtomicCounters                 = 8;
    resources.maxCombinedAtomicCounters                 = 8;
    resources.maxAtomicCounterBindings                  = 1;
    resources.maxVertexAtomicCounterBuffers             = 0;
    resources.maxTessControlAtomicCounterBuffers        = 0;
    resources.maxTessEvaluationAtomicCounterBuffers     = 0;
    resources.maxGeometryAtomicCounterBuffers           = 0;
    resources.maxFragmentAtomicCounterBuffers           = 1;
    resources.maxCombinedAtomicCounterBuffers           = 1;
    resources.maxAtomicCounterBufferSize                = 16384;
    resources.maxTransformFeedbackBuffers               = 4;
    resources.maxTransformFeedbackInterleavedComponents = 64;
    resources.maxCullDistances                          = 8;
    resources.maxCombinedClipAndCullDistances           = 8;
    resources.maxSamples                                = 4;
    resources.maxMeshOutputVerticesNV                   = 256;
    resources.maxMeshOutputPrimitivesNV                 = 512;
    resources.maxMeshWorkGroupSizeX_NV                  = 32;
    resources.maxMeshWorkGroupSizeY_NV                  = 1;
    resources.maxMeshWorkGroupSizeZ_NV                  = 1;
    resources.maxTaskWorkGroupSizeX_NV                  = 32;
    resources.maxTaskWorkGroupSizeY_NV                  = 1;
    resources.maxTaskWorkGroupSizeZ_NV                  = 1;
    resources.maxMeshViewCountNV                        = 4;

    resources.limits.nonInductiveForLoops                 = 1;
    resources.limits.whileLoops                           = 1;
    resources.limits.doWhileLoops                         = 1;
    resources.limits.generalUniformIndexing               = 1;
    resources.limits.generalAttributeMatrixVectorIndexing = 1;
    resources.limits.generalVaryingIndexing               = 1;
    resources.limits.generalSamplerIndexing               = 1;
    resources.limits.generalVariableIndexing              = 1;
    resources.limits.generalConstantMatrixVectorIndexing  = 1;

    return resources;
}