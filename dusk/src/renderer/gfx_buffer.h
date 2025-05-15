#pragma once

#include "dusk.h"
#include "backend/vulkan/vk_types.h"

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

/**
 * @brief struct for buffer creation params
 */
struct GfxBufferParams
{
    size_t      sizeInBytes = 0u;
    uint32_t    usage;
    uint32_t    memoryType;
    size_t      alignmentSize = 1u;
    std::string debugName;
};

/**
 * @brief Class to manage graphics buffer object
 */
struct GfxBuffer
{
    VulkanGfxBuffer vkBuffer;
    uint32_t        instanceAlignmentSize = 0u;
    uint32_t        instanceCount         = 0u;

    GfxBuffer()                           = default;
    ~GfxBuffer()                          = default;

    /**
     * @brief initialize the empty buffer
     * @param usage type of the buffer
     * @param total size in bytes for allocation
     * @param type of memory
     * @param debugName
     */
    void init(
        uint32_t           usage,
        size_t             sizeInBytes,
        uint32_t           memoryType,
        const std::string& debugName = "");

    /**
     * @brief release the resources of the buffer
     */
    void free();

    /**
     * @brief Check if buffer was allocated
     * @return true if allocated else false
     */
    bool isAllocated() const;

    /**
     * @brief map the buffer to host memory
     */
    void map();

    /**
     * @brief unmap host memory
     */
    void unmap();

    /**
     * @brief write data in the mapped buffer at a particular offset
     * @param offset to start of the writing location
     * @param source pointer to read from
     * @param total size to write
     */
    void write(uint32_t offset, void* data, size_t size);

    /**
     * @brief Write data in the mapped buffer and immediately flushes into the device buffer
     * @param offset to start of the writing location
     * @param source pointer to read from
     * @param total size to write
     */
    void writeAndFlush(uint32_t offset, void* data, size_t size);

    /**
     * @brief Write data in the mapped buffer at a particular index(if buffer
     * is being used as an array)
     * @param index of the writing location
     * @param source pointer to read from
     * @param total size to write
     */
    void writeAtIndex(uint32_t index, void* data, size_t size);

    /**
     * @brief Write data in the mapped buffer at a particular index(if buffer
     * is being used as an array) and immediately flushes into the device buffer
     * @param index of the writing location
     * @param source pointer to read from
     * @param total size to write
     */
    void writeAndFlushAtIndex(uint32_t index, void* data, size_t size);

    /**
     * @brief Flush all the pending writes in mapped block into the device buffer
     */
    void flush();

    /**
     * @brief Flush all the pending writes in mapped block at a particular index into the device buffer
     * @param index of the writing location
     */
    void flushAtIndex(uint32_t index);

    /**
     * @brief copy the data from source buffer into the current buffer
     * @param source buffer
     * @param total size to write from source buffer
     */
    void copyFrom(const GfxBuffer& srcBuffer, size_t size);

    /**
     * @brief Get descriptor info for writing into the descriptor set for the
     * current buffer
     * @param index for which descriptor info is required
     * @return descriptor buffer info of the indexed instance data
     */
    VkDescriptorBufferInfo getDescriptorInfoAtIndex(uint32_t index) const;

    /**
     * @brief Create buffer to be used by host to write to
     * @param usage type of the buffer
     * @param size of a particular instance
     * @param total instances to allocate
     * @param debugName
     * @param pointer to the buffer to allocate
     */
    static void createHostWriteBuffer(
        uint32_t           usage,
        size_t             instanceSize,
        uint32_t           instanceCount,
        const std::string& debugName,
        GfxBuffer*         pOutBuffer);

    /**
     * @brief Create buffer to be used by host to read from and write to
     * @param usage type of the buffer
     * @param size of a particular instance
     * @param total instances to allocate
     * @param debugName
     * @param pointer to the buffer to allocate
     */
    static void createHostReadWriteBuffer(
        uint32_t           usage,
        size_t             instanceSize,
        uint32_t           instanceCount,
        const std::string& debugName,
        GfxBuffer*         pOutBuffer);

    /**
     * @brief Create buffer to be used exclusively by the device only
     * @param usage type of the buffer
     * @param size of a particular instance
     * @param total instances to allocate
     * @param debugName
     * @param pointer to the buffer to allocate
     */
    static void createDeviceLocalBuffer(
        uint32_t           usage,
        size_t             instanceSize,
        uint32_t           instanceCount,
        const std::string& debugName,
        GfxBuffer*         pOutBuffer);
};

/**
 * @brief Calculate alignment for each instances as per vulkan limits
 * @param usage type of the buffer
 * @param size of a particular instance
 * @return alignmentsize of the instance
 */
uint32_t getBufferOffsetAlignment(uint32_t usage, size_t instanceSize);
} // namespace dusk