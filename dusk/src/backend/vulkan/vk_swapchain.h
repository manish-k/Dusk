#pragma once

#include "dusk.h"
#include "vk.h"
#include "vk_types.h"
#include "vk_allocator.h"
#include "renderer/texture.h"

namespace dusk
{
struct VkGfxSwapChainParams
{
    uint32_t windowWidth;
    uint32_t windowHeight;
};

class VkGfxSwapChain
{
public:
    Error          create(VulkanContext& vkContext, VkGfxSwapChainParams& params, Shared<VkGfxSwapChain> oldSwapChain = nullptr);
    void           destroy();

    VulkanResult   acquireNextImage(uint32_t* imageIndex);
    VulkanResult   submitCommandBuffers(const VkCommandBuffer* buffers, uint32_t* imageIndex);

    VkSwapchainKHR getSwapChain() const { return m_swapChain; }
    uint32_t       getImagesCount() const { return m_imagesCount; }
    VkExtent2D     getCurrentExtent() const { return m_currentExtent; }
    VkImageView    getImageView(uint32_t imageIndex) { return m_swapChainImageViews[imageIndex]; }
    VkFormat       getImageFormat() const { return m_imageFormat; }
    VkImage        getImage(uint32_t imageIndex) const { return m_swapChainImages[imageIndex]; }

    GfxTexture     getCurrentSwapImageTexture();

private:
    Error              createSwapChain(const VkGfxSwapChainParams& params);
    void               destroySwapChain();

    Error              createImageViews(VkFormat format);
    void               destroyImageViews();

    Error              createSyncObjects();
    void               destroySyncObjects();

    VkSurfaceFormatKHR getBestSurfaceFormat() const;
    VkPresentModeKHR   getPresentMode() const;
    VkExtent2D         getSwapExtent(uint32_t width, uint32_t height) const;
    VkFormat           findDepthFormat();

private:
    VkPhysicalDevice          m_physicalDevice           = VK_NULL_HANDLE;
    VkDevice                  m_device                   = VK_NULL_HANDLE;
    VkSurfaceKHR              m_surface                  = VK_NULL_HANDLE;
    VkSwapchainKHR            m_swapChain                = VK_NULL_HANDLE;

    Shared<VkGfxSwapChain>    m_oldSwapChain             = nullptr;

    VkSurfaceCapabilitiesKHR  m_capabilities             = {};

    uint32_t                  m_imagesCount              = 0u;
    DynamicArray<VkImage>     m_swapChainImages          = {};
    DynamicArray<VkImageView> m_swapChainImageViews      = {};
    VkFormat                  m_imageFormat              = {};

    VkExtent2D                m_currentExtent            = {};

    DynamicArray<VkSemaphore> m_imageAvailableSemaphores = {};
    DynamicArray<VkSemaphore> m_renderFinishedSemaphores = {};
    DynamicArray<VkSemaphore> m_submitSemaphores         = {};

    DynamicArray<VkFence>     m_inFlightFences           = {};

    uint32_t                  m_currentFrame             = 0u;

    VkQueue                   m_graphicsQueue            = VK_NULL_HANDLE;
    VkQueue                   m_presentQueue             = VK_NULL_HANDLE;
    VkQueue                   m_computeQueue             = VK_NULL_HANDLE;
    VkQueue                   m_transferQueue            = VK_NULL_HANDLE;
};
} // namespace dusk