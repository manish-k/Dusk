#include "vk_swapchain.h"

#include "debug/profiler.h"

#include <limits>
#include <algorithm>

namespace dusk
{
Error VkGfxSwapChain::create(VulkanContext& vkContext, VkGfxSwapChainParams& params, Shared<VkGfxSwapChain> oldSwapChain)
{
    m_physicalDevice           = vkContext.physicalDevice;
    m_device                   = vkContext.device;
    m_surface                  = vkContext.surface;

    m_graphicsQueueFamilyIndex = vkContext.graphicsQueueFamilyIndex;
    m_computeQueueFamilyIndex  = vkContext.computeQueueFamilyIndex;

    m_graphicsQueue            = vkContext.graphicsQueue;
    m_presentQueue             = vkContext.presentQueue;
    m_computeQueue             = vkContext.computeQueue;
    m_transferQueue            = vkContext.transferQueue;

    m_oldSwapChain             = oldSwapChain;

    Error err                  = createSwapChain(params);
    if (err != Error::Ok)
    {
        return err;
    }

    return err;
}

void VkGfxSwapChain::destroy()
{
    destroyImageViews();

    if (m_oldSwapChain)
    {
        m_oldSwapChain->destroy();
    }

    // TODO: handle double destroy of swapchain,
    // can trigger after failed image view creation
    destroySwapChain();
}

VulkanResult VkGfxSwapChain::acquireNextImage(uint32_t frameIndex, uint32_t* imageIndex)
{
    DUSK_PROFILE_FUNCTION;

    {
        DUSK_PROFILE_SECTION("fence_wait");
        vkWaitForFences(
            m_device, 1, &m_inFlightFences[frameIndex], VK_TRUE, UINT64_MAX);
    }

    {
        DUSK_PROFILE_SECTION("reset_fence");
        vkResetFences(m_device, 1, &m_inFlightFences[frameIndex]);
    }

    VulkanResult result = vkAcquireNextImageKHR(m_device, m_swapChain, UINT64_MAX, m_imageAvailableSemaphores[frameIndex], VK_NULL_HANDLE, imageIndex);

    if (result.hasError())
    {
        DUSK_ERROR("Unable to acquire swapchain image {}", result.toString());
    }

    return result;
}

VulkanResult VkGfxSwapChain::submitCommandBuffers(
    DynamicArray<VulkanSubmitBatch>& batches,
    uint32_t                         frameIndex,
    uint32_t                         imageIndex)
{
    DUSK_PROFILE_FUNCTION;

    uint32_t    submtiCount          = static_cast<uint32_t>(batches.size());

    uint32_t    batchTimelineCounter = 0u;
    VkSemaphore timelineSemaphore    = m_submitSemaphores[frameIndex];

    // DUSK_DEBUG("Submitting for frame={}, image={}", frameIndex, imageIndex);

    {
        DUSK_PROFILE_SECTION("batch_submits");

        for (uint32_t batchIndex = 0u; batchIndex < submtiCount; ++batchIndex)
        {
            auto&                                   batch = batches[batchIndex];

            DynamicArray<VkSemaphoreSubmitInfo>     waitSemaphoreSubmitInfos;
            DynamicArray<VkSemaphoreSubmitInfo>     signalSemaphoreSubmitInfos;
            DynamicArray<VkCommandBufferSubmitInfo> commandBufferSubmitInfos;

            waitSemaphoreSubmitInfos.reserve(2u);
            signalSemaphoreSubmitInfos.reserve(2u);
            commandBufferSubmitInfos.reserve(2u);

            VkQueue queue = m_graphicsQueue;

            if (batch.targetQueueFamily == m_computeQueueFamilyIndex)
            {
                queue = m_computeQueue;
            }

            // DUSK_DEBUG("[{}] Submitting batch {}: present={}", batch.targetQueueFamily, batchIndex, batch.isPresentBatch);
            // DUSK_DEBUG("[{}] batch {}: global={}, wait={}, signal={}", batch.targetQueueFamily, batchIndex, m_globalTimelineCounter, batch.semaphoreWaitValue, batch.semaphoreSignalValue);

            if (batch.semaphoreWaitValue > 0)
            {
                VkSemaphoreSubmitInfo waitInfo { VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO };
                waitInfo.semaphore = timelineSemaphore;
                waitInfo.value     = m_globalTimelineCounter + batch.semaphoreWaitValue;
                waitInfo.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
                waitSemaphoreSubmitInfos.push_back(waitInfo);
            }

            if (batch.semaphoreSignalValue > 0)
            {
                VkSemaphoreSubmitInfo signalInfo { VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO };
                signalInfo.semaphore = timelineSemaphore;
                signalInfo.value     = m_globalTimelineCounter + batch.semaphoreSignalValue;
                signalInfo.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;

                batchTimelineCounter = std::max(batchTimelineCounter, batch.semaphoreSignalValue);

                signalSemaphoreSubmitInfos.push_back(signalInfo);
            }

            VkFence frameFence = VK_NULL_HANDLE;
            if (batchIndex == submtiCount - 1)
            {
                // wait on acquire image
                VkSemaphoreSubmitInfo waitInfo { VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO };
                waitInfo.semaphore = m_imageAvailableSemaphores[frameIndex];
                waitInfo.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
                waitSemaphoreSubmitInfos.push_back(waitInfo);

                // add a signal for presentation queue
                VkSemaphoreSubmitInfo presentSignalInfo { VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO };
                presentSignalInfo.semaphore = m_renderFinishedSemaphores[frameIndex];
                presentSignalInfo.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;

                signalSemaphoreSubmitInfos.push_back(presentSignalInfo);

                frameFence = m_inFlightFences[frameIndex];
            }

            VkCommandBufferSubmitInfo cmdBufferInfo { VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO };
            cmdBufferInfo.commandBuffer = batch.recordedBuffer;

            commandBufferSubmitInfos.push_back(cmdBufferInfo);

            VkSubmitInfo2 submitInfo            = { VK_STRUCTURE_TYPE_SUBMIT_INFO_2 };
            submitInfo.waitSemaphoreInfoCount   = waitSemaphoreSubmitInfos.size();
            submitInfo.pWaitSemaphoreInfos      = waitSemaphoreSubmitInfos.data();
            submitInfo.commandBufferInfoCount   = commandBufferSubmitInfos.size();
            submitInfo.pCommandBufferInfos      = commandBufferSubmitInfos.data();
            submitInfo.signalSemaphoreInfoCount = signalSemaphoreSubmitInfos.size();
            submitInfo.pSignalSemaphoreInfos    = signalSemaphoreSubmitInfos.data();

            // submit to corresponding queue
            VulkanResult result = vkQueueSubmit2(
                queue,
                1u,
                &submitInfo,
                frameFence);

            if (result.hasError())
            {
                DUSK_ERROR("Failed to submit batch to queue family {}: {}", batch.targetQueueFamily, result.toString());
                return result;
            }
        }
    }

    m_globalTimelineCounter += batchTimelineCounter;

    VulkanResult result;
    {
        DUSK_PROFILE_SECTION("presentation");
        VkSemaphore      signalSemaphores[] = { m_renderFinishedSemaphores[frameIndex] };

        VkPresentInfoKHR presentInfo        = {};
        presentInfo.sType                   = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        presentInfo.waitSemaphoreCount      = 1u;
        presentInfo.pWaitSemaphores         = signalSemaphores;

        VkSwapchainKHR swapChains[]         = { m_swapChain };
        presentInfo.swapchainCount          = 1u;
        presentInfo.pSwapchains             = swapChains;

        presentInfo.pImageIndices           = &imageIndex;

        // present image
        result = vkQueuePresentKHR(m_presentQueue, &presentInfo);

        if (result.hasError())
        {
            DUSK_ERROR("Failed to present image {}", result.toString());
            return result;
        }
    }

    return result;
}

Error VkGfxSwapChain::createSwapChain(const VkGfxSwapChainParams& params)
{
    VulkanResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
        m_physicalDevice,
        m_surface,
        &m_capabilities);

