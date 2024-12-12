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

    VkResult result                    = vkCreateInstance(&createInfo, nullptr, &m_instance);
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

    result                                                 = vkCreateDebugUtilsMessengerEXT(
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

void VkGfxDevice::createDevice(VkSurfaceKHR surface)
{
    DASSERT(m_instance != VK_NULL_HANDLE, "VkInstance is not present");
    DASSERT(surface != VK_NULL_HANDLE, "Invalid surface given");

    uint32_t deviceCount = 0u;
    VkResult result      = vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);

    if (deviceCount == 0u)
    {
        DUSK_ERROR("No GPU available with vulkan support");
        return;
    }

    if (result != VK_SUCCESS)
    {
        DUSK_ERROR("Error in getting physical devices");
        return;
    }

    DynamicArray<VkPhysicalDevice> physicalDevices(deviceCount);
    result = vkEnumeratePhysicalDevices(m_instance, &deviceCount, physicalDevices.data());

    if (result != VK_SUCCESS)
    {
        DUSK_ERROR("Error in getting physical devices");
        return;
    }

    struct PhysicalDeviceInfo
    {
        VkPhysicalDeviceProperties deviceProperties;
        VkPhysicalDeviceFeatures   deviceFeatures;
        DynamicArray<const char*>  activeDeviceExtensions;

        uint32_t                   graphicsQueueIndex;
        uint32_t                   computeQueueIndex;
        uint32_t                   transferQueueIndex;
        uint32_t                   presentQueueIndex;

        bool                       isSupported = false;
    };

    DynamicArray<PhysicalDeviceInfo> physicalDeviceInfo(deviceCount);

    for (uint32_t deviceIndex = 0u; deviceIndex < deviceCount; ++deviceIndex)
    {
        const VkPhysicalDevice physicalDevice = physicalDevices[deviceIndex];
        PhysicalDeviceInfo*    pDeviceInfo    = &physicalDeviceInfo[deviceIndex];

        // Get physical device properties
        vkGetPhysicalDeviceProperties(physicalDevice, &pDeviceInfo->deviceProperties);
        DUSK_INFO("Vulkan Device #{} {}", deviceIndex, pDeviceInfo->deviceProperties.deviceName);
        DUSK_INFO("- api version {}", deviceIndex, pDeviceInfo->deviceProperties.apiVersion);
        DUSK_INFO("- vendor id {}", deviceIndex, pDeviceInfo->deviceProperties.vendorID);
        DUSK_INFO("- device id {}", deviceIndex, pDeviceInfo->deviceProperties.deviceID);
        // DUSK_INFO("- device type {}", deviceIndex, pDeviceInfo->deviceProperties.deviceType);
        DUSK_INFO("- driver version {}", deviceIndex, pDeviceInfo->deviceProperties.driverVersion);

        // Get supported features
        vkGetPhysicalDeviceFeatures(physicalDevice, &pDeviceInfo->deviceFeatures);

        // check available queue families
        uint32_t familyCount = 0u;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &familyCount, nullptr);

        DynamicArray<VkQueueFamilyProperties> queueFamiliesProperties(familyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &familyCount, queueFamiliesProperties.data());
        DUSK_INFO("- found {} queue families", familyCount);

        uint32_t commonQueueIndex   = familyCount;
        uint32_t computeQueueIndex  = familyCount;
        uint32_t transferQueueIndex = familyCount;

        for (uint32_t familyIndex = 0; familyIndex < familyCount; ++familyIndex)
        {
            DUSK_INFO("- - Vulkan queue family #{}", familyIndex);
            const bool supportsGraphic  = queueFamiliesProperties[familyIndex].queueFlags & VK_QUEUE_GRAPHICS_BIT;
            const bool supportsCompute  = queueFamiliesProperties[familyIndex].queueFlags & VK_QUEUE_COMPUTE_BIT;
            const bool supportsTransfer = queueFamiliesProperties[familyIndex].queueFlags & VK_QUEUE_TRANSFER_BIT;

            DUSK_INFO("- - - supports graphic {}", supportsGraphic);
            DUSK_INFO("- - - supports compute {}", supportsCompute);
            DUSK_INFO("- - - supports transfer {}", supportsTransfer);

            VkBool32 supportsPresent = false;
            VkResult result          = vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, familyIndex, surface, &supportsPresent);
            if (result != VK_SUCCESS)
            {
                DUSK_ERROR("Unable to query surface support");
                return;
            }
            DUSK_INFO("- - - supports present {}", supportsPresent);

            if (supportsGraphic && supportsCompute && supportsPresent && commonQueueIndex == familyCount)
            {
                commonQueueIndex = familyIndex;
            }

            if (supportsCompute && !supportsGraphic)
            {
                computeQueueIndex = familyIndex;
            }

            if (supportsTransfer && !supportsGraphic)
            {
                transferQueueIndex = familyIndex;
            }

            DUSK_INFO("- - - queue count {}", queueFamiliesProperties[familyIndex].queueCount);
            DUSK_INFO("- - - timestampValidBits {}", queueFamiliesProperties[familyIndex].timestampValidBits);
        }

        if (commonQueueIndex == familyCount)
        {
            DUSK_INFO("Skipping device because no vulkan family found which supports graphics, compute and presentation for given surface");
            continue;
        }
        pDeviceInfo->graphicsQueueIndex = commonQueueIndex;

        if (computeQueueIndex == familyCount)
        {
            // No dedicated compute queue was found, assign common queue
            computeQueueIndex = commonQueueIndex;
        }
        pDeviceInfo->computeQueueIndex = computeQueueIndex;

        if (transferQueueIndex == familyCount)
        {
            // No dedicated transfer queue was found, assign common queue index
            // Graphics queue can always be used for transfer if required
            transferQueueIndex = commonQueueIndex;
        }
        pDeviceInfo->transferQueueIndex = transferQueueIndex;

        // check device extension support
        uint32_t deviceExtensionCount   = 0u;
        vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &deviceExtensionCount, nullptr);

        DynamicArray<VkExtensionProperties> availableExtensions(deviceExtensionCount);
        vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &deviceExtensionCount, availableExtensions.data());
        DUSK_INFO("- found {} supported device extensions", deviceExtensionCount);

        HashSet<size_t> availableExtensionsSet;
        for (auto& extension : availableExtensions)
        {
            DUSK_INFO("- - {}", extension.extensionName);
            availableExtensionsSet.emplace(hash(extension.extensionName));
        }

        if (!availableExtensionsSet.has(hash(VK_KHR_SWAPCHAIN_EXTENSION_NAME)))
        {
            DUSK_INFO("Skipping device because it does not support extension {}", VK_KHR_SWAPCHAIN_EXTENSION_NAME);
            continue;
        }
        pDeviceInfo->activeDeviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

        // Feature checks
        if (!pDeviceInfo->deviceFeatures.samplerAnisotropy)
        {
            DUSK_INFO("Skipping device because it does not support samplerAnisotropy");
            continue;
        }

        pDeviceInfo->isSupported = true;
    }

    std::optional<uint32_t> selectedPhysicalDeviceIndex;

    for (uint32_t i = 0; i < deviceCount; ++i)
    {
        const PhysicalDeviceInfo* pDeviceInfo = &physicalDeviceInfo[i];

        if (!pDeviceInfo->isSupported)
        {
            continue;
        }

        // select very first supported device
        if (!selectedPhysicalDeviceIndex.has_value())
        {
            selectedPhysicalDeviceIndex = i;
        }

        // give preference to device with discrete GPU
        if (pDeviceInfo->isSupported && pDeviceInfo->deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        {
            selectedPhysicalDeviceIndex = i;
            break;
        }
    }

    if (!selectedPhysicalDeviceIndex.has_value())
    {
        DUSK_ERROR("No Vulkan supported physical device was found");
        return;
    }

    m_physicalDevice                                = physicalDevices[selectedPhysicalDeviceIndex.value()];
    const PhysicalDeviceInfo* pSelectedDeviceInfo   = &physicalDeviceInfo[selectedPhysicalDeviceIndex.value()];

    constexpr float           transferQueuePriority = 0.0f;
    constexpr float           graphicsQueuePriority = 1.0f;
    constexpr float           computeQueuePriority  = 1.0f;

    HashSet<uint32_t>         uniqueQueueIndices;
    uniqueQueueIndices.emplace(pSelectedDeviceInfo->graphicsQueueIndex);
    uniqueQueueIndices.emplace(pSelectedDeviceInfo->computeQueueIndex);
    uniqueQueueIndices.emplace(pSelectedDeviceInfo->transferQueueIndex);

    DynamicArray<VkDeviceQueueCreateInfo> queuesCreateInfo;

    // Add graphics queue. Also it is the main queue for presentation
    {
        VkDeviceQueueCreateInfo queueCreateInfo {};
        queueCreateInfo.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = pSelectedDeviceInfo->graphicsQueueIndex;
        queueCreateInfo.queueCount       = 1;
        queueCreateInfo.pQueuePriorities = &graphicsQueuePriority;

        queuesCreateInfo.push_back(queueCreateInfo);
        uniqueQueueIndices.erase(pSelectedDeviceInfo->graphicsQueueIndex);
    }

    // Add dedicated compute queue if available
    if (uniqueQueueIndices.has(pSelectedDeviceInfo->computeQueueIndex))
    {
        VkDeviceQueueCreateInfo queueCreateInfo {};
        queueCreateInfo.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = pSelectedDeviceInfo->computeQueueIndex;
        queueCreateInfo.queueCount       = 1;
        queueCreateInfo.pQueuePriorities = &computeQueuePriority;

        queuesCreateInfo.push_back(queueCreateInfo);
        uniqueQueueIndices.erase(pSelectedDeviceInfo->computeQueueIndex);
    }

    // Add dedicated transfer queue if available
    if (uniqueQueueIndices.has(pSelectedDeviceInfo->transferQueueIndex))
    {
        VkDeviceQueueCreateInfo queueCreateInfo {};
        queueCreateInfo.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = pSelectedDeviceInfo->transferQueueIndex;
        queueCreateInfo.queueCount       = 1;
        queueCreateInfo.pQueuePriorities = &transferQueuePriority;

        queuesCreateInfo.push_back(queueCreateInfo);
        uniqueQueueIndices.erase(pSelectedDeviceInfo->transferQueueIndex);
    }

    DASSERT(queuesCreateInfo.size() != 0, "empty queue create info");

    VkDeviceCreateInfo deviceCreateInfo {};
    deviceCreateInfo.sType                 = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.queueCreateInfoCount  = static_cast<uint32_t>(queuesCreateInfo.size());
    deviceCreateInfo.pQueueCreateInfos     = queuesCreateInfo.data();
    deviceCreateInfo.pEnabledFeatures      = nullptr;

    deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(
        pSelectedDeviceInfo->activeDeviceExtensions.size());
    deviceCreateInfo.ppEnabledExtensionNames = pSelectedDeviceInfo->activeDeviceExtensions.data();

    result                                   = vkCreateDevice(m_physicalDevice, &deviceCreateInfo, nullptr, &m_device);

    if (result != VK_SUCCESS)
    {
        DUSK_ERROR("Failed to create logical device");
        return;
    }
    DUSK_INFO("Logical device creation successful");

    volkLoadDevice(m_device);

    // fetch graphic queue handle
    vkGetDeviceQueue(m_device, pSelectedDeviceInfo->graphicsQueueIndex, 0, &m_graphicsQueue);
    if (m_graphicsQueue == VK_NULL_HANDLE)
    {
        DUSK_ERROR("Unable to get graphic queue from vulkan device");
        return;
    }

    // fetch compute queue handle
    vkGetDeviceQueue(m_device, pSelectedDeviceInfo->computeQueueIndex, 0, &m_computeQueue);
    if (m_computeQueue == VK_NULL_HANDLE)
    {
        DUSK_ERROR("Unable to get compute queue from vulkan device");
        return;
    }

    // fetch graphic queue handle
    vkGetDeviceQueue(m_device, pSelectedDeviceInfo->transferQueueIndex, 0, &m_transferQueue);
    if (m_transferQueue == VK_NULL_HANDLE)
    {
        DUSK_ERROR("Unable to get transfer queue from vulkan device");
        return;
    }
}

void VkGfxDevice::destroyInstance()
{
#ifdef VK_RENDERER_DEBUG
    vkDestroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);
#endif

    vkDestroyInstance(m_instance, nullptr);
}

void VkGfxDevice::destroyDevice()
{
    m_graphicsQueue = VK_NULL_HANDLE;
    m_computeQueue = VK_NULL_HANDLE;
    m_transferQueue = VK_NULL_HANDLE;

    vkDestroyDevice(m_device, nullptr);
    m_device = VK_NULL_HANDLE;
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
        DUSK_INFO(" - {}", extension.extensionName);
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
        DUSK_INFO(" - {}", layer.layerName);
        m_layersSet.emplace(hash(layer.layerName));
    }
}

bool VkGfxDevice::hasLayer(const char* pLayerName)
{
    return m_layersSet.has(hash(pLayerName));
}

} // namespace dusk