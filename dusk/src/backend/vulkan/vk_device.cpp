#pragma once

#include "vk_device.h"

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

namespace dusk
{
VulkanContext VkGfxDevice::s_sharedVkContext = VulkanContext {};

#ifdef DUSK_ENABLE_PROFILING
tracy::VkCtx* VkGfxDevice::s_gpuProfilerCtx = VK_NULL_HANDLE;
#endif

VkGfxDevice::VkGfxDevice(GLFWVulkanWindow& window) :
    m_window(window)
{
}

VkGfxDevice::~VkGfxDevice()
{
}

Error VkGfxDevice::initGfxDevice()
{
    // Initialize instance function pointers
    VulkanResult result = volkInitialize();
    if (result.hasError())
    {
        DUSK_ERROR("Volk initialization failed. Vulkan loader might not be present {}", result.toString());
        return Error::InitializationFailed;
    }

    DynamicArray<const char*> extensions = m_window.getRequiredWindowExtensions();
    DUSK_INFO("Required {} vulkan extensions for GLFW", extensions.size());
    for (int i = 0; i < extensions.size(); ++i)
    {
        DUSK_INFO(" - {}", extensions[i]);
    }

    Error err = createInstance("DUSK", 1, extensions);

    if (err != Error::Ok)
    {
        return err;
    }

    err = m_window.createWindowSurface(m_instance, &m_surface);
    if (err != Error::Ok)
    {
        return err;
    }

    // pick physical device and create logical device
    err = createDevice();
    if (err != Error::Ok)
    {
        return err;
    }

    // create GPU allocator using vma
    result = vulkan::createGPUAllocator(s_sharedVkContext, m_gpuAllocator);
    if (result.hasError())
    {
        DUSK_ERROR("Error in creating vma gpu allocator {}", result.toString());
        return Error::InitializationFailed;
    }
    VkGfxDevice::s_sharedVkContext.gpuAllocator = m_gpuAllocator;

    return Error::Ok;
}

void VkGfxDevice::cleanupGfxDevice()
{
    vkDestroySurfaceKHR(m_instance, m_surface, nullptr);

    vulkan::destroyGPUAllocator(m_gpuAllocator);

    destroyDevice();
    destroyInstance();

    volkFinalize();
}

Error VkGfxDevice::createInstance(const char* appName, uint32_t version, DynamicArray<const char*> requiredExtensionNames)
{
    VkApplicationInfo appInfo {};
    appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName   = appName;
    appInfo.applicationVersion = VK_MAKE_VERSION(1, version, 0);
    appInfo.pEngineName        = "Dusk";
    appInfo.engineVersion      = VK_MAKE_VERSION(1, version, 0);
    appInfo.apiVersion         = VK_API_VERSION_1_3;

    VkInstanceCreateInfo createInfo {};
    createInfo.sType            = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    DynamicArray<const char*> validationLayerNames;
#ifdef VK_ENABLE_VALIDATION_LAYERS
    Error err = populateLayerNames();
    if (err != Error::Ok)
        return err;

    const char* validationLayerName = "VK_LAYER_KHRONOS_validation";
    if (hasLayer(validationLayerName))
    {
        err = populateLayerExtensionNames(validationLayerName);
        if (err != Error::Ok)
            return err;

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

    VulkanResult result                = vkCreateInstance(&createInfo, nullptr, &m_instance);
    if (result.hasError())
    {
        DUSK_ERROR("vkInstance creation failed {}", result.toString());
        return result.getErrorId();
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

    if (result.hasError())
    {
        DUSK_ERROR("Unable to setup debug messenger {}", result.toString());
        return result.getErrorId();
    }
#endif

    DUSK_INFO("VkInstance created successfully");
    return Error::Ok;
}

Error VkGfxDevice::createDevice()
{
    DASSERT(m_instance != VK_NULL_HANDLE, "VkInstance is not present");
    DASSERT(m_surface != VK_NULL_HANDLE, "Invalid surface given");

    uint32_t     deviceCount = 0u;
    VulkanResult result      = vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);

    if (result.hasError())
    {
        DUSK_ERROR("Error in getting physical devices {}", result.toString());
        return result.getErrorId();
    }

    if (deviceCount == 0u)
    {
        DUSK_ERROR("No GPU available with vulkan support");
        return Error::NotFound;
    }

    DynamicArray<VkPhysicalDevice> physicalDevices(deviceCount);
    result = vkEnumeratePhysicalDevices(m_instance, &deviceCount, physicalDevices.data());

    if (result.hasError())
    {
        DUSK_ERROR("Error in getting physical devices {}", result.toString());
        return result.getErrorId();
    }

    struct PhysicalDeviceInfo
    {
        VkPhysicalDeviceProperties deviceProperties;

        // set features info which have to be enabled
        VkPhysicalDeviceFeatures2                deviceFeatures2         = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2, &deviceFeaturesVk11 };
        VkPhysicalDeviceVulkan11Features         deviceFeaturesVk11      = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES, &deviceFeaturesVk12 };
        VkPhysicalDeviceVulkan12Features         deviceFeaturesVk12      = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES, &dynamicRenderingFeature };
        VkPhysicalDeviceDynamicRenderingFeatures dynamicRenderingFeature = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR };

        DynamicArray<const char*>                activeDeviceExtensions;

        uint32_t                                 graphicsQueueIndex;
        uint32_t                                 computeQueueIndex;
        uint32_t                                 transferQueueIndex;
        uint32_t                                 presentQueueIndex;

        // device is eligible for use
        bool isSupported = false;
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
        DUSK_INFO("- driver version {}", deviceIndex, pDeviceInfo->deviceProperties.driverVersion);

        // pNext chaining to gather all the supported features
        VkPhysicalDeviceVulkan12Features deviceFeaturesVk12 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
        VkPhysicalDeviceVulkan11Features deviceFeaturesVk11 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES, &deviceFeaturesVk12 };
        VkPhysicalDeviceFeatures2        deviceFeatures2    = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2, &deviceFeaturesVk11 };
        const VkPhysicalDeviceFeatures&  deviceFeatures     = deviceFeatures2.features;

        // Get supported features for this device
        vkGetPhysicalDeviceFeatures2(physicalDevice, &deviceFeatures2);

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

            VkBool32     supportsPresent = false;
            VulkanResult result          = vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, familyIndex, m_surface, &supportsPresent);
            if (result.hasError())
            {
                DUSK_ERROR("Unable to query surface support {}", result.toString());
                return result.getErrorId();
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
        uint32_t deviceExtensionCount = 0u;
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

        // Checking required features and enabling them for the current device

        // Enable dynamic rendering
        if (!availableExtensionsSet.has(hash(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME)))
        {
            DUSK_INFO("Skipping device because it does not support dynamic rendering {}", VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);
            continue;
        }
        pDeviceInfo->dynamicRenderingFeature.dynamicRendering = VK_TRUE;
        pDeviceInfo->activeDeviceExtensions.push_back(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);

        // check samplerAnisotropy
        if (!deviceFeatures.samplerAnisotropy)
        {
            DUSK_INFO("Skipping device because it does not support samplerAnisotropy");
            continue;
        }
        pDeviceInfo->deviceFeatures2.features.samplerAnisotropy = VK_TRUE;

        // check for gpu profiling extension/features
#ifdef DUSK_ENABLE_PROFILING
        if (!availableExtensionsSet.has(hash(VK_EXT_CALIBRATED_TIMESTAMPS_EXTENSION_NAME)))
        {
            DUSK_INFO("Skipping device because it does not support calibrated timestamps");
            continue;
        }
        pDeviceInfo->activeDeviceExtensions.push_back(VK_EXT_CALIBRATED_TIMESTAMPS_EXTENSION_NAME);

        if (!availableExtensionsSet.has(hash(VK_EXT_HOST_QUERY_RESET_EXTENSION_NAME)))
        {
            DUSK_INFO("Skipping device because it does not support host query reset extension");
            continue;
        }
        pDeviceInfo->deviceFeaturesVk12.hostQueryReset = VK_TRUE;
        pDeviceInfo->activeDeviceExtensions.push_back(VK_EXT_HOST_QUERY_RESET_EXTENSION_NAME);
#endif

        // check for descriptor indexing
        if (!deviceFeaturesVk12.descriptorIndexing)
        {
            DUSK_INFO("Skipping device because it does not support descriptor indexing");
            continue;
        }
        pDeviceInfo->deviceFeaturesVk12.descriptorIndexing = VK_TRUE;

        // additional features required with descriptor indexing
        pDeviceInfo->deviceFeaturesVk12.descriptorBindingSampledImageUpdateAfterBind  = VK_TRUE;
        pDeviceInfo->deviceFeaturesVk12.shaderSampledImageArrayNonUniformIndexing     = VK_TRUE;

        pDeviceInfo->deviceFeaturesVk12.descriptorBindingStorageBufferUpdateAfterBind = VK_TRUE;
        pDeviceInfo->deviceFeaturesVk12.shaderStorageBufferArrayNonUniformIndexing    = VK_TRUE;

        pDeviceInfo->deviceFeaturesVk12.descriptorBindingStorageImageUpdateAfterBind  = VK_TRUE;
        pDeviceInfo->deviceFeaturesVk12.shaderStorageImageArrayNonUniformIndexing     = VK_TRUE;

        pDeviceInfo->deviceFeaturesVk12.descriptorBindingUniformBufferUpdateAfterBind = VK_TRUE;
        pDeviceInfo->deviceFeaturesVk12.shaderUniformBufferArrayNonUniformIndexing    = VK_TRUE;

        pDeviceInfo->deviceFeaturesVk12.runtimeDescriptorArray                        = VK_TRUE;
        pDeviceInfo->deviceFeaturesVk12.descriptorBindingPartiallyBound               = VK_TRUE;

        // device has all the expected features
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
        return Error::NotFound;
    }

    m_physicalDevice                              = physicalDevices[selectedPhysicalDeviceIndex.value()];
    const PhysicalDeviceInfo* pSelectedDeviceInfo = &physicalDeviceInfo[selectedPhysicalDeviceIndex.value()];

    // queues identification
    constexpr float   transferQueuePriority = 0.0f;
    constexpr float   graphicsQueuePriority = 1.0f;
    constexpr float   computeQueuePriority  = 1.0f;

    HashSet<uint32_t> uniqueQueueIndices;
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
    deviceCreateInfo.pNext                   = &pSelectedDeviceInfo->deviceFeatures2;

    result                                   = vkCreateDevice(m_physicalDevice, &deviceCreateInfo, nullptr, &m_device);

    if (result.hasError())
    {
        DUSK_ERROR("Failed to create logical device {}", result.toString());
        return result.getErrorId();
    }
    DUSK_INFO("Logical device creation successful");

    volkLoadDevice(m_device);

    // fetch graphic queue handle
    vkGetDeviceQueue(m_device, pSelectedDeviceInfo->graphicsQueueIndex, 0, &m_graphicsQueue);
    if (m_graphicsQueue == VK_NULL_HANDLE)
    {
        DUSK_ERROR("Unable to get graphic queue from vulkan device");
        return Error::NotFound;
    }
    m_presentQueue = m_graphicsQueue;

    // fetch compute queue handle
    vkGetDeviceQueue(m_device, pSelectedDeviceInfo->computeQueueIndex, 0, &m_computeQueue);
    if (m_computeQueue == VK_NULL_HANDLE)
    {
        DUSK_ERROR("Unable to get compute queue from vulkan device");
        return Error::NotFound;
    }

    // fetch transfer queue handle
    vkGetDeviceQueue(m_device, pSelectedDeviceInfo->transferQueueIndex, 0, &m_transferQueue);
    if (m_transferQueue == VK_NULL_HANDLE)
    {
        DUSK_ERROR("Unable to get transfer queue from vulkan device");
        return Error::NotFound;
    }

    VkCommandPoolCreateInfo poolInfo {};
    poolInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = pSelectedDeviceInfo->graphicsQueueIndex;

    result                    = vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_commandPool);

    if (result.hasError())
    {
        DUSK_ERROR("Failed to create command pool {}", result.toString());
        return result.getErrorId();
    }
    DUSK_INFO("Command pool created with graphics queue");

