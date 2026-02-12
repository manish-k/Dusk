#pragma once

#include <volk.h>
#include <vk_mem_alloc.h>

namespace dusk
{
// TODO:: check if synchronization is required for vmaAllocation access
struct VulkanGPUAllocator
{
    VmaAllocator vmaAllocator;
    size_t       allocatedBufferBytes = 0u;
    size_t       allocatedImageBytes  = 0u;
};

struct VulkanGfxBuffer
{
    VkBuffer              buffer        = VK_NULL_HANDLE;
    VmaAllocation         allocation    = VK_NULL_HANDLE;
    void*                 mappedMemory  = nullptr;
    size_t                sizeInBytes   = 0u;
    VkMemoryPropertyFlags memoryFlags   = 0;
    size_t                alignmentSize = 0u;
};

struct VulkanGfxImage
{
    VkImage               vkImage      = VK_NULL_HANDLE;
    VmaAllocation         allocation   = VK_NULL_HANDLE;
    void*                 mappedMemory = nullptr;
    size_t                sizeInBytes  = 0u;
    VkMemoryPropertyFlags memoryFlags  = 0;
};

struct VulkanContext
{
    VkInstance                 vulkanInstance;
    VkPhysicalDevice           physicalDevice;
    VkDevice                   device;
    VkSurfaceKHR               surface;

    VkCommandPool              commandPool;
    VkCommandPool              transferCommandPool;

    VkPhysicalDeviceProperties physicalDeviceProperties;
    VkPhysicalDeviceFeatures   physicalDeviceFeatures;

    uint32_t                   graphicsQueueFamilyIndex;
    uint32_t                   presentQueueFamilyIndex;
    uint32_t                   computeQueueFamilyIndex;
    uint32_t                   transferQueueFamilyIndex;

    VkQueue                    graphicsQueue;
    VkQueue                    presentQueue;
    VkQueue                    computeQueue;
    VkQueue                    transferQueue;

    VulkanGPUAllocator*        gpuAllocator;
};

struct VulkanSampler
{
    VkSampler sampler;
};

struct VulkanImageBarier
{
    VkImage              image;
    VkImageUsageFlags    usage;
    VkImageLayout        oldLayout;
    VkImageLayout        newLayout;
    VkPipelineStageFlags srcStage;
    VkAccessFlags        srcAccess;
    VkPipelineStageFlags dstStage;
    VkAccessFlags        dstAccess;
};

} // namespace dusk