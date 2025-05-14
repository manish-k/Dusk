#pragma once

#include "vk.h"

namespace dusk
{
// TODO:: check if synchronization is required for vmaAllocation access
struct VulkanGPUAllocator
{
    VmaAllocator vmaAllocator;
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
    VkImage               image        = VK_NULL_HANDLE;
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

    VulkanGPUAllocator         gpuAllocator;
};

struct VulkanTexture
{
    VulkanGfxImage image;
    VkImageView    imageView;
};

struct VulkanSampler
{
    VkSampler sampler;
};

} // namespace dusk