#ifdef VK_RENDERER_DEBUG
    vkdebug::setObjectName(
        m_device,
        VK_OBJECT_TYPE_COMMAND_POOL,
        (uint64_t)m_commandPool,
        "cmd_pool");

#endif

    poolInfo                  = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
    poolInfo.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = pSelectedDeviceInfo->transferQueueIndex;

    result                    = vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_transferCommandPool);
    if (result.hasError())
    {
        DUSK_ERROR("Unable to create transfer command pools {}", result.toString());
        return result.getErrorId();
    }
    DUSK_INFO("Command pool created with transfer queue");

#ifdef VK_RENDERER_DEBUG
    vkdebug::setObjectName(
        m_device,
        VK_OBJECT_TYPE_COMMAND_POOL,
        (uint64_t)m_transferCommandPool,
        "transfer_cmd_pool");

#endif

    // setup tracy profiling ctx
#ifdef DUSK_ENABLE_PROFILING
    s_gpuProfilerCtx = TracyVkContextHostCalibrated(
        m_physicalDevice,
        m_device,
        vkResetQueryPool,
        vkGetPhysicalDeviceCalibrateableTimeDomainsEXT,
        vkGetCalibratedTimestampsEXT);
#endif

    s_sharedVkContext.vulkanInstance           = m_instance;
    s_sharedVkContext.physicalDevice           = m_physicalDevice;
    s_sharedVkContext.device                   = m_device;
    s_sharedVkContext.surface                  = m_surface;
    s_sharedVkContext.commandPool              = m_commandPool;
    s_sharedVkContext.transferCommandPool      = m_transferCommandPool;

    s_sharedVkContext.physicalDeviceProperties = pSelectedDeviceInfo->deviceProperties;
    s_sharedVkContext.physicalDeviceFeatures   = pSelectedDeviceInfo->deviceFeatures2.features;

    s_sharedVkContext.graphicsQueueFamilyIndex = pSelectedDeviceInfo->graphicsQueueIndex;
    s_sharedVkContext.presentQueueFamilyIndex  = pSelectedDeviceInfo->presentQueueIndex;
    s_sharedVkContext.computeQueueFamilyIndex  = pSelectedDeviceInfo->computeQueueIndex;
    s_sharedVkContext.transferQueueFamilyIndex = pSelectedDeviceInfo->transferQueueIndex;

    s_sharedVkContext.graphicsQueue            = m_graphicsQueue;
    s_sharedVkContext.presentQueue             = m_presentQueue;
    s_sharedVkContext.computeQueue             = m_computeQueue;
    s_sharedVkContext.transferQueue            = m_transferQueue;
    s_sharedVkContext.transferQueue            = m_transferQueue;
    s_sharedVkContext.transferQueue            = m_transferQueue;

    return Error::Ok;
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
    m_computeQueue  = VK_NULL_HANDLE;
    m_transferQueue = VK_NULL_HANDLE;

    if (m_commandPool != VK_NULL_HANDLE)
        vkDestroyCommandPool(m_device, m_commandPool, nullptr);

    vkDestroyCommandPool(m_device, m_transferCommandPool, nullptr);

