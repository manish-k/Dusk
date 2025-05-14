#include "gfx_buffer.h"

#include "engine.h"

namespace dusk
{
void GfxBuffer::createHostWriteBuffer(
    uint32_t           usage,
    size_t             instanceSize,
    uint32_t           instanceCount,
    const std::string& debugName,
    GfxBuffer*         pOutBuffer)
{
    DASSERT(pOutBuffer, "Not a valid pointer");
    DASSERT(!pOutBuffer->isAllocated(), "Allocation already exists");

    pOutBuffer->instanceAlignmentSize = getBufferOffsetAlignment(usage, instanceSize);
    pOutBuffer->instanceCount         = instanceCount;

    pOutBuffer->init(
        usage,
        pOutBuffer->instanceCount * pOutBuffer->instanceAlignmentSize,
        GfxBufferMemoryTypeFlags::HostSequentialWrite,
        debugName);
}

void GfxBuffer::createHostReadWriteBuffer(
    uint32_t           usage,
    size_t             instanceSize,
    uint32_t           instanceCount,
    const std::string& debugName,
    GfxBuffer*         pOutBuffer)
{
    DASSERT(pOutBuffer, "Not a valid pointer");
    DASSERT(!pOutBuffer->isAllocated(), "Allocation already exists");

    pOutBuffer->instanceAlignmentSize = getBufferOffsetAlignment(usage, instanceSize);
    pOutBuffer->instanceCount         = instanceCount;

    pOutBuffer->init(
        usage,
        pOutBuffer->instanceCount * pOutBuffer->instanceAlignmentSize,
        GfxBufferMemoryTypeFlags::HostRandomAccess,
        debugName);
}

void GfxBuffer::createDeviceLocalBuffer(
    uint32_t           usage,
    size_t             instanceSize,
    uint32_t           instanceCount,
    const std::string& debugName,
    GfxBuffer*         pOutBuffer)
{
    DASSERT(pOutBuffer, "Not a valid pointer");
    DASSERT(!pOutBuffer->isAllocated(), "Allocation already exists");

    pOutBuffer->instanceAlignmentSize = getBufferOffsetAlignment(usage, instanceSize);
    pOutBuffer->instanceCount         = instanceCount;

    pOutBuffer->init(
        usage,
        pOutBuffer->instanceCount * pOutBuffer->instanceAlignmentSize,
        GfxBufferMemoryTypeFlags::DedicatedDeviceMemory,
        debugName);
}

void GfxBuffer::init(
    uint32_t           usage,
    size_t             sizeInBytes,
    uint32_t           memoryType,
    const std::string& debugName)
{
    GfxBufferParams uboParams {};
    uboParams.sizeInBytes = sizeInBytes;
    uboParams.usage       = usage;
    uboParams.memoryType  = memoryType;
    uboParams.debugName   = debugName; // TODO: string copy

    Engine::get().getGfxDevice().createBuffer(uboParams, &vkBuffer);
}

void GfxBuffer::free()
{
    Engine::get().getGfxDevice().freeBuffer(&vkBuffer);
}

bool GfxBuffer::isAllocated() const
{
    if (!vkBuffer.allocation) return false;
    return true;
}

void GfxBuffer::map()
{
    Engine::get().getGfxDevice().mapBuffer(&vkBuffer);
}

void GfxBuffer::unmap()
{
    Engine::get().getGfxDevice().unmapBuffer(&vkBuffer);
}

void GfxBuffer::write(uint32_t offset, void* data, size_t size)
{
    map();
    memcpy((char*)vkBuffer.mappedMemory + offset, data, size);
    unmap();
}

void GfxBuffer::writeAndFlush(uint32_t offset, void* data, size_t size)
{
    Engine::get().getGfxDevice().writeToBuffer(&vkBuffer, data, offset, size);
}

void GfxBuffer::writeAtIndex(uint32_t index, void* data, size_t size)
{
    map();
    memcpy((char*)vkBuffer.mappedMemory + index * instanceAlignmentSize, data, size);
    unmap();
}

void GfxBuffer::writeAndFlushAtIndex(uint32_t index, void* data, size_t size)
{
    DASSERT(index < instanceCount);
    Engine::get().getGfxDevice().writeToBuffer(&vkBuffer, data, index * instanceAlignmentSize, size);
}

void GfxBuffer::flush()
{
    Engine::get().getGfxDevice().flushBuffer(&vkBuffer);
}

void GfxBuffer::flushAtIndex(uint32_t index)
{
    DASSERT(index < instanceCount);
    Engine::get().getGfxDevice().flushBufferOffset(&vkBuffer, index * instanceAlignmentSize, instanceAlignmentSize);
}

VkDescriptorBufferInfo GfxBuffer::getDescriptorInfoAtIndex(uint32_t index) const
{
    VkDescriptorBufferInfo bufferInfo {};
    bufferInfo.buffer = vkBuffer.buffer;
    bufferInfo.offset = instanceAlignmentSize * index;
    bufferInfo.range  = instanceAlignmentSize;

    return bufferInfo;
}

uint32_t getBufferOffsetAlignment(uint32_t usage, size_t instanceSize)
{
    uint32_t      minOffsetAlignment = 1;
    VulkanContext ctx                = VkGfxDevice::getSharedVulkanContext();

    if (usage & GfxBufferUsageFlags::UniformBuffer)
    {
        minOffsetAlignment = ctx.physicalDeviceProperties.limits.minUniformBufferOffsetAlignment;
    }
    else if (usage & GfxBufferUsageFlags::StorageBuffer)
    {
        minOffsetAlignment = ctx.physicalDeviceProperties.limits.minStorageBufferOffsetAlignment;
    }

    return getAlignment(instanceSize, minOffsetAlignment);
}

} // namespace dusk