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
    auto& device = Engine::get().getGfxDevice();

    device.freeBuffer(&m_indexBuffer);

    device.freeBuffer(&m_vertexBuffer);
}

Error SubMesh::initGfxVertexBuffer(const DynamicArray<Vertex>& vertices)
{
    auto& device = Engine::get().getGfxDevice();

    // vertex info
    uint32_t vertexSize     = sizeof(vertices[0]);
    m_vertexCount           = vertices.size();
    VkDeviceSize bufferSize = vertexSize * m_vertexCount;

    // staging buffer params for creation
    GfxBufferParams stagingBufferParams {};
    stagingBufferParams.sizeInBytes = bufferSize;
    stagingBufferParams.usage       = GfxBufferUsageFlags::TransferSource;
    stagingBufferParams.memoryType  = GfxBufferMemoryTypeFlags::HostSequentialWrite | GfxBufferMemoryTypeFlags::PersistentlyMapped;

    // create staging
    VulkanGfxBuffer stagingBuffer {};

    VulkanResult    result = device.createBuffer(stagingBufferParams, &stagingBuffer);
    if (result.hasError())
    {
        DUSK_ERROR("Unable to create staging buffer for uploading vertex data");
        return Error::OutOfMemory;
    }

    memcpy(stagingBuffer.mappedMemory, vertices.data(), bufferSize);

    // device vertex buffer
    GfxBufferParams vertexBufferParams {};
    vertexBufferParams.sizeInBytes = bufferSize;
    vertexBufferParams.usage       = GfxBufferUsageFlags::VertexBuffer | GfxBufferUsageFlags::TransferTarget;
    vertexBufferParams.memoryType  = GfxBufferMemoryTypeFlags::DedicatedDeviceMemory;

    // create vertex buffer
    result = device.createBuffer(vertexBufferParams, &m_vertexBuffer);
    if (result.hasError())
    {
        DUSK_ERROR("Unable to create device local vertex buffer for uploading vertex data");
        device.freeBuffer(&stagingBuffer);
        // TODO: figure out correct error type
        return Error::OutOfMemory;
    }

    // copy buffer
    device.copyBuffer(stagingBuffer.buffer, m_vertexBuffer.buffer, bufferSize);

    device.freeBuffer(&stagingBuffer);

    return Error::Ok;
}

Error SubMesh::initGfxIndexBuffer(const DynamicArray<uint32_t>& indices)
{
    auto& device = Engine::get().getGfxDevice();

    // vertex info
    uint32_t indexSize      = sizeof(indices[0]);
    m_indexCount            = indices.size();
    VkDeviceSize bufferSize = indexSize * m_indexCount;

    // staging buffer params for creation
    GfxBufferParams stagingBufferParams {};
    stagingBufferParams.sizeInBytes = bufferSize;
    stagingBufferParams.usage       = GfxBufferUsageFlags::TransferSource;
    stagingBufferParams.memoryType  = GfxBufferMemoryTypeFlags::HostSequentialWrite | GfxBufferMemoryTypeFlags::PersistentlyMapped;

    // create staging
    VulkanGfxBuffer stagingBuffer {};
    VulkanResult    result = device.createBuffer(stagingBufferParams, &stagingBuffer);
    if (result.hasError())
    {
        DUSK_ERROR("Unable to create staging buffer for uploading indices data");
        return Error::OutOfMemory;
    }

    memcpy(stagingBuffer.mappedMemory, indices.data(), bufferSize);

    // device vertex buffer
    GfxBufferParams indexBufferParams {};
    indexBufferParams.sizeInBytes = bufferSize;
    indexBufferParams.usage       = GfxBufferUsageFlags::IndexBuffer | GfxBufferUsageFlags::TransferTarget;
    indexBufferParams.memoryType  = GfxBufferMemoryTypeFlags::DedicatedDeviceMemory;

    // create vertex buffer
    result = device.createBuffer(indexBufferParams, &m_indexBuffer);
    if (result.hasError())
    {
        DUSK_ERROR("Unable to create device local index buffer for uploading indices data");
        return Error::OutOfMemory;
    }

    // copy buffer
    device.copyBuffer(stagingBuffer.buffer, m_indexBuffer.buffer, bufferSize);

    device.freeBuffer(&stagingBuffer);

    return Error::Ok;
}

} // namespace dusk