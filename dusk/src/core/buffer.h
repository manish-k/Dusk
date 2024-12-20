#pragma once

#include <stdint.h>
#include <cstring>

namespace dusk
{
struct BaseBuffer
{
    uint8_t* data = nullptr;
    size_t   size = 0;

    BaseBuffer()  = default;

    BaseBuffer(size_t size)
    {
        allocate(size);
    }

    BaseBuffer(const BaseBuffer&) = default;

    static BaseBuffer copy(BaseBuffer other)
    {
        BaseBuffer result(other.size);
        memcpy(result.data, other.data, other.size);
        return result;
    }

    void allocate(size_t allocationSize)
    {
        release();

        data = new uint8_t[allocationSize];
        size = allocationSize;
    }

    void release()
    {
        delete[] data;
        data = nullptr;
        size = 0;
    }
};

struct Buffer
{
    Buffer(BaseBuffer buff) :
        m_buffer(buff)
    {
    }

    Buffer(size_t size) :
        m_buffer(size)
    {
    }

    ~Buffer()
    {
        m_buffer.release();
    }

    uint8_t* data() const { return m_buffer.data; }
    size_t   size() const { return m_buffer.size; }

    template <typename T>
    T* as()
    {
        return (T*)(m_buffer.data);
    }

private:
    BaseBuffer m_buffer;
};
} // namespace dusk