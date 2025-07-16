#include "sub_mesh.h"
#include "engine.h"
#include "backend/vulkan/vk_allocator.h"

namespace dusk
{
Error SubMesh::init(const DynamicArray<Vertex>& vertices, const DynamicArray<uint32_t>& indices)
{
    Error err = initGfxVertexBuffer(vertices);
    if (err != Error::Ok)
    {
        return err;
    }

    err = initGfxIndexBuffer(indices);
    if (err != Error::Ok)
    {
        return err;
    }

    return Error();
}

void SubMesh::free()
{
    m_indexBuffer.free();
    m_vertexBuffer.free();
}

Error SubMesh::initGfxVertexBuffer(const DynamicArray<Vertex>& vertices)
{
    // vertex info
    uint32_t vertexSize     = sizeof(vertices[0]);
    m_vertexCount           = vertices.size();
    VkDeviceSize bufferSize = vertexSize * m_vertexCount;

    // staging buffer params for creation
    GfxBuffer stagingBuffer;
    GfxBuffer::createHostWriteBuffer(
        GfxBufferUsageFlags::TransferSource,
        bufferSize,
        1,
        "staging_vertex_buffer",
        &stagingBuffer);

    if (!stagingBuffer.isAllocated())
        return Error::InitializationFailed;

    stagingBuffer.writeAndFlush(0, (void*)vertices.data(), bufferSize);

    // device vertex buffer
    m_vertexBuffer.init(
        GfxBufferUsageFlags::VertexBuffer | GfxBufferUsageFlags::TransferTarget,
        bufferSize,
        GfxBufferMemoryTypeFlags::DedicatedDeviceMemory,
        "vertex_buffer");

    if (!m_vertexBuffer.isAllocated())
        return Error::InitializationFailed;

    // copy buffer
    m_vertexBuffer.copyFrom(stagingBuffer, bufferSize);

    stagingBuffer.free();

    return Error::Ok;
}

Error SubMesh::initGfxIndexBuffer(const DynamicArray<uint32_t>& indices)
{
    // vertex info
    uint32_t indexSize      = sizeof(indices[0]);
    m_indexCount            = indices.size();
    VkDeviceSize bufferSize = indexSize * m_indexCount;

    // staging buffer params for creation
    GfxBuffer stagingBuffer;
    GfxBuffer::createHostWriteBuffer(
        GfxBufferUsageFlags::TransferSource,
        bufferSize,
        1,
        "staging_index_buffer",
        &stagingBuffer);

    if (!stagingBuffer.isAllocated())
        return Error::InitializationFailed;

    stagingBuffer.writeAndFlush(0, (void*)indices.data(), bufferSize);

    // device index buffer
    m_indexBuffer.init(
        GfxBufferUsageFlags::IndexBuffer | GfxBufferUsageFlags::TransferTarget,
        bufferSize,
        GfxBufferMemoryTypeFlags::DedicatedDeviceMemory,
        "index_buffer");

    if (!m_indexBuffer.isAllocated())
        return Error::InitializationFailed;

    // copy buffer
    m_indexBuffer.copyFrom(stagingBuffer, bufferSize);

    stagingBuffer.free();

    return Error::Ok;
}

Shared<SubMesh> SubMesh::createCubeMesh()
{
    auto cubeMesh = createShared<SubMesh>();

    // load cube mesh
    DynamicArray<Vertex> skyboxVertices = {
        // positions
        { glm::vec3(-1.0f, -1.0f, -1.0f) }, // 0
        { glm::vec3(1.0f, -1.0f, -1.0f) },  // 1
        { glm::vec3(1.0f, 1.0f, -1.0f) },   // 2
        { glm::vec3(-1.0f, 1.0f, -1.0f) },  // 3
        { glm::vec3(-1.0f, -1.0f, 1.0f) },  // 4
        { glm::vec3(1.0f, -1.0f, 1.0f) },   // 5
        { glm::vec3(1.0f, 1.0f, 1.0f) },    // 6
        { glm::vec3(-1.0f, 1.0f, 1.0f) }    // 7
    };

    DynamicArray<uint32_t> skyboxIndices = {
        // Back face
        0,
        1,
        2,
        2,
        3,
        0,
        // Front face
        4,
        7,
        6,
        6,
        5,
        4,
        // Left face
        0,
        3,
        7,
        7,
        4,
        0,
        // Right face
        1,
        5,
        6,
        6,
        2,
        1,
        // Top face
        3,
        2,
        6,
        6,
        7,
        3,
        // Bottom face
        0,
        4,
        5,
        5,
        1,
        0
    };

    cubeMesh->init(skyboxVertices, skyboxIndices);

    return cubeMesh;
}

} // namespace dusk