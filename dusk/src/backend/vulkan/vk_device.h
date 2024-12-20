#pragma once

#include "dusk.h"
#include "vk.h"
#include "vk_types.h"

#include <volk.h>

#include <unordered_map>
#include <optional>

namespace dusk
{
class VkGfxDevice
{
public:
    VkGfxDevice();
    ~VkGfxDevice();

    Error createInstance(const char* appName, uint32_t version, DynamicArray<const char*> requiredExtensions, VulkanContext& vkContext);
    void  destroyInstance();

    Error createDevice(VulkanContext& vkContext);
    void  destroyDevice();

    Error populateLayerNames();
    Error populateLayerExtensionNames(const char* pLayerName);

    bool  hasLayer(const char* pLayerName);
    bool  hasInstanceExtension(const char* pExtensionName);

#ifdef VK_RENDERER_DEBUG
    static VKAPI_ATTR VkBool32 VKAPI_CALL vulkanDebugMessengerCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT             messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void*                                       pUserData);
#endif

private:
    VkInstance       m_instance       = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkDevice         m_device         = VK_NULL_HANDLE;
    VkCommandPool    m_commandPool    = VK_NULL_HANDLE;

    HashSet<size_t>  m_instanceExtensionsSet;
    HashSet<size_t>  m_layersSet;

    VkQueue          m_graphicsQueue = VK_NULL_HANDLE;
    VkQueue          m_presentQueue  = VK_NULL_HANDLE;
    VkQueue          m_computeQueue  = VK_NULL_HANDLE;
    VkQueue          m_transferQueue = VK_NULL_HANDLE;

#ifdef VK_RENDERER_DEBUG
    VkDebugUtilsMessengerEXT m_debugMessenger;
#endif
};
} // namespace dusk