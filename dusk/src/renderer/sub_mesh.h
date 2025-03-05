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

    Error init(DynamicArray<Vertex>& vertices, DynamicArray<uint32_t>& indices);

private:
    Error initGfxVertexBuffer(DynamicArray<Vertex>& vertices);
    Error initGfxIndexBuffer(DynamicArray<uint32_t>& indices);

private:
    GfxBuffer m_vertexBuffer;
    uint32_t  m_vertexCount = 0u;

    GfxBuffer m_indexBuffer;
    uint32_t  m_indexCount = 0u;
};
} // namespace dusk