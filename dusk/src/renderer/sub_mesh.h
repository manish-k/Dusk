#pragma once

#include "dusk.h"
#include "vertex.h"
#include "gfx_buffer.h"
#include "geometry/aabb.h"

namespace dusk
{
class SubMesh
{
public:
    SubMesh()  = default;
    ~SubMesh() = default;

    CLASS_UNCOPYABLE(SubMesh);

    Error      init(const DynamicArray<Vertex>& vertices, const DynamicArray<uint32_t>& indices);
    void       cleanup();

    GfxBuffer& getVertexBuffer() { return m_vertexBuffer; };
    uint32_t   getVertexCount() const { return m_vertexCount; };
    GfxBuffer& getIndexBuffer() { return m_indexBuffer; };
    uint32_t   getIndexCount() const { return m_indexCount; };

    int32_t    getVertexOffset() const { return m_globalVertexOffset; };
    uint32_t   getIndexBufferIndex() const { return static_cast<uint32_t>(m_globalIndexOffset); };

    const AABB& getAABB() const { return m_aabb; };

public:
    static Shared<SubMesh> createCubeMesh();

private:
    Error initGfxVertexBuffer(const DynamicArray<Vertex>& vertices);
    Error initGfxIndexBuffer(const DynamicArray<uint32_t>& indices);

private:
    GfxBuffer m_vertexBuffer {};
    uint32_t  m_vertexCount        = 0u;
    int32_t   m_globalVertexOffset = 0u;

    GfxBuffer m_indexBuffer {};
    uint32_t  m_indexCount        = 0u;
    size_t    m_globalIndexOffset = 0u;

    AABB      m_aabb              = {};
};

} // namespace dusk