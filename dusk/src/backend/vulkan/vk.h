#pragma once

#include "dusk.h"
#include "vk_base.h"
#include "vk_debug.h"
#include "platform/platform.h"
#include "core/error.h"
#include "renderer/vertex.h"

#include <volk.h>
#include <string>
#include <iostream>
#include <sstream>
#include <vk_mem_alloc.h>

namespace dusk
{
/**
 * @brief Encapsulation of VkResult with useful functions
 */
struct VulkanResult
{
    VkResult vkResult = VK_SUCCESS;

    VulkanResult()    = default;
    VulkanResult(VkResult result) :
        vkResult(result) { }

    bool                 isOk() const { return vkResult == VK_SUCCESS; }
    bool                 hasError() const { return vkResult != VK_SUCCESS; }

    Error                getErrorId() const;
    std::string          toString() const;

    friend std::ostream& operator<<(std::ostream& os, const VulkanResult& result) { return os << result.toString(); }
};

// Vk values to string
const char* getDeviceTypeString(VkPhysicalDeviceType deviceType);
const char* getVkFormatString(VkFormat format);
const char* getVkColorSpaceString(VkColorSpaceKHR colorSpace);
const char* getVkPresentModeString(VkPresentModeKHR presentMode);

// Custom values to Vk values
VkFormat getVkVertexAttributeFormat(VertexAttributeFormat format);

// vertex descriptor functions
DynamicArray<VkVertexInputBindingDescription>   getVertexBindingDescription();
DynamicArray<VkVertexInputAttributeDescription> getVertexAtrributeDescription();

// Buffer usage and type related funcs
VkBufferUsageFlags          getBufferUsageFlagBits(uint32_t usage);
VmaAllocationCreateFlagBits getVmaAllocationCreateFlagBits(uint32_t flags);

} // namespace dusk