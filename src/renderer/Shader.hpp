#pragma once

#include <common.hpp>

struct ShaderSources {
    const char* vertexSource;
    const char* fragmentSource;
};

class Shader {
private:
    ~Shader();

public:
    static Shader* create(const ShaderSources& sources);

    static Shader* create(const fs::path& vertexPath, const fs::path& fragmentPath);

    static inline void destroy(Shader* shader) {
        delete shader;
    }

    inline void use() {
        glUseProgram(program);
    }

    inline u32 location(const char* name) {
        return glGetUniformLocation(program, name);
    }

    void setMatrix4(u32 location, const float* data);
    inline void setMatrix4(const char* name, const float* data) {
        setMatrix4(location(name), data);
    }

    void setInt(u32 location, i32 value);
    inline void setInt(const char* name, i32 value) {
        setInt(location(name), value);
    }

    void setTexture(u32 location, i32 id, cocos2d::CCTexture2D* texture);
    inline void setTexture(const char* name, i32 id, cocos2d::CCTexture2D* texture) {
        setTexture(location(name), id, texture);
    }

    void setTextureArray(u32 location, i32 count, cocos2d::CCTexture2D** textures);
    inline void setTextureArray(const char* name, i32 count, cocos2d::CCTexture2D** textures) {
        setTextureArray(location(name), count, textures);
    }

private:
    u32 program = 0;
};