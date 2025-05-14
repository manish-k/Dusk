#pragma once

#include "dusk.h"
#include "vk.h"
#include "vk_types.h"

#include <volk/volk.h>
#include <string>

namespace dusk
{

namespace vulkan
{
VulkanResult createGPUAllocator(const VulkanContext& context, VulkanGPUAllocator& gpuAllocator);
void         destroyGPUAllocator(VulkanGPUAllocator& gpuAllocator);

VulkanResult allocateGPUBuffer(
    VulkanGPUAllocator&       gpuAllocator,
    const VkBufferCreateInfo& bufferCreateInfo,
    VmaMemoryUsage            usage,
    VmaAllocationCreateFlags  flags,
    VulkanGfxBuffer*          pBufferResult);

void freeGPUBuffer(
    VulkanGPUAllocator& gpuAllocator,
    VulkanGfxBuffer*    pGfxBuffer);

VulkanResult mapGPUMemory(
    VulkanGPUAllocator& gpuAllocator,
    VmaAllocation       allocation,
    void*               pMappedBlock);
void unmapGPUMemory(
    VulkanGPUAllocator& gpuAllocator,
    VmaAllocation       allocation);

VulkanResult allocateGPUImage(
    VulkanGPUAllocator&      gpuAllocator,
    const VkImageCreateInfo& imageCreateInfo,
    VmaMemoryUsage           usage,
    VmaAllocationCreateFlags flags,
    VulkanGfxImage*          pImageResult);

void freeGPUImage(
    VulkanGPUAllocator& gpuAllocator,
    VulkanGfxImage*     pGfxImage);

VulkanResult flushCPUMemory(
    VulkanGPUAllocator&         gpuAllocator,
    DynamicArray<VmaAllocation> allocations,
    DynamicArray<VkDeviceSize>  offsets,
    DynamicArray<VkDeviceSize>  sizes);

VulkanResult invalidateCPUMemory(
    VulkanGPUAllocator&         gpuAllocator,
    DynamicArray<VmaAllocation> allocations,
    DynamicArray<VkDeviceSize>  offsets,
    DynamicArray<VkDeviceSize>  sizes);

VulkanResult writeToAllocation(
    VulkanGPUAllocator& gpuAllocator,
    const void*         pSrcHostBlock,
    VmaAllocation       dstAllocation,
    VkDeviceSize        dstOffset,
    VkDeviceSize        size);

void setAllocationName(
    VulkanGPUAllocator& gpuAllocator,
    VmaAllocation       dstAllocation,
    const std::string&        name);

} // namespace vulkan
} // namespace dusk