    if (result.hasError())
    {
        DUSK_ERROR("Unable to get surface capabilities of vulkan device {}", result.toString());
        return result.getErrorId();
    }

    VkSurfaceFormatKHR surfaceFormat = getBestSurfaceFormat();
    DUSK_INFO("Selected surface format {}: colorspace {}", vulkan::getVkFormatString(surfaceFormat.format), vulkan::getVkColorSpaceString(surfaceFormat.colorSpace));
    m_imageFormat                = surfaceFormat.format;

    VkPresentModeKHR presentMode = getPresentMode();
    DUSK_INFO("Selecting present mode {}", vulkan::getVkPresentModeString(presentMode));

    VkExtent2D extent = getSwapExtent(params.windowWidth, params.windowHeight);
    DUSK_INFO("Selecting extent width={}, height={}", extent.width, extent.height);
    m_currentExtent = extent;

    DUSK_INFO("Surface capabilities minImageCount={} and maxImageCount={}", m_capabilities.minImageCount, m_capabilities.maxImageCount);
    m_imagesCount = m_capabilities.minImageCount + 1;
    if (m_capabilities.maxImageCount > 0
        && m_imagesCount > m_capabilities.maxImageCount)
    {
        m_imagesCount = m_capabilities.maxImageCount;
    }
    DUSK_INFO("Setting image count {} for swapchain images", m_imagesCount);

