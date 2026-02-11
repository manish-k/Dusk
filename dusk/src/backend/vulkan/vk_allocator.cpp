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

VulkanResult vulkan::allocateGPUBuffer(
    VulkanGPUAllocator&       gpuAllocator,
    const VkBufferCreateInfo& bufferCreateInfo,
    VmaMemoryUsage            usage,
    VmaAllocationCreateFlags  flags,
    VulkanGfxBuffer*          pBufferResult)
{
    DASSERT(pBufferResult != nullptr, "out bufferResult should not be null");

    VmaAllocationCreateInfo allocationCreateInfo = {};
    allocationCreateInfo.usage                   = usage;
    allocationCreateInfo.flags                   = flags;

    VmaAllocationInfo allocationInfo {};
    VulkanResult      result = vmaCreateBuffer(
        gpuAllocator.vmaAllocator,
        &bufferCreateInfo,
        &allocationCreateInfo,
        &pBufferResult->buffer,
        &pBufferResult->allocation,
        &allocationInfo);

    if (result.hasError())
    {
        return result;
    }

    vmaGetMemoryTypeProperties(gpuAllocator.vmaAllocator, allocationInfo.memoryType, &pBufferResult->memoryFlags);

    pBufferResult->mappedMemory = nullptr;
    if (allocationCreateInfo.flags & VMA_ALLOCATION_CREATE_MAPPED_BIT)
    {
        // TODO check allignment of the mapped memory pointer
        pBufferResult->mappedMemory = allocationInfo.pMappedData;
    }
    pBufferResult->sizeInBytes = allocationInfo.size;
    gpuAllocator.allocatedBufferBytes += allocationInfo.size;

    return result;
}

void vulkan::freeGPUBuffer(
    VulkanGPUAllocator& gpuAllocator,
    VulkanGfxBuffer*    pGfxBuffer)
{
    DASSERT(pGfxBuffer != nullptr, "attempt to free a nullptr");
    if (pGfxBuffer->buffer != VK_NULL_HANDLE)
    {
        vmaDestroyBuffer(gpuAllocator.vmaAllocator, pGfxBuffer->buffer, pGfxBuffer->allocation);
        gpuAllocator.allocatedBufferBytes -= pGfxBuffer->sizeInBytes;
    }
}

VulkanResult vulkan::mapGPUMemory(
    VulkanGPUAllocator& gpuAllocator,
    VmaAllocation       allocation,
    void**              ppMappedBlock)
{
    void*        mappedData;
    VulkanResult result = vmaMapMemory(gpuAllocator.vmaAllocator, allocation, &mappedData);
    if (result.hasError())
    {
        *ppMappedBlock = nullptr;
    }

    *ppMappedBlock = mappedData;
    return result;
}

void vulkan::unmapGPUMemory(VulkanGPUAllocator& gpuAllocator, VmaAllocation allocation)
{
    vmaUnmapMemory(gpuAllocator.vmaAllocator, allocation);
}

VulkanResult vulkan::allocateGPUImage(
    VulkanGPUAllocator&      gpuAllocator,
    const VkImageCreateInfo& imageCreateInfo,
    VmaMemoryUsage           usage,
    VmaAllocationCreateFlags flags,
    VulkanGfxImage*          pImageResult)
{
    DASSERT(pImageResult != nullptr, "out imageResult should not be null");

    VmaAllocationCreateInfo allocCreateInfo {};
    allocCreateInfo.usage = usage;
    allocCreateInfo.flags = flags;

    VmaAllocationInfo allocationInfo {};
    VulkanResult      result = vmaCreateImage(
        gpuAllocator.vmaAllocator,
        &imageCreateInfo,
        &allocCreateInfo,
        &pImageResult->vkImage,
        &pImageResult->allocation,
        &allocationInfo);

    pImageResult->sizeInBytes = allocationInfo.size;
    gpuAllocator.allocatedImageBytes += allocationInfo.size;

    return result;
}

void vulkan::freeGPUImage(VulkanGPUAllocator& gpuAllocator, VulkanGfxImage* pGfxImage)
{
    DASSERT(pGfxImage != nullptr, "attempt to free a nullptr image");
    if (pGfxImage->vkImage)
    {
        vmaDestroyImage(gpuAllocator.vmaAllocator, pGfxImage->vkImage, pGfxImage->allocation);
        gpuAllocator.allocatedImageBytes -= pGfxImage->sizeInBytes;
    }
}

VulkanResult vulkan::flushCPUMemory(
    VulkanGPUAllocator&         gpuAllocator,
    DynamicArray<VmaAllocation> allocations,
    DynamicArray<VkDeviceSize>  offsets,
    DynamicArray<VkDeviceSize>  sizes)
{
    return vmaFlushAllocations(
        gpuAllocator.vmaAllocator,
        allocations.size(),
        allocations.data(),
        offsets.data(),
        sizes.data());
}

VulkanResult vulkan::invalidateCPUMemory(
    VulkanGPUAllocator&         gpuAllocator,
    DynamicArray<VmaAllocation> allocations,
    DynamicArray<VkDeviceSize>  offsets,
    DynamicArray<VkDeviceSize>  sizes)
{
    return vmaInvalidateAllocations(
        gpuAllocator.vmaAllocator,
        allocations.size(),
        allocations.data(),
        offsets.data(),
        sizes.data());
}

VulkanResult vulkan::writeToAllocation(
    VulkanGPUAllocator& gpuAllocator,
    const void*         pSrcHostBlock,
    VmaAllocation       dstAllocation,
    VkDeviceSize        dstOffset,
    VkDeviceSize        size)
{
    return vmaCopyMemoryToAllocation(
        gpuAllocator.vmaAllocator,
        pSrcHostBlock,
        dstAllocation,
        dstOffset,
        size);
}

void vulkan::setAllocationName(VulkanGPUAllocator& gpuAllocator, VmaAllocation dstAllocation, const std::string& name)
{
    vmaSetAllocationName(
        gpuAllocator.vmaAllocator,
        dstAllocation,
        name.c_str());
}

} // namespace dusk