#ifdef DUSK_ENABLE_PROFILING
    TracyVkDestroy(s_gpuProfilerCtx);
    s_gpuProfilerCtx = VK_NULL_HANDLE;
#endif

    if (m_device != VK_NULL_HANDLE)
        vkDestroyDevice(m_device, nullptr);
    m_device = VK_NULL_HANDLE;
}

Error VkGfxDevice::populateLayerExtensionNames(const char* pLayerName)
{
    uint32_t     extensionCount = 0;
    VulkanResult result         = vkEnumerateInstanceExtensionProperties(pLayerName, &extensionCount, nullptr);
    if (result.hasError())
    {
        DUSK_ERROR("Vulkan instance extension enumeration failed {}", result.toString());
        return result.getErrorId();
    }

    DynamicArray<VkExtensionProperties> instanceExtensions(extensionCount);
    result = vkEnumerateInstanceExtensionProperties(pLayerName, &extensionCount, instanceExtensions.data());
    if (result.hasError())
    {
        DUSK_ERROR("Vulkan instance extension enumeration failed {}", result.toString());
        return result.getErrorId();
    }

    DUSK_INFO("Supported Vulkan Instance Extensions for layer {}", pLayerName);
    for (const auto& extension : instanceExtensions)
    {
        DUSK_INFO(" - {}", extension.extensionName);
        m_instanceExtensionsSet.emplace(hash(extension.extensionName));
    }

    return Error::Ok;
}