    VkSurfaceTransformFlagBitsKHR transform = m_capabilities.currentTransform;
    if (m_capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
    {
        transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    }

    VkSwapchainCreateInfoKHR createInfo {};
    createInfo.sType                 = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface               = m_surface;

    createInfo.minImageCount         = m_imagesCount;
    createInfo.imageFormat           = surfaceFormat.format;
    createInfo.imageColorSpace       = surfaceFormat.colorSpace;
    createInfo.imageExtent           = m_currentExtent;
    createInfo.imageArrayLayers      = 1;
    createInfo.imageUsage            = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    createInfo.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.queueFamilyIndexCount = 0u;
    createInfo.pQueueFamilyIndices   = nullptr;

    createInfo.preTransform          = transform;
    createInfo.compositeAlpha        = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode           = presentMode;
    createInfo.clipped               = VK_TRUE;

    if (m_oldSwapChain != nullptr)
    {
        createInfo.oldSwapchain = m_oldSwapChain->getVkSwapChain();
    }

    // create swapchain now
    result = vkCreateSwapchainKHR(m_device, &createInfo, nullptr, &m_swapChain);
    if (result.hasError())
    {
        DUSK_ERROR("Swap chain creation failed {}", result.toString());
        return result.getErrorId();
    }

    result = vkGetSwapchainImagesKHR(m_device, m_swapChain, &m_imagesCount, nullptr);
    if (result.hasError())
    {
        DUSK_ERROR("Unable to get images handle count {}", result.toString());
        return result.getErrorId();
    }

    m_swapChainImages.resize(m_imagesCount);
    result = vkGetSwapchainImagesKHR(m_device, m_swapChain, &m_imagesCount, m_swapChainImages.data());
    if (result.hasError())
    {
        DUSK_ERROR("Unable to get images handle {}", result.toString());
        return result.getErrorId();
    }

    for (uint32_t imgIndex = 0u; imgIndex < m_swapChainImages.size(); ++imgIndex)
    {
#ifdef VK_RENDERER_DEBUG
        std::string imgName = "swapchain_img_" + std::to_string(imgIndex);
        vkdebug::setObjectName(
            m_device,
            VK_OBJECT_TYPE_IMAGE,
            (uint64_t)m_swapChainImages[imgIndex],
            (imgName).c_str());
#endif // VK_RENDERER_DEBUG
    }

    Error err = createImageViews(surfaceFormat.format);
    if (err != Error::Ok)
    {
        destroySwapChain();
        return err;
    }

    err = createSyncObjects();
    if (err != Error::Ok)
    {
        destroySwapChain();
        return err;
    }

    DUSK_INFO("Swapchain created successfully");
    return Error::Ok;
}

void VkGfxSwapChain::destroySwapChain()
{
    if (m_swapChain != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(m_device, m_swapChain, nullptr);
        m_swapChain = VK_NULL_HANDLE;
    }

    destroySyncObjects();
}

// TODO: maybe this functionality should go into VkGfxDevice
Error VkGfxSwapChain::createImageViews(VkFormat format)
{
    m_swapChainImageViews.resize(m_swapChainImages.size());

    for (uint32_t i = 0u; i < m_swapChainImages.size(); i++)
    {
        VkImageViewCreateInfo createInfo {};
        createInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image                           = m_swapChainImages[i];
        createInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format                          = format;

        createInfo.components.r                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY;

        createInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel   = 0;
        createInfo.subresourceRange.levelCount     = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount     = 1;

        // create image views
        VulkanResult result = vkCreateImageView(m_device, &createInfo, nullptr, &m_swapChainImageViews[i]);
        if (result.hasError())
        {
            DUSK_ERROR("Unable to create image view of swapchain images {}", result.toString());
            return result.getErrorId();
        }
    }

    return Error();
}

void VkGfxSwapChain::destroyImageViews()
{
    for (auto imageView : m_swapChainImageViews)
    {
        vkDestroyImageView(m_device, imageView, nullptr);
    }
    m_swapChainImageViews.clear();
}

Error VkGfxSwapChain::createSyncObjects()
{
    m_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_submitSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        VulkanResult result = vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]);
        if (result.hasError())
        {
            DUSK_ERROR("Unable to create imageAvailableSemaphore {}", result.toString());
            return result.getErrorId();
        }

        result = vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]);
        if (result.hasError())
        {
            DUSK_ERROR("Unable to create renderFinishedSemaphores {}", result.toString());
            return result.getErrorId();
        }

        // allocate timeline semaphores for submitting command buffers
        VkSemaphoreTypeCreateInfo timelineCreateInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO };
        timelineCreateInfo.pNext                     = nullptr;
        timelineCreateInfo.semaphoreType             = VK_SEMAPHORE_TYPE_TIMELINE;
        timelineCreateInfo.initialValue              = 0;

        // chain timeline create info to semaphore create info
        VkSemaphoreCreateInfo submiSemaphoreCreateInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
        submiSemaphoreCreateInfo.pNext                 = &timelineCreateInfo;

        result                                         = vkCreateSemaphore(m_device, &submiSemaphoreCreateInfo, nullptr, &m_submitSemaphores[i]);
        if (result.hasError())
        {
            DUSK_ERROR("Unable to create submit timeline semaphores {}", result.toString());
            return result.getErrorId();
        }

        result = vkCreateFence(m_device, &fenceInfo, nullptr, &m_inFlightFences[i]);
        if (result.hasError())
        {
            DUSK_ERROR("Unable to create inFlight fence {}", result.toString());
            return result.getErrorId();
        }
    }

    return Error::Ok;
}

