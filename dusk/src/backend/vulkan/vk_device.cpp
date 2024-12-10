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
    VkDebugUtilsMessengerCreateInfoEXT messengerCreateInfo = { VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
    messengerCreateInfo.messageSeverity                    = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
    messengerCreateInfo.messageType                        = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
    messengerCreateInfo.pfnUserCallback                    = vulkanDebugMessengerCallback;
    messengerCreateInfo.pUserData                          = this;

    result = vkCreateDebugUtilsMessengerEXT(
        m_instance,
        &messengerCreateInfo,
        nullptr,
        &m_debugMessenger);

    if (result != VK_SUCCESS)
    {
        DUSK_ERROR("Unable to setup debug messenger");
    }
#endif

    return VK_SUCCESS;
}

void VkGfxDevice::destroyInstance()
{
#ifdef VK_RENDERER_DEBUG
    vkDestroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);
#endif

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

#ifdef VK_RENDERER_DEBUG
VKAPI_ATTR VkBool32 VKAPI_CALL VkGfxDevice::vulkanDebugMessengerCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT             messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void*                                       pUserData)
{
    if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
    {
        DUSK_INFO("Vulkan: {}", pCallbackData->pMessage);
        if (pCallbackData->cmdBufLabelCount)
        {
            DUSK_INFO(" debug labels:");
            for (size_t i = 0u; i < pCallbackData->cmdBufLabelCount; ++i)
            {
                const size_t reverseIndex = pCallbackData->cmdBufLabelCount - 1u - i;
                DUSK_INFO("     - {}", pCallbackData->pCmdBufLabels[reverseIndex].pLabelName);
            }
        }
    }

    if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    {
        DUSK_WARN("Vulkan: {}", pCallbackData->pMessage);
    }

    if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
    {
        DUSK_ERROR("Vulkan: {}", pCallbackData->pMessage);
    }

    return VK_FALSE;
}
#endif

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