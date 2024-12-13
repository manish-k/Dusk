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

    VkPhysicalDeviceProperties physicalDeviceProperties;
    VkPhysicalDeviceFeatures   physicalDeviceFeatures;

    uint32_t                   graphicsQueueFamilyIndex;
    uint32_t                   presentQueueFamilyIndex;
    uint32_t                   computeQueueFamilyIndex;
    uint32_t                   transferQueueFamilyIndex;
};
} // namespace dusk