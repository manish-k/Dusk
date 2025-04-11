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
    VkBuffer              buffer;
    VmaAllocation         allocation;
    void*                 mappedMemory;
    size_t                sizeInBytes;
    VkMemoryPropertyFlags memoryFlags;
};

struct VulkanGfxImage
{
    VkImage               image;
    VmaAllocation         allocation;
    void*                 mappedMemory;
    size_t                sizeInBytes;
    VkMemoryPropertyFlags memoryFlags;
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