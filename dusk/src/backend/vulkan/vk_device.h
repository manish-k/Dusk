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

    bool hasLayer(const char* pLayerName);
    bool hasInstanceExtension(const char* pExtensionName);



private:
    VkInstance       m_instance;
    VkPhysicalDevice m_physicalDevice;
    VkDevice         m_device;

    HashSet<size_t> m_instanceExtensionsSet;
    HashSet<size_t> m_layersSet;
};
} // namespace dusk