#pragma once

#include "dusk.h"
#include "vk_base.h"
#include "vk_debug.h"
#include "renderer/gfx_types.h"

#include <volk.h>
#include <string>
#include <iostream>
#include <sstream>
#include <vk_mem_alloc.h>
#include <spdlog/fmt/bundled/printf.h>

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

namespace vulkan
{

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
VkBufferUsageFlags getBufferUsageFlagBits(uint32_t usage);
uint32_t           getVmaAllocationCreateFlagBits(uint32_t flags);

// Texture related helpers
VkImageUsageFlags   getTextureUsageFlagBits(uint32_t usage);
VkImageType         getImageType(TextureType type);
VkImageViewType     getImageViewType(TextureType type);

VkAttachmentLoadOp  getLoadOp(GfxLoadOperation loadOp);
VkAttachmentStoreOp getStoreOp(GfxStoreOperation storeOp);

VkFormat            getPixelVkFormat(PixelFormat format);
PixelFormat         getPixelFormat(VkFormat format);
} // namespace vulkan

} // namespace dusk