#pragma once

#include "dusk.h"
#include "vk_base.h"

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

    VkResult   createInstance(const char* appName, uint32_t version, DynamicArray<const char*> requiredExtensions);
    void       destroyInstance();
    VkInstance getVkInstance() { return m_instance; } // TODO: Handle without getter, maybe with some struct

    void       createDevice(VkSurfaceKHR surface);
    void       destroyDevice();

    void       populateLayerNames();
    void       populateLayerExtensionNames(const char* pLayerName);

    bool       hasLayer(const char* pLayerName);
    bool       hasInstanceExtension(const char* pExtensionName);

#ifdef VK_RENDERER_DEBUG
    static VKAPI_ATTR VkBool32 VKAPI_CALL vulkanDebugMessengerCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT             messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void*                                       pUserData);
#endif

private:
    VkInstance       m_instance;
    VkPhysicalDevice m_physicalDevice;
    VkDevice         m_device;

    HashSet<size_t>  m_instanceExtensionsSet;
    HashSet<size_t>  m_layersSet;

    uint32_t         m_graphicsQueueIndex;
    uint32_t         m_computeQueueIndex;
    uint32_t         m_transferQueueIndex;

#ifdef VK_RENDERER_DEBUG
    VkDebugUtilsMessengerEXT m_debugMessenger;
#endif
};
} // namespace dusk