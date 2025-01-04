#pragma once

#include "dusk.h"
#include "vk.h"
#include "vk_types.h"

#include <volk/volk.h>
#include <vk_mem_alloc.h>

namespace dusk
{

// TODO:: check if synchronization is required for vmaAllocation access
struct VulkanGPUAllocator
{
    VmaAllocator vmaAllocator;
};

struct VulkanGfxBuffer
{
    VkBuffer              buffer;
    VmaAllocation         allocation;
    void*                 mappedMemory;
    size_t                sizeInBytes;
    VkMemoryPropertyFlags memoryFlags;
};

namespace vulkan
{
VulkanResult createGPUAllocator(const VulkanContext& context, VulkanGPUAllocator& gpuAllocator);
void         destroyGPUAllocator(VulkanGPUAllocator& gpuAllocator);

VulkanResult allocateGPUBuffer(VulkanGPUAllocator& gpuAllocator, const VkBufferCreateInfo& bufferCreateInfo, VmaMemoryUsage usage, VmaAllocationCreateFlags flags, VulkanGfxBuffer* bufferResult);
void         freeGPUBuffer(VulkanGPUAllocator& gpuAllocator, VulkanGfxBuffer* gfxBuffer);

VulkanResult mapGPUMemory(VulkanGPUAllocator& gpuAllocator, VmaAllocation allocation, void* pMappedBlock);
void         unmapGPUMemory(VulkanGPUAllocator& gpuAllocator, VmaAllocation allocation);

VulkanResult flushCPUMemory(VulkanGPUAllocator& gpuAllocator, DynamicArray<VmaAllocation> allocations, DynamicArray<VkDeviceSize> offsets, DynamicArray<VkDeviceSize> sizes);
VulkanResult invalidateCPUMemory(VulkanGPUAllocator& gpuAllocator, DynamicArray<VmaAllocation> allocations, DynamicArray<VkDeviceSize> offsets, DynamicArray<VkDeviceSize> sizes);

VulkanResult writeToAllocation(VulkanGPUAllocator& gpuAllocator, const void* pSrcHostBlock, VmaAllocation dstAllocation, VkDeviceSize dstOffset, VkDeviceSize size);
} // namespace vulkan
} // namespace dusk