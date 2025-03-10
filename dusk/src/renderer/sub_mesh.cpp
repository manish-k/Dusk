#include "sub_mesh.h"
#include "backend/vulkan/vk_allocator.h"

#include "engine.h"
#include "render_api.h"

namespace dusk
{
Error SubMesh::init(DynamicArray<Vertex>& vertices, DynamicArray<uint32_t>& indices)
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

Error SubMesh::initGfxVertexBuffer(DynamicArray<Vertex>& vertices)
{
    auto& device = Engine::get().getGfxDevice();

    // vertex info
    uint32_t     vertexSize = sizeof(vertices[0]);
    VkDeviceSize bufferSize = vertexSize * vertices.size();

    // staging buffer params for creation
    GfxBufferParams stagingBufferParams {};
    stagingBufferParams.sizeInBytes = bufferSize;
    stagingBufferParams.usage       = GfxBufferUsageFlags::TransferSource;
    stagingBufferParams.memoryType  = GfxBufferMemoryTypeFlags::HostSequentialWrite | GfxBufferMemoryTypeFlags::PersistentlyMapped;

    // create staging
    Unique<VulkanGfxBuffer> stagingBuffer = device.createBuffer(stagingBufferParams);
    if (!stagingBuffer)
    {
        DUSK_ERROR("Unable to create staging buffer for uploading vertex data");
        return Error::OutOfMemory;
    }

    memcpy(stagingBuffer->mappedMemory, vertices.data(), bufferSize);

    // device vertex buffer
    GfxBufferParams vertexBufferParams {};
    vertexBufferParams.sizeInBytes = bufferSize;
    vertexBufferParams.usage       = GfxBufferUsageFlags::VertexBuffer | GfxBufferUsageFlags::TransferTarget;
    vertexBufferParams.memoryType  = GfxBufferMemoryTypeFlags::DedicatedDeviceMemory;

    // create vertex buffer
    m_vertexBuffer = device.createBuffer(vertexBufferParams);
    if (!m_vertexBuffer)
    {
        DUSK_ERROR("Unable to create device local vertex buffer for uploading vertex data");
        return Error::OutOfMemory;
    }

    // copy buffer

    return Error::Ok;
}

Error SubMesh::initGfxIndexBuffer(DynamicArray<uint32_t>& indices)
{
    auto& device = Engine::get().getGfxDevice();

    // vertex info
    uint32_t     indexSize  = sizeof(indices[0]);
    VkDeviceSize bufferSize = indexSize * indices.size();

    // staging buffer params for creation
    GfxBufferParams stagingBufferParams {};
    stagingBufferParams.sizeInBytes = bufferSize;
    stagingBufferParams.usage       = GfxBufferUsageFlags::TransferSource;
    stagingBufferParams.memoryType  = GfxBufferMemoryTypeFlags::HostSequentialWrite | GfxBufferMemoryTypeFlags::PersistentlyMapped;

    // create staging
    Unique<VulkanGfxBuffer> stagingBuffer = device.createBuffer(stagingBufferParams);
    if (!stagingBuffer)
    {
        DUSK_ERROR("Unable to create staging buffer for uploading indices data");
        return Error::OutOfMemory;
    }

    memcpy(stagingBuffer->mappedMemory, indices.data(), bufferSize);

    // device vertex buffer
    GfxBufferParams indexBufferParams {};
    indexBufferParams.sizeInBytes = bufferSize;
    indexBufferParams.usage       = GfxBufferUsageFlags::IndexBuffer | GfxBufferUsageFlags::TransferTarget;
    indexBufferParams.memoryType  = GfxBufferMemoryTypeFlags::DedicatedDeviceMemory;

    // create vertex buffer
    m_indexBuffer = device.createBuffer(indexBufferParams);
    if (!m_indexBuffer)
    {
        DUSK_ERROR("Unable to create device local index buffer for uploading indices data");
        return Error::OutOfMemory;
    }

    // copy buffer

    return Error::Ok;
}

} // namespace dusk