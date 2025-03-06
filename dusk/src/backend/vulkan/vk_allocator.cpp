#include "vk_allocator.h"

#define VMA_IMPLEMENTATION
#define VMA_STATIC_VULKAN_FUNCTIONS  0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0

namespace dusk
{
VulkanResult vulkan::createGPUAllocator(const VulkanContext& context, VulkanGPUAllocator& gpuAllocator)
{
    VmaVulkanFunctions vulkanFunctions                      = {};
    vulkanFunctions.vkGetInstanceProcAddr                   = vkGetInstanceProcAddr;
    vulkanFunctions.vkGetDeviceProcAddr                     = vkGetDeviceProcAddr;
    vulkanFunctions.vkGetPhysicalDeviceProperties           = vkGetPhysicalDeviceProperties;
    vulkanFunctions.vkGetPhysicalDeviceMemoryProperties     = vkGetPhysicalDeviceMemoryProperties;
    vulkanFunctions.vkAllocateMemory                        = vkAllocateMemory;
    vulkanFunctions.vkFreeMemory                            = vkFreeMemory;
    vulkanFunctions.vkMapMemory                             = vkMapMemory;
    vulkanFunctions.vkUnmapMemory                           = vkUnmapMemory;
    vulkanFunctions.vkFlushMappedMemoryRanges               = vkFlushMappedMemoryRanges;
    vulkanFunctions.vkInvalidateMappedMemoryRanges          = vkInvalidateMappedMemoryRanges;
    vulkanFunctions.vkBindBufferMemory                      = vkBindBufferMemory;
    vulkanFunctions.vkBindImageMemory                       = vkBindImageMemory;
    vulkanFunctions.vkGetBufferMemoryRequirements           = vkGetBufferMemoryRequirements;
    vulkanFunctions.vkGetImageMemoryRequirements            = vkGetImageMemoryRequirements;
    vulkanFunctions.vkCreateBuffer                          = vkCreateBuffer;
    vulkanFunctions.vkCreateImage                           = vkCreateImage;
    vulkanFunctions.vkDestroyBuffer                         = vkDestroyBuffer;
    vulkanFunctions.vkDestroyImage                          = vkDestroyImage;
    vulkanFunctions.vkCmdCopyBuffer                         = vkCmdCopyBuffer;
    vulkanFunctions.vkGetBufferMemoryRequirements2KHR       = vkGetBufferMemoryRequirements2KHR;
    vulkanFunctions.vkGetImageMemoryRequirements2KHR        = vkGetImageMemoryRequirements2KHR;
    vulkanFunctions.vkBindBufferMemory2KHR                  = vkBindBufferMemory2KHR;
    vulkanFunctions.vkBindImageMemory2KHR                   = vkBindImageMemory2KHR;
    vulkanFunctions.vkGetPhysicalDeviceMemoryProperties2KHR = vkGetPhysicalDeviceMemoryProperties2KHR;
    vulkanFunctions.vkGetDeviceBufferMemoryRequirements     = vkGetDeviceBufferMemoryRequirements;
    vulkanFunctions.vkGetDeviceImageMemoryRequirements      = vkGetDeviceImageMemoryRequirements;
    vulkanFunctions.vkGetMemoryWin32HandleKHR               = vkGetMemoryWin32HandleKHR;

    // TODO: explore vma pool for effecient allocations

    VmaAllocatorCreateInfo allocatorCreateInfo
        = {};
    allocatorCreateInfo.flags            = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
    allocatorCreateInfo.vulkanApiVersion = VK_API_VERSION_1_3;
    allocatorCreateInfo.physicalDevice   = context.physicalDevice;
    allocatorCreateInfo.device           = context.device;
    allocatorCreateInfo.instance         = context.vulkanInstance;
    allocatorCreateInfo.pVulkanFunctions = &vulkanFunctions;

    VulkanResult result                  = vmaCreateAllocator(&allocatorCreateInfo, &gpuAllocator.vmaAllocator);

    if (result.hasError())
    {
        destroyGPUAllocator(gpuAllocator);
    }
    return result;
}

void vulkan::destroyGPUAllocator(VulkanGPUAllocator& gpuAllocator)
{
    if (gpuAllocator.vmaAllocator != nullptr)
    {
        vmaDestroyAllocator(gpuAllocator.vmaAllocator);
        gpuAllocator.vmaAllocator = nullptr;
    }
}

VulkanResult vulkan::allocateGPUBuffer(VulkanGPUAllocator& gpuAllocator, const VkBufferCreateInfo& bufferCreateInfo, VmaMemoryUsage usage, VmaAllocationCreateFlags flags, VulkanGfxBuffer* bufferResult)
{
    DASSERT(bufferResult != nullptr, "out bufferResult should not be null");

    VmaAllocationCreateInfo allocationCreateInfo = {};
    allocationCreateInfo.usage                   = usage;
    allocationCreateInfo.flags                   = flags;

    VmaAllocationInfo allocationInfo {};
    VulkanResult      result = vmaCreateBuffer(gpuAllocator.vmaAllocator, &bufferCreateInfo, &allocationCreateInfo, &bufferResult->buffer, &bufferResult->allocation, &allocationInfo);

    if (result.hasError())
    {
        return result;
    }

    vmaGetMemoryTypeProperties(gpuAllocator.vmaAllocator, allocationInfo.memoryType, &bufferResult->memoryFlags);

    bufferResult->mappedMemory = nullptr;
    if (allocationCreateInfo.flags & VMA_ALLOCATION_CREATE_MAPPED_BIT)
    {
        // TODO check allignment of the mapped memory pointer
        bufferResult->mappedMemory = allocationInfo.pMappedData;
    }
    bufferResult->sizeInBytes = allocationInfo.size;

    return result;
}

void vulkan::freeGPUBuffer(VulkanGPUAllocator& gpuAllocator, VulkanGfxBuffer* gfxBuffer)
{
    DASSERT(gfxBuffer != nullptr, "attempt to free a nullptr");
    if (gfxBuffer->buffer != VK_NULL_HANDLE)
    {
        vmaDestroyBuffer(gpuAllocator.vmaAllocator, gfxBuffer->buffer, gfxBuffer->allocation);
    }
}

VulkanResult vulkan::mapGPUMemory(VulkanGPUAllocator& gpuAllocator, VmaAllocation allocation, void* pMappedBlock)
{
    void*        mappedBlock;
    VulkanResult result = vmaMapMemory(gpuAllocator.vmaAllocator, allocation, &pMappedBlock);
    if (result.hasError())
    {
        pMappedBlock = nullptr;
    }

    return result;
}

void vulkan::unmapGPUMemory(VulkanGPUAllocator& gpuAllocator, VmaAllocation allocation)
{
    vmaUnmapMemory(gpuAllocator.vmaAllocator, allocation);
}

VulkanResult vulkan::flushCPUMemory(VulkanGPUAllocator& gpuAllocator, DynamicArray<VmaAllocation> allocations, DynamicArray<VkDeviceSize> offsets, DynamicArray<VkDeviceSize> sizes)
{
    return vmaFlushAllocations(gpuAllocator.vmaAllocator, allocations.size(), allocations.data(), offsets.data(), sizes.data());
}

VulkanResult vulkan::invalidateCPUMemory(VulkanGPUAllocator& gpuAllocator, DynamicArray<VmaAllocation> allocations, DynamicArray<VkDeviceSize> offsets, DynamicArray<VkDeviceSize> sizes)
{
    return vmaInvalidateAllocations(gpuAllocator.vmaAllocator, allocations.size(), allocations.data(), offsets.data(), sizes.data());
}

VulkanResult vulkan::writeToAllocation(VulkanGPUAllocator& gpuAllocator, const void* pSrcHostBlock, VmaAllocation dstAllocation, VkDeviceSize dstOffset, VkDeviceSize size)
{
    return vmaCopyMemoryToAllocation(gpuAllocator.vmaAllocator, pSrcHostBlock, dstAllocation, dstOffset, size);
}

} // namespace dusk