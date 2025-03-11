#pragma once

#include "dusk.h"
#include "vertex.h"
#include "gfx_buffer.h"

namespace dusk
{
class SubMesh
{
public:
    SubMesh()  = default;
    ~SubMesh() = default;

    CLASS_UNCOPYABLE(SubMesh);

    Error init(const DynamicArray<Vertex>& vertices, const DynamicArray<uint32_t>& indices);
    void  free();

private:
    Error initGfxVertexBuffer(const DynamicArray<Vertex>& vertices);
    Error initGfxIndexBuffer(const DynamicArray<uint32_t>& indices);

private:
    Unique<VulkanGfxBuffer> m_vertexBuffer;
    uint32_t                m_vertexCount = 0u;

    Unique<VulkanGfxBuffer> m_indexBuffer;
    uint32_t                m_indexCount = 0u;
};
} // namespace dusk