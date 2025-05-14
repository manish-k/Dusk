#pragma once

#include "dusk.h"
#include "backend/vulkan/vk_types.h"
#include "utils/utils.h"

#include <string>

namespace dusk
{
struct VkGfxDevice;

enum GfxBufferUsageFlags : uint32_t
{
    TransferSource = 0x00000001,
    TransferTarget = 0x00000002,
    UniformBuffer  = 0x00000004,
    VertexBuffer   = 0x00000008,
    IndexBuffer    = 0x00000010,
    StorageBuffer  = 0x00000012
};

enum GfxBufferMemoryTypeFlags : uint32_t
{
    PersistentlyMapped    = 0x00000001,
    HostSequentialWrite   = 0x00000002,
    HostRandomAccess      = 0x00000004,
    DedicatedDeviceMemory = 0x00000008
};

struct GfxBufferParams
{
    size_t      sizeInBytes = 0u;
    uint32_t    usage;
    uint32_t    memoryType;
    size_t      alignmentSize = 1u;
    std::string debugName;
};

struct GfxBuffer
{
    VulkanGfxBuffer vkBuffer;
    uint32_t        instanceAlignmentSize = 0u;
    uint32_t        instanceCount         = 0u;

    GfxBuffer()                           = default;
    ~GfxBuffer()                          = default;

    void init(
        uint32_t           usage,
        size_t             sizeInBytes,
        uint32_t           memoryType,
        const std::string& debugName = "");
    void                   free();
    bool                   isAllocated() const;

    void                   map();
    void                   unmap();
    void                   write(uint32_t offset, void* data, size_t size);
    void                   writeAndFlush(uint32_t offset, void* data, size_t size);
    void                   writeAtIndex(uint32_t index, void* data, size_t size);
    void                   writeAndFlushAtIndex(uint32_t index, void* data, size_t size);
    void                   flush();
    void                   flushAtIndex(uint32_t index);

    void                   copyFrom(const GfxBuffer& srcBuffer, size_t size);

    VkDescriptorBufferInfo getDescriptorInfoAtIndex(uint32_t index) const;

    static void            createHostWriteBuffer(
                   uint32_t           usage,
                   size_t             instanceSize,
                   uint32_t           instanceCount,
                   const std::string& debugName,
                   GfxBuffer*         pOutBuffer);
    static void createHostReadWriteBuffer(
        uint32_t           usage,
        size_t             instanceSize,
        uint32_t           instanceCount,
        const std::string& debugName,
        GfxBuffer*         pOutBuffer);
    static void createDeviceLocalBuffer(
        uint32_t           usage,
        size_t             instanceSize,
        uint32_t           instanceCount,
        const std::string& debugName,
        GfxBuffer*         pOutBuffer);
};

uint32_t getBufferOffsetAlignment(uint32_t usage, size_t instanceSize);
} // namespace dusk