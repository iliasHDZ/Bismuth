#pragma once

#include <common.hpp>
#include "Shader.hpp"

class Buffer {
public:
    ~Buffer();

    inline usize getSize() const { return size; }
    inline u32 getId() const { return id; }

    void read(void* data, usize size, usize offset = 0);

    void write(void* data, usize size, usize offset = 0);

    template <typename T>
    void write(std::span<T> span) {
        write(span.data(), span.size_bytes());
    }

    template <typename T>
    void write(const std::vector<T>& vector) {
        write(std::span<T>(vector));
    }

    inline void bindAs(GLenum binding) {
        glBindBuffer(binding, id);
    }

    inline void bindAsUniformBuffer(u32 binding) {
        glBindBufferBase(GL_UNIFORM_BUFFER, binding, id);
    }

    inline void bindAsStorageBuffer(u32 binding) {
        // geode::log::info("SHADER STORAGE BUFFER, {}, {}", binding, byteSizeToString(size));
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding, id);
    }

    inline void bindAsStorageBuffer(u32 binding, u32 offset, u32 size) {
        glBindBufferRange(GL_SHADER_STORAGE_BUFFER, binding, id, offset, size);
    }

    void* map(i32 type);

    inline void* mapWriteOnly() {
        return map(GL_WRITE_ONLY);
    }

    inline void* mapReadWrite() {
        return map(GL_READ_WRITE);
    }

    void unmap();

    void flush(usize offset, usize size);

public:
    static UPtr<Buffer> create(const char* name, usize size, GLenum usage);

    /*
        Buffer to be written to once by the CPU
        and used by the GPU
    */
    inline static UPtr<Buffer> createStaticDraw(const char* name, void* data, usize size) {
        auto ret = create(name, size, GL_STATIC_DRAW);
        if (data)
            ret->write(data, size);
        return ret;
    }

    /*
        Buffer to be written to many times repeatedly by the CPU
        and used by the GPU
    */
    inline static UPtr<Buffer> createDynamicDraw(const char* name, usize size) {
        return create(name, size, GL_DYNAMIC_DRAW);
    }

    /*
        Buffer to be written to many times repeatedly by the GPU
        and used by the CPU
    */
    inline static UPtr<Buffer> createDynamicCopy(const char* name, usize size) {
        return create(name, size, GL_DYNAMIC_COPY);
    }

    /*
        Buffer to be written to and read from by the GPU exclusively
    */
    static UPtr<Buffer> createGPUExclusive(const char* name, usize size) {
        return create(name, size, GL_DYNAMIC_DRAW); // GL_DYNAMIC_DRAW for now
    }

private:
    const char* name;
    usize size = 0;
    u32 id = 0;
    bool shouldMap = true;
    u8* mappedData = nullptr;
};