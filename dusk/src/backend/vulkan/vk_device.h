#pragma once

#include "dusk.h"
#include "vk.h"
#include "vk_types.h"
#include "vk_allocator.h"
#include "renderer/gfx_buffer.h"
#include "platform/glfw_vulkan_window.h"

#include <volk.h>

#include <unordered_map>
#include <optional>

#ifdef DUSK_ENABLE_PROFILING
#    include <tracy/Tracy.hpp>
#    include <tracy/TracyVulkan.hpp>
#endif

namespace dusk
{
class VkGfxDevice
{
public:
    VkGfxDevice(GLFWVulkanWindow& window);
    ~VkGfxDevice();

    Error           initGfxDevice();
    void            cleanupGfxDevice();

    Error           createInstance(const char* appName, uint32_t version, DynamicArray<const char*> requiredExtensions);
    void            destroyInstance();

    Error           createDevice();
    void            destroyDevice();

    Error           populateLayerNames();
    Error           populateLayerExtensionNames(const char* pLayerName);

    bool            hasLayer(const char* pLayerName);
    bool            hasInstanceExtension(const char* pExtensionName);

    VkCommandBuffer beginSingleTimeCommands() const;
    void            endSingleTimeCommands(VkCommandBuffer commandBuffer) const;

    VkCommandBuffer beginSingleTimeTransferCommands() const;
    void            endSingleTimeTransferCommands(VkCommandBuffer commandBuffer) const;

    VulkanResult    createBuffer(const GfxBufferParams& params, VulkanGfxBuffer* pOutBuffer);
    void            freeBuffer(VulkanGfxBuffer* buffer);
    void            copyBuffer(
                   const VulkanGfxBuffer& srcBuffer,
                   VkDeviceSize           srcOffset,
                   VulkanGfxBuffer&       dstBuffer,
                   VkDeviceSize           dstOffset,
                   VkDeviceSize           size);
    void mapBuffer(VulkanGfxBuffer* buffer);
    void unmapBuffer(VulkanGfxBuffer* buffer);
    void flushBuffer(VulkanGfxBuffer* buffer);
    void flushBufferOffset(
        VulkanGfxBuffer* buffer,
        uint32_t,
        size_t size);
    void writeToBuffer(
        VulkanGfxBuffer* buffer,
        void*            hostBlock,
        VkDeviceSize     offset,
        VkDeviceSize     size);

    //
    void copyBufferToImage(
        VulkanGfxBuffer* buffer,
        VulkanGfxImage*  img,
        uint32_t         width,
        uint32_t         height);

    void copyBufferToImageRegions(
        VulkanGfxBuffer*                 buffer,
        VulkanGfxImage*                  img,
        DynamicArray<VkBufferImageCopy>& regions);

    Error transitionImageWithLayout(
        VulkanGfxImage* img,
        VkFormat        format,
        VkImageLayout   oldLayout,
        VkImageLayout   newLayout,
        uint32_t        mipLevelCount,
        uint32_t        layersCount);

    VulkanResult createImageView(
        VulkanGfxImage*    img,
        VkImageViewType    type,
        VkFormat           format,
        VkImageAspectFlags aspectMaskFlags,
        uint32_t           baseMipLevel,
        uint32_t           mipLevelCount,
        uint32_t           baseLayer,
        uint32_t           layersCount,
        VkImageView*       pImageView) const;
    void                  freeImageView(VkImageView* pImageView) const;
    VulkanResult          createImageSampler(VulkanSampler* sampler) const;
    void                  freeImageSampler(VulkanSampler* sampler) const;

    static VulkanContext& getSharedVulkanContext() { return s_sharedVkContext; }

#ifdef VK_RENDERER_DEBUG
    static VKAPI_ATTR VkBool32 VKAPI_CALL vulkanDebugMessengerCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT             messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void*                                       pUserData);
#endif

#ifdef DUSK_ENABLE_PROFILING
    static tracy::VkCtx* getProfilerContext() { return s_gpuProfilerCtx; };
#endif

private:
    GLFWVulkanWindow&  m_window;

    VkInstance         m_instance            = VK_NULL_HANDLE;
    VkPhysicalDevice   m_physicalDevice      = VK_NULL_HANDLE;
    VkDevice           m_device              = VK_NULL_HANDLE;
    VkCommandPool      m_commandPool         = VK_NULL_HANDLE;
    VkCommandPool      m_transferCommandPool = VK_NULL_HANDLE;
    VkSurfaceKHR       m_surface             = VK_NULL_HANDLE;

    VulkanGPUAllocator m_gpuAllocator {};

    HashSet<size_t>    m_instanceExtensionsSet {};
    HashSet<size_t>    m_layersSet {};

    VkQueue            m_graphicsQueue = VK_NULL_HANDLE;
    VkQueue            m_presentQueue  = VK_NULL_HANDLE;
    VkQueue            m_computeQueue  = VK_NULL_HANDLE;
    VkQueue            m_transferQueue = VK_NULL_HANDLE;

#ifdef VK_RENDERER_DEBUG
    VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;
#endif

private:
    static VulkanContext s_sharedVkContext;

#ifdef DUSK_ENABLE_PROFILING
    static tracy::VkCtx* s_gpuProfilerCtx;
#endif
};
} // namespace dusk