#pragma once

#include "vk_device.h"

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

namespace dusk
{
VulkanContext VkGfxDevice::s_sharedVkContext = VulkanContext {};

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
        VkPhysicalDeviceFeatures   deviceFeatures;
        DynamicArray<const char*>  activeDeviceExtensions;

        uint32_t                   graphicsQueueIndex;
        uint32_t                   computeQueueIndex;
        uint32_t                   transferQueueIndex;
        uint32_t                   presentQueueIndex;

        bool                       isSupported = false;
    };

    DynamicArray<PhysicalDeviceInfo> physicalDeviceInfo(deviceCount);

    // dynamic rendering feature info
    VkPhysicalDeviceDynamicRenderingFeaturesKHR dynamicRenderingFeature = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR };

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

        // Enable dynamic rendering
        if (!availableExtensionsSet.has(hash(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME)))
        {
            DUSK_INFO("Skipping device because it does not support dynamic rendering {}", VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);
            continue;
        }

        dynamicRenderingFeature.dynamicRendering = VK_TRUE;
        pDeviceInfo->activeDeviceExtensions.push_back(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);

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
    deviceCreateInfo.pNext                   = &dynamicRenderingFeature;

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

    s_sharedVkContext.vulkanInstance           = m_instance;
    s_sharedVkContext.physicalDevice           = m_physicalDevice;
    s_sharedVkContext.device                   = m_device;
    s_sharedVkContext.surface                  = m_surface;
    s_sharedVkContext.commandPool              = m_commandPool;

    s_sharedVkContext.physicalDeviceProperties = pSelectedDeviceInfo->deviceProperties;
    s_sharedVkContext.physicalDeviceFeatures   = pSelectedDeviceInfo->deviceFeatures;

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

VkCommandBuffer VkGfxDevice::beginSingleTimeCommands()
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

void VkGfxDevice::endSingleTimeCommands(VkCommandBuffer commandBuffer)
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

VulkanResult VkGfxDevice::createBuffer(const GfxBufferParams& params, VulkanGfxBuffer* pOutBuffer)
{
    VkBufferCreateInfo bufferCreateInfo { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    bufferCreateInfo.size        = params.sizeInBytes;
    bufferCreateInfo.usage       = getBufferUsageFlagBits(params.usage);
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    // allocation
    VulkanResult result = vulkan::allocateGPUBuffer(m_gpuAllocator, bufferCreateInfo, VMA_MEMORY_USAGE_AUTO, getVmaAllocationCreateFlagBits(params.memoryType), pOutBuffer);

    if (result.hasError())
    {
        DUSK_ERROR("Error in creating buffer {}", result.toString());
    }

    return result;
}

void VkGfxDevice::freeBuffer(VulkanGfxBuffer* buffer)
{
    vulkan::freeGPUBuffer(m_gpuAllocator, buffer);
}

void VkGfxDevice::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    VkBufferCopy    copyRegion {};
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    copyRegion.size      = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

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

void VkGfxDevice::writeToBuffer(VulkanGfxBuffer* buffer, void* hostBlock, VkDeviceSize offset, VkDeviceSize size)
{
    vulkan::writeToAllocation(m_gpuAllocator, hostBlock, buffer->allocation, offset, size);
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