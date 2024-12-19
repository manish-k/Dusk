#pragma once

#include "vk.h"

namespace dusk
{
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
};
} // namespace dusk