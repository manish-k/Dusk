#pragma once

#include "dusk.h"
#include "vk.h"
#include "vk_types.h"
#include "vk_renderpass.h"

namespace dusk
{
struct VkGfxSwapChainParams
{
    uint32_t       windowWidth;
    uint32_t       windowHeight;

    VkSwapchainKHR oldSwapChain;
};

class VkGfxSwapChain
{
public:
    Error          create(VulkanContext& vkContext, VkGfxSwapChainParams& params);
    void           destroy();

    void           resize();

    Error          initFrameBuffers(VkGfxRenderPass& renderPass);

    VkSwapchainKHR getSwapChain() const { return m_swapChain; }
    uint32_t       getImagesCount() const { return m_imagesCount; }

private:
    Error              createSwapChain(const VkGfxSwapChainParams& params);
    void               destroySwapChain();

    Error              createImageViews(VkFormat format);
    void               destroyImageViews();

    Error              createSyncObjects();

    VkSurfaceFormatKHR getBestSurfaceFormat() const;
    VkPresentModeKHR   getPresentMode() const;
    VkExtent2D         getSwapExtent(uint32_t width, uint32_t height) const;

private:
    VkPhysicalDevice            m_physicalDevice = VK_NULL_HANDLE;
    VkDevice                    m_device         = VK_NULL_HANDLE;
    VkSurfaceKHR                m_surface        = VK_NULL_HANDLE;
    VkSwapchainKHR              m_swapChain      = VK_NULL_HANDLE;

    VkSurfaceCapabilitiesKHR    m_capabilities;

    uint32_t                    m_imagesCount;
    DynamicArray<VkImage>       m_swapChainImages;
    DynamicArray<VkImageView>   m_swapChainImageViews;
    DynamicArray<VkFramebuffer> m_frameBuffers;

    VkExtent2D                  m_currentExtent;

    DynamicArray<VkSemaphore>   m_imageAvailableSemaphores;
    DynamicArray<VkSemaphore>   m_renderFinishedSemaphores;

    DynamicArray<VkFence>       m_inFlightFences;
    DynamicArray<VkFence>       m_imagesInFlight;

    uint32_t currentFrame = 0u;
};
} // namespace dusk