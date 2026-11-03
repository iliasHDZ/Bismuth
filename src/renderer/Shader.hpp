#pragma once

#include <common.hpp>

#include <string>
#include <map>

struct ShaderSources {
    std::string vertexSource;
    std::string fragmentSource;
};

class Shader {
public:
    ~Shader();

    static UPtr<Shader> create(
        const fs::path& vertexPath,
        const fs::path& fragmentPath,
        std::map<std::string, std::string> macroVariables = {}
    );

    static UPtr<Shader> createCompute(const fs::path& path, std::map<std::string, std::string> macroVariables = {});

    inline bool isInUse() const {
        i32 boundProgram;
        glGetIntegerv(GL_CURRENT_PROGRAM, &boundProgram);
        return boundProgram == program;
    }

    inline void use() {
        cocos2d::ccGLUseProgram(program);
    }

    inline u32 location(const char* name) {
        return glGetUniformLocation(program, name);
    }

    u32 getBufferBinding(const char* name, bool ssbo);

    void setMatrix4(u32 location, const float* data);
    inline void setMatrix4(const char* name, const float* data) {
        setMatrix4(location(name), data);
    }

    void setInt(u32 location, i32 value);
    inline void setInt(const char* name, i32 value) {
        setInt(location(name), value);
    }

    void setUInt(u32 location, u32 value);
    inline void setUInt(const char* name, u32 value) {
        setUInt(location(name), value);
    }

    void setFloat(u32 location, float value);
    inline void setFloat(const char* name, float value) {
        setFloat(location(name), value);
    }

    void setVec2(u32 location, glm::vec2 value);
    inline void setVec2(const char* name, glm::vec2 value) {
        setVec2(location(name), value);
    }

    void setVec3(u32 location, glm::vec3 value);
    inline void setVec3(const char* name, glm::vec3 value) {
        setVec3(location(name), value);
    }

    void setVec4(u32 location, glm::vec4 value);
    inline void setVec4(const char* name, glm::vec4 value) {
        setVec4(location(name), value);
    }

    void setTexture(u32 location, i32 id, u32 texture);
    inline void setTexture(u32 location, i32 id, cocos2d::CCTexture2D* texture) {
        setTexture(location, id, texture->getName());
    }
    inline void setTexture(const char* name, i32 id, cocos2d::CCTexture2D* texture) {
        setTexture(location(name), id, texture);
    }

    void setTextureArray(u32 location, i32 count, u32* textures);
    void setTextureArray(u32 location, i32 count, cocos2d::CCTexture2D** textures);
    inline void setTextureArray(const char* name, i32 count, cocos2d::CCTexture2D** textures) {
        setTextureArray(location(name), count, textures);
    }

private:
    struct BufferBinding {
        u32 binding;
        u32 index;
    };

    u32 program = 0;
    std::map<std::string, BufferBinding> bufferBindings;
    u32 bufferBindingCounter = 0;
};