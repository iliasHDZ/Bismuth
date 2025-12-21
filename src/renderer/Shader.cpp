#include "Shader.hpp"

#include <Geode/Geode.hpp>
#include <vector>

using namespace geode;

Shader::~Shader() {
    if (program)
        glDeleteProgram(program);
}

void printErrorLog(char* log) {
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

static u32 createShader(i32 type, const char* source) {
    u32 shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    i32 success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[512];
        glGetShaderInfoLog(shader, 512, NULL, log);
        const char* shaderType = (type == GL_FRAGMENT_SHADER) ? "fragment" : "vertex";
        geode::log::error("Failed to compile {} shader:", shaderType);
        printErrorLog(log);
        return 0;
    };

    return shader;
}

Shader* Shader::create(const ShaderSources& sources) {
    u32 vertexShader   = createShader(GL_VERTEX_SHADER, sources.vertexSource);
    u32 fragmentShader = createShader(GL_FRAGMENT_SHADER, sources.fragmentSource);

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

// TODO: Do error checking if it failed to load!
std::string readFile(const fs::path& path) {
    std::ifstream t(path);
    t.seekg(0, std::ios::end);
    size_t size = t.tellg();
    std::string buffer(size, ' ');
    t.seekg(0);
    t.read(buffer.data(), size);
    t.close();

    return buffer;
}

Shader* Shader::create(const fs::path& vertexPath, const fs::path& fragmentPath) {
    auto vertexRawPath   = Mod::get()->getResourcesDir() / vertexPath;
    auto fragmentRawPath = Mod::get()->getResourcesDir() / fragmentPath;

    std::string vertexSource   = readFile(vertexRawPath);
    std::string fragmentSource = readFile(fragmentRawPath);

    return Shader::create({ vertexSource.c_str(), fragmentSource.c_str() });
}

void Shader::setMatrix4(u32 location, const float* data) {
    use();
    glUniformMatrix4fv(location, 1, GL_FALSE, data);
}

void Shader::setInt(u32 location, i32 value) {
    use();
    glUniform1i(location, value);
}

void Shader::setTexture(u32 location, i32 id, cocos2d::CCTexture2D* texture) {
    use();
    setInt(location, id);
    glActiveTexture(GL_TEXTURE0 + id);
    glBindTexture(GL_TEXTURE_2D, texture->getName());
}