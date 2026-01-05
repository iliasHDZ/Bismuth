#pragma once

#include <common.hpp>

#define GL_SHADER_STORAGE_BUFFER 0x90D2

class Buffer {
public:
    ~Buffer();

    inline usize getSize() const { return size; }
    inline u32 getId() const { return id; }

    void read(void* data, usize size, usize offset = 0);

    void write(void* data, usize size, usize offset = 0);

    inline void bindAs(GLenum binding) {
        glBindBuffer(binding, id);
    }

    inline void bindAsUniformBuffer(u32 binding) {
        glBindBufferBase(GL_UNIFORM_BUFFER, binding, id);
    }

    inline void bindAsStorageBuffer(u32 binding) {
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding, id);
    }

public:
    static Buffer* create(usize size, GLenum usage);

    inline static void destroy(Buffer* buffer) {
        delete buffer;
    }

    /*
        Buffer to be written to once by the CPU
        and used by the GPU
    */
    inline static Buffer* createStaticDraw(void* data, usize size) {
        auto ret = create(size, GL_STATIC_DRAW);
        ret->write(data, size);
        return ret;
    }

    /*
        Buffer to be written to many times repeatedly by the CPU
        and used by the GPU
    */
    inline static Buffer* createDynamicDraw(usize size) {
        return create(size, GL_DYNAMIC_DRAW);
    }

    /*
        Buffer to be written to many times repeatedly by the GPU
        and used by the CPU
    */
    inline static Buffer* createDynamicCopy(usize size) {
        return create(size, GL_DYNAMIC_COPY);
    }

private:
    usize size = 0;
    u32 id = 0;
};