bool VkGfxDevice::hasInstanceExtension(const char* pExtensionName)
{
    return m_instanceExtensionsSet.has(hash(pExtensionName));
}

VkCommandBuffer VkGfxDevice::beginSingleTimeCommands() const
{
    VkCommandBufferAllocateInfo allocInfo {};
    allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool        = m_commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(m_device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void VkGfxDevice::endSingleTimeCommands(VkCommandBuffer commandBuffer) const
{
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo {};
    submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers    = &commandBuffer;

    vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(m_graphicsQueue);

    vkFreeCommandBuffers(m_device, m_commandPool, 1, &commandBuffer);
}

VkCommandBuffer VkGfxDevice::beginSingleTimeTransferCommands() const
{
    VkCommandBufferAllocateInfo allocInfo {};
    allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool        = m_transferCommandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(m_device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void VkGfxDevice::endSingleTimeTransferCommands(VkCommandBuffer commandBuffer) const
{
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo {};
    submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers    = &commandBuffer;

    vkQueueSubmit(m_transferQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(m_transferQueue);

    vkFreeCommandBuffers(m_device, m_transferCommandPool, 1, &commandBuffer);
}

VulkanResult VkGfxDevice::createBuffer(const GfxBufferParams& params, VulkanGfxBuffer* pOutBuffer)
{
    VkBufferCreateInfo bufferCreateInfo { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    bufferCreateInfo.size        = params.sizeInBytes;
    bufferCreateInfo.usage       = vulkan::getBufferUsageFlagBits(params.usage);
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    // allocation
    VulkanResult result = vulkan::allocateGPUBuffer(m_gpuAllocator, bufferCreateInfo, VMA_MEMORY_USAGE_AUTO, vulkan::getVmaAllocationCreateFlagBits(params.memoryType), pOutBuffer);

    if (result.hasError())
    {
        DUSK_ERROR("Error in creating buffer {}", result.toString());
    }

#ifdef VK_RENDERER_DEBUG
    vkdebug::setObjectName(
        m_device,
        VK_OBJECT_TYPE_BUFFER,
        (uint64_t)pOutBuffer->buffer,
        params.debugName.c_str());
#endif

    pOutBuffer->alignmentSize = params.alignmentSize;
    return result;
}

void VkGfxDevice::freeBuffer(VulkanGfxBuffer* buffer)
{
    vulkan::freeGPUBuffer(m_gpuAllocator, buffer);
}

void VkGfxDevice::copyBuffer(const VulkanGfxBuffer& srcBuffer, VulkanGfxBuffer& dstBuffer, VkDeviceSize size)
{
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    VkBufferCopy    copyRegion {};
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    copyRegion.size      = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer.buffer, dstBuffer.buffer, 1, &copyRegion);

    endSingleTimeCommands(commandBuffer);
}

void VkGfxDevice::mapBuffer(VulkanGfxBuffer* buffer)
{
    vulkan::mapGPUMemory(m_gpuAllocator, buffer->allocation, buffer->mappedMemory);
}

void VkGfxDevice::unmapBuffer(VulkanGfxBuffer* buffer)
{
    vulkan::unmapGPUMemory(m_gpuAllocator, buffer->allocation);
}

void VkGfxDevice::flushBuffer(VulkanGfxBuffer* buffer)
{
    vulkan::flushCPUMemory(m_gpuAllocator, { buffer->allocation }, { 0 }, { buffer->sizeInBytes });
}

void VkGfxDevice::flushBufferOffset(VulkanGfxBuffer* buffer, uint32_t offset, size_t size)
{
    vulkan::flushCPUMemory(m_gpuAllocator, { buffer->allocation }, { offset }, { size });
}

void VkGfxDevice::writeToBuffer(VulkanGfxBuffer* buffer, void* hostBlock, VkDeviceSize offset, VkDeviceSize size)
{
    vulkan::writeToAllocation(m_gpuAllocator, hostBlock, buffer->allocation, offset, size);
}

void VkGfxDevice::copyBufferToImage(
    VulkanGfxBuffer* buffer,
    VulkanGfxImage*  img,
    uint32_t         width,
    uint32_t         height)
{
    VkCommandBuffer   commandBuffer = beginSingleTimeCommands();

    VkBufferImageCopy region {};
    region.bufferOffset                    = 0;
    region.bufferRowLength                 = 0;
    region.bufferImageHeight               = 0;

    region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel       = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount     = 1;

    region.imageOffset                     = { 0, 0, 0 };
    region.imageExtent                     = {
        width,
        height,
        1
    };

    vkCmdCopyBufferToImage(
        commandBuffer,
        buffer->buffer,
        img->vkImage,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region);

    endSingleTimeCommands(commandBuffer);
}

void VkGfxDevice::copyBufferToImageRegions(
    VulkanGfxBuffer*                 buffer,
    VulkanGfxImage*                  img,
    DynamicArray<VkBufferImageCopy>& regions)
{
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    vkCmdCopyBufferToImage(
        commandBuffer,
        buffer->buffer,
        img->vkImage,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        regions.size(),
        regions.data());

    endSingleTimeCommands(commandBuffer);
}

Error VkGfxDevice::transitionImageWithLayout(
    VulkanGfxImage* img,
    VkFormat        format,
    VkImageLayout   oldLayout,
    VkImageLayout   newLayout,
    uint32_t        mipLevelCount,
    uint32_t        layersCount)
{
    VkCommandBuffer      commandBuffer = beginSingleTimeCommands();

    VkImageMemoryBarrier barrier {};
    barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout                       = oldLayout;
    barrier.newLayout                       = newLayout;
    barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barrier.image                           = img->vkImage;
    barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel   = 0;
    barrier.subresourceRange.levelCount     = mipLevelCount;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount     = layersCount;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        sourceStage           = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        barrier.srcAccessMask = 0;

        destinationStage      = VK_PIPELINE_STAGE_TRANSFER_BIT;
        barrier.dstAccessMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        sourceStage           = VK_PIPELINE_STAGE_TRANSFER_BIT;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        destinationStage      = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    }
    else
    {
        return Error::NotSupported;
    }

    vkCmdPipelineBarrier(
        commandBuffer,
        sourceStage,
        destinationStage,
        0,
        0,
        nullptr,
        0,
        nullptr,
        1,
        &barrier);

    endSingleTimeCommands(commandBuffer);

    return Error::Ok;
}

VulkanResult VkGfxDevice::createImageView(
    VulkanGfxImage*    img,
    VkImageViewType    type,
    VkFormat           format,
    VkImageAspectFlags aspectMaskFlags,
    uint32_t           mipLevelCount,
    uint32_t           layersCount,
    VkImageView*       pImageView) const
{
    VkImageViewCreateInfo viewInfo {};
    viewInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image                           = img->vkImage;
    viewInfo.viewType                        = type;
    viewInfo.format                          = format;
    viewInfo.subresourceRange.aspectMask     = aspectMaskFlags;
    viewInfo.subresourceRange.baseMipLevel   = 0;
    viewInfo.subresourceRange.levelCount     = mipLevelCount;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount     = layersCount;

    VulkanResult result                      = vkCreateImageView(m_device, &viewInfo, nullptr, pImageView);

    if (result.hasError())
    {
        DUSK_ERROR("Unable to create image view {}", result.toString());
    }
    return result;
}

void VkGfxDevice::freeImageView(VkImageView* pImageView) const
{
    vkDestroyImageView(m_device, *pImageView, nullptr);
}

VulkanResult VkGfxDevice::createImageSampler(VulkanSampler* sampler) const
{
    VkSamplerCreateInfo samplerInfo {};
    samplerInfo.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter               = VK_FILTER_LINEAR;
    samplerInfo.minFilter               = VK_FILTER_LINEAR;
    samplerInfo.addressModeU            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable        = VK_FALSE;
    samplerInfo.maxAnisotropy           = s_sharedVkContext.physicalDeviceProperties.limits.maxSamplerAnisotropy;
    samplerInfo.borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable           = VK_FALSE;
    samplerInfo.compareOp               = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.minLod                  = 0.0f;
    samplerInfo.maxLod                  = VK_LOD_CLAMP_NONE;
    samplerInfo.mipLodBias              = 0.0f;

    VulkanResult result                 = vkCreateSampler(m_device, &samplerInfo, nullptr, &sampler->sampler);

    if (result.hasError())
    {
        DUSK_ERROR("Unable to create image sampler {}", result.toString());
    }

    return result;
}

void VkGfxDevice::freeImageSampler(VulkanSampler* sampler) const
{
    vkDestroySampler(m_device, sampler->sampler, nullptr);
}

#ifdef VK_RENDERER_DEBUG
VKAPI_ATTR VkBool32 VKAPI_CALL VkGfxDevice::vulkanDebugMessengerCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT             messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void*                                       pUserData)
{
    const auto& logger = Logger::getVulkanLogger();
    if (messageSeverity & (VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT))
    {
        logger->info("{}", pCallbackData->pMessage);
        if (pCallbackData->cmdBufLabelCount)
        {
            logger->info(" debug labels:");
            for (size_t i = 0u; i < pCallbackData->cmdBufLabelCount; ++i)
            {
                const size_t reverseIndex = pCallbackData->cmdBufLabelCount - 1u - i;
                logger->info("     - {}", pCallbackData->pCmdBufLabels[reverseIndex].pLabelName);
            }
        }
    }

    if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    {
        logger->warn("Vulkan: {}", pCallbackData->pMessage);
    }

    if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
    {
        logger->error("Vulkan: {}", pCallbackData->pMessage);
    }

    return VK_FALSE;
}
#endif

Error VkGfxDevice::populateLayerNames()
{
    uint32_t     layerCount = 0;
    VulkanResult result     = vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    if (result.hasError())
    {
        DUSK_ERROR("Vulkan instance layers enumeration failed {}", result.toString());
        return result.getErrorId();
    }

    DynamicArray<VkLayerProperties> instanceLayers(layerCount);
    result = vkEnumerateInstanceLayerProperties(&layerCount, instanceLayers.data());
    if (result.hasError())
    {
        DUSK_ERROR("Vulkan instance layers enumeration failed {}", result.toString());
        return result.getErrorId();
    }

    DUSK_INFO("Supported Vulkan layers");
    for (const auto& layer : instanceLayers)
    {
        DUSK_INFO(" - {}", layer.layerName);
        m_layersSet.emplace(hash(layer.layerName));
    }

    return Error::Ok;
}

bool VkGfxDevice::hasLayer(const char* pLayerName)
{
    return m_layersSet.has(hash(pLayerName));
}

} // namespace dusk