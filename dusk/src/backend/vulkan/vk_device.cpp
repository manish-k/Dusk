#pragma once

#include "vk_device.h"

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

namespace dusk
{
VkGfxDevice::VkGfxDevice()
{
}

VkGfxDevice::~VkGfxDevice()
{
}

VkResult VkGfxDevice::createInstance(const char* appName, uint32_t version, DynamicArray<const char*> requiredExtensionNames)
{
    VkApplicationInfo appInfo {};
    appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName   = appName;
    appInfo.applicationVersion = VK_MAKE_VERSION(1, version, 0);
    appInfo.pEngineName        = "Dusk";
    appInfo.engineVersion      = VK_MAKE_VERSION(1, version, 0);
    appInfo.apiVersion         = VK_API_VERSION_1_2;

    VkInstanceCreateInfo createInfo {};
    createInfo.sType            = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    DynamicArray<const char*> validationLayerNames;
#ifdef VK_ENABLE_VALIDATION_LAYERS
    populateLayerNames();

    const char* validationLayerName = "VK_LAYER_KHRONOS_validation";
    if (hasLayer(validationLayerName))
    {
        populateLayerExtensionNames(validationLayerName);

        if (hasInstanceExtension("VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME"))
        {
            requiredExtensionNames.push_back("VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME");
        }

        validationLayerNames.push_back(validationLayerName);
    }
    else
    {
        DUSK_WARN("Validation layer not found");
    }
#endif

#ifdef VK_RENDERER_DEBUG
    requiredExtensionNames.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

    createInfo.enabledExtensionCount   = static_cast<uint32_t>(requiredExtensionNames.size());
    createInfo.ppEnabledExtensionNames = requiredExtensionNames.data();
    createInfo.enabledLayerCount       = static_cast<uint32_t>(validationLayerNames.size());
    createInfo.ppEnabledLayerNames     = validationLayerNames.data();

    VkResult result = vkCreateInstance(&createInfo, nullptr, &m_instance);
    if (result != VK_SUCCESS)
    {
        return result;
    }

    volkLoadInstance(m_instance);

#ifdef VK_RENDERER_DEBUG
    // debug messenger
#endif

    return VK_SUCCESS;
}

void VkGfxDevice::destroyInstance()
{
    vkDestroyInstance(m_instance, nullptr);
}

void VkGfxDevice::populateLayerExtensionNames(const char* pLayerName)
{
    uint32_t extensionCount = 0;
    VkResult result         = vkEnumerateInstanceExtensionProperties(pLayerName, &extensionCount, nullptr);
    if (result != VK_SUCCESS)
    {
        DUSK_ERROR("Vulkan instance extension enumeration failed");
        return;
    }

    DynamicArray<VkExtensionProperties> instanceExtensions(extensionCount);
    result = vkEnumerateInstanceExtensionProperties(pLayerName, &extensionCount, instanceExtensions.data());
    if (result != VK_SUCCESS)
    {
        DUSK_ERROR("Vulkan instance extension enumeration failed");
        return;
    }

    DUSK_INFO("Supported Vulkan Instance Extensions for layer {}", pLayerName);
    for (const auto& extension : instanceExtensions)
    {
        DUSK_INFO("{}", extension.extensionName);
        m_instanceExtensionsSet.emplace(hash(extension.extensionName));
    }
}

bool VkGfxDevice::hasInstanceExtension(const char* pExtensionName)
{
    return m_instanceExtensionsSet.has(hash(pExtensionName));
}

void VkGfxDevice::populateLayerNames()
{
    uint32_t layerCount = 0;
    VkResult result     = vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    if (result != VK_SUCCESS)
    {
        DUSK_ERROR("Vulkan instance layers enumeration failed");
        return;
    }

    DynamicArray<VkLayerProperties> instanceLayers(layerCount);
    result = vkEnumerateInstanceLayerProperties(&layerCount, instanceLayers.data());
    if (result != VK_SUCCESS)
    {
        DUSK_ERROR("Vulkan instance layers enumeration failed");
        return;
    }

    DUSK_INFO("Supported Vulkan layers");
    for (const auto& layer : instanceLayers)
    {
        DUSK_INFO("{}", layer.layerName);
        m_layersSet.emplace(hash(layer.layerName));
    }
}

bool VkGfxDevice::hasLayer(const char* pLayerName)
{
    return m_layersSet.has(hash(pLayerName));
}

} // namespace dusk