void VkGfxSwapChain::destroySyncObjects()
{
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        vkDestroySemaphore(m_device, m_imageAvailableSemaphores[i], nullptr);
        vkDestroySemaphore(m_device, m_renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(m_device, m_submitSemaphores[i], nullptr);
        vkDestroyFence(m_device, m_inFlightFences[i], nullptr);
    }
    m_imageAvailableSemaphores.clear();
    m_renderFinishedSemaphores.clear();
    m_submitSemaphores.clear();
    m_inFlightFences.clear();
}

VkSurfaceFormatKHR VkGfxSwapChain::getBestSurfaceFormat() const
{
    uint32_t     formatCount = 0u;
    VulkanResult result      = vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, m_surface, &formatCount, nullptr);

    if (result.hasError())
    {
        DUSK_ERROR("Unable to query surface format count in physical device {}", result.toString());
    }

    DynamicArray<VkSurfaceFormatKHR> surfaceFormats(formatCount);
    result = vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, m_surface, &formatCount, surfaceFormats.data());

    if (result.hasError())
    {
        DUSK_ERROR("Unable to query surface formats supported in physical device {}", result.toString());
    }

    VkSurfaceFormatKHR selectedSurfaceFormat = surfaceFormats[0];
    DUSK_INFO("Supported device surface formats:");
    for (uint32_t i = 0u; i < formatCount; ++i)
    {
        auto& surfaceFormat = surfaceFormats[i];
        DUSK_INFO("- format {}: colorspace {}", vulkan::getVkFormatString(surfaceFormat.format), vulkan::getVkColorSpaceString(surfaceFormat.colorSpace));

        if (surfaceFormat.format == VK_FORMAT_B8G8R8A8_SRGB && surfaceFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            selectedSurfaceFormat = surfaceFormat;
        }
    }

    return selectedSurfaceFormat;
}

VkPresentModeKHR VkGfxSwapChain::getPresentMode() const
{
    uint32_t     presentModeCount = 0u;
    VulkanResult result           = vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, m_surface, &presentModeCount, nullptr);
    if (result.hasError())
    {
        DUSK_ERROR("Unable to get present mode count {}", result.toString());
    }

    DASSERT(presentModeCount != 0, "No present mode supported by physical device")

    DynamicArray<VkPresentModeKHR> presentModes(presentModeCount);
    result = vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, m_surface, &presentModeCount, presentModes.data());
    if (result.hasError())
    {
        DUSK_ERROR("Unable to get present mode count {}", result.toString());
    }

    DUSK_INFO("Supported present modes:");
    VkPresentModeKHR selectedPresentMode = VK_PRESENT_MODE_FIFO_KHR;
    for (uint32_t i = 0u; i < presentModeCount; ++i)
    {
        auto& presentMode = presentModes[i];

        DUSK_INFO("- {}", vulkan::getVkPresentModeString(presentMode));

        if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            selectedPresentMode = presentMode;
        }
    }

    return selectedPresentMode;
}

VkExtent2D VkGfxSwapChain::getSwapExtent(uint32_t width, uint32_t height) const
{
    if (m_capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
        return m_capabilities.currentExtent;
    }
    else
    {
        VkExtent2D actualExtent;
        actualExtent.width = std::max(
            m_capabilities.minImageExtent.width,
            std::min(m_capabilities.maxImageExtent.width, width));
        actualExtent.height = std::max(
            m_capabilities.minImageExtent.height,
            std::min(m_capabilities.maxImageExtent.height, height));

        return actualExtent;
    }
}

VkFormat VkGfxSwapChain::findDepthFormat()
{
    DynamicArray<VkFormat> candidates { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT };

    for (VkFormat format : candidates)
    {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(m_physicalDevice, format, &props);

        if ((props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) == VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
        {
            return format;
        }
    }

    return VkFormat();
}

} // namespace dusk