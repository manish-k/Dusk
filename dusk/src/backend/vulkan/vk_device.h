#pragma once

#include "dusk.h"
#include "vk_base.h"

#include <volk.h>

#include <unordered_map>

namespace dusk
{
class VkGfxDevice
{
public:
    VkGfxDevice();
    ~VkGfxDevice();

    VkResult createInstance(const char* appName, uint32_t version, DynamicArray<const char*> requiredExtensions);
    void destroyInstance();

    void populateLayerNames();
    void populateLayerExtensionNames(const char* pLayerName);

    bool hasLayer(const char* pLayerName) { return m_layersMap.contains(pLayerName); };
    bool hasInstanceExtension(const char* pExtensionName) { return m_instanceExtensionsMap.contains(pExtensionName); };



private:
    VkInstance       m_instance;
    VkPhysicalDevice m_physicalDevice;
    VkDevice         m_device;

    std::unordered_map<const char*, int> m_instanceExtensionsMap;
    std::unordered_map<const char*, int> m_deviceExtensionsMap;
    std::unordered_map<const char*, int> m_layersMap;
};
} // namespace dusk