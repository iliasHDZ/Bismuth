#include "Buffer.hpp"

using namespace geode::prelude;

Buffer::~Buffer() {
    if (shouldMap) {
        bindAs(GL_ARRAY_BUFFER);
        glUnmapBuffer(GL_ARRAY_BUFFER);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    } else
        delete[] mappedData;
    glDeleteBuffers(1, &id);
}

void Buffer::read(void* data, usize size, usize offset) {
    assert(data != nullptr);
    assert((offset + size) <= this->size);

    i32 previouslyBoundBuffer;
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &previouslyBoundBuffer);

    glBindBuffer(GL_ARRAY_BUFFER, id);
    glGetBufferSubData(GL_ARRAY_BUFFER, offset, size, data);

    glBindBuffer(GL_ARRAY_BUFFER, previouslyBoundBuffer);
}

void Buffer::write(void* data, usize size, usize offset) {
    assert(data != nullptr);
    assert((offset + size) <= this->size);

    i32 previouslyBoundBuffer;
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &previouslyBoundBuffer);

    glBindBuffer(GL_ARRAY_BUFFER, id);
    glBufferSubData(GL_ARRAY_BUFFER, offset, size, data);

    glBindBuffer(GL_ARRAY_BUFFER, previouslyBoundBuffer);
}

void* Buffer::map(i32 type) {
    if (mappedData) return nullptr;

    if (shouldMap) {
        bindAs(GL_ARRAY_BUFFER);
        mappedData = (u8*)glMapBuffer(GL_ARRAY_BUFFER, type);
    } else {
        mappedData = new u8[size];
    }
    return mappedData;
}

void Buffer::unmap() {
    if (!mappedData) return;

    if (shouldMap) {
        bindAs(GL_ARRAY_BUFFER);
        glUnmapBuffer(GL_ARRAY_BUFFER);
    } else {
        delete[] mappedData;
    }
    mappedData = nullptr;
}

void Buffer::flush(usize offset, usize size) {
    if (!mappedData) return;

    if (shouldMap) {
        bindAs(GL_ARRAY_BUFFER);
        glFlushMappedBufferRange(GL_ARRAY_BUFFER, offset, size);
    } else {
        write(mappedData + offset, size, offset);
    }
}

UPtr<Buffer> Buffer::create(const char* name, usize size, GLenum usage) {
    u32 buffer;
    glGenBuffers(1, &buffer);

    glObjectLabel(GL_BUFFER, buffer, strlen(name), name);

    i32 previouslyBoundBuffer;
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &previouslyBoundBuffer);

    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferData(GL_ARRAY_BUFFER, size, NULL, usage);

    glBindBuffer(GL_ARRAY_BUFFER, previouslyBoundBuffer);

    auto ret = std::make_unique<Buffer>();
    ret->name = name;
    ret->id = buffer;
    ret->size = size;
    ret->shouldMap = Mod::get()->getSettingValue<bool>("use_mapped_buffers");

    return ret;
}