#include "vk_swapchain.h"

#include <limits>
#include <algorithm>

namespace dusk
{
Error VkGfxSwapChain::create(VulkanContext& context, VkGfxSwapChainParams& params)
{
    m_physicalDevice = context.physicalDevice;
    m_device         = context.device;
    m_surface        = context.surface;

    return createSwapChain(params);
}

void VkGfxSwapChain::destroy()
{
    destroyImageViews();

    // TODO: handle double destroy of swapchain,
    // can trigger after failed image view creation
    destroySwapChain();
}

void VkGfxSwapChain::resize()
{
}

Error VkGfxSwapChain::initFrameBuffers(VkGfxRenderPass& renderPass)
{
    m_frameBuffers.resize(m_swapChainImageViews.size());

    for (uint32_t i = 0u; i < m_swapChainImageViews.size(); ++i)
    {
        Array<VkImageView, 1> attachments[] = {
            m_swapChainImageViews[i],
        };

        VkFramebufferCreateInfo framebufferInfo {};
        framebufferInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass      = renderPass.m_renderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments->size());
        framebufferInfo.pAttachments    = attachments->data();
        framebufferInfo.width           = m_currentExtent.width;
        framebufferInfo.height          = m_currentExtent.height;
        framebufferInfo.layers          = 1;

        VulkanResult result             = vkCreateFramebuffer(m_device, &framebufferInfo, nullptr, &m_frameBuffers[i]);

        if (result.hasError())
        {
            DUSK_ERROR("Unable to creat framebuffers {}");
            return result.getErrorId();
        }
    }

    return Error::Ok;
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
    DUSK_INFO("Selected surface format {}: colorspace {}", getVkFormatString(surfaceFormat.format), getVkColorSpaceString(surfaceFormat.colorSpace));

    VkPresentModeKHR presentMode = getPresentMode();
    DUSK_INFO("Selecting present mode {}", getVkPresentModeString(presentMode));

    VkExtent2D extent = getSwapExtent(params.windowWidth, params.windowHeight);
    DUSK_INFO("Selecting extent width={}, height={}", extent.width, extent.height);

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
    createInfo.imageExtent           = extent;
    createInfo.imageArrayLayers      = 1;
    createInfo.imageUsage            = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    createInfo.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.queueFamilyIndexCount = 0u;
    createInfo.pQueueFamilyIndices   = nullptr;

    createInfo.preTransform          = transform;
    createInfo.compositeAlpha        = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode           = presentMode;
    createInfo.clipped               = VK_TRUE;
    createInfo.oldSwapchain          = params.oldSwapChain;

    // create swapchain now
    result = vkCreateSwapchainKHR(m_device, &createInfo, nullptr, &m_swapChain);
    if (result.hasError())
    {
        DUSK_ERROR("Swap chain creation failed {}", result.toString());
        return result.getErrorId();
    }

    Error err = createImageViews(surfaceFormat.format);
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
    for (auto imageView : m_swapChainImageViews)
    {
        vkDestroyImageView(m_device, imageView, nullptr);
    }
    m_swapChainImageViews.clear();

    if (m_swapChain != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(m_device, m_swapChain, nullptr);
        m_swapChain = VK_NULL_HANDLE;
    }

    for (auto framebuffer : m_frameBuffers)
    {
        vkDestroyFramebuffer(m_device, framebuffer, nullptr);
    }
    m_frameBuffers.clear();
}

// TODO: this functionality should go into VkGfxDevice
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

        VulkanResult result                        = vkCreateImageView(m_device, &createInfo, nullptr, &m_swapChainImageViews[i]);
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
        DUSK_INFO("- format {}: colorspace {}", getVkFormatString(surfaceFormat.format), getVkColorSpaceString(surfaceFormat.colorSpace));

        if (surfaceFormat.format == VK_FORMAT_B8G8R8A8_SRGB && surfaceFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            selectedSurfaceFormat = surfaceFormat;
            ;
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

        DUSK_INFO("- {}", getVkPresentModeString(presentMode));

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

} // namespace dusk