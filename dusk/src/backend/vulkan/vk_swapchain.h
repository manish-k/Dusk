#pragma once

#include "dusk.h"
#include "vk.h"
#include "vk_types.h"
#include "vk_renderpass.h"
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

    void           resize();

    Error          initFrameBuffers();

    VulkanResult   acquireNextImage(uint32_t* imageIndex);
    VulkanResult   submitCommandBuffers(const VkCommandBuffer* buffers, uint32_t* imageIndex);

    VkSwapchainKHR getSwapChain() const { return m_swapChain; }
    uint32_t       getImagesCount() const { return m_imagesCount; }
    VkExtent2D     getCurrentExtent() const { return m_currentExtent; }
    VkFramebuffer  getFrameBuffer(uint32_t imageIndex) { return m_frameBuffers[imageIndex]; }
    VkImageView    getImageView(uint32_t imageIndex) { return m_swapChainImageViews[imageIndex]; }
    VkRenderPass   getRenderPass() const { return m_renderPass->get(); }
    VkFormat       getImageFormat() const { return m_imageFormat; }
    VkImage        getImage(uint32_t imageIndex) const { return m_swapChainImages[imageIndex]; }
    VkImage        getDepthImage(uint32_t imageIndex)
    {
        return m_depthImages[imageIndex].vkImage;
    };
    VkImageView getDepthImageView(uint32_t imageIndex) { return m_depthImageViews[imageIndex]; };
    GfxTexture  getCurrentSwapImageTexture();

private:
    Error              createSwapChain(const VkGfxSwapChainParams& params);
    void               destroySwapChain();

    Error              createImageViews(VkFormat format);
    void               destroyImageViews();

    Error              createSyncObjects();

    Error              createDepthResources();
    void               destroyDepthResources();

    VkSurfaceFormatKHR getBestSurfaceFormat() const;
    VkPresentModeKHR   getPresentMode() const;
    VkExtent2D         getSwapExtent(uint32_t width, uint32_t height) const;
    VkFormat           findDepthFormat();

private:
    VkPhysicalDevice             m_physicalDevice = VK_NULL_HANDLE;
    VkDevice                     m_device         = VK_NULL_HANDLE;
    VkSurfaceKHR                 m_surface        = VK_NULL_HANDLE;
    VkSwapchainKHR               m_swapChain      = VK_NULL_HANDLE;

    Shared<VkGfxSwapChain>       m_oldSwapChain   = nullptr;
    Unique<VkGfxRenderPass>      m_renderPass     = nullptr;

    VkSurfaceCapabilitiesKHR     m_capabilities {};

    VulkanGPUAllocator*          m_gpuAllocator;

    uint32_t                     m_imagesCount = 0u;
    DynamicArray<VkImage>        m_swapChainImages {};
    DynamicArray<VkImageView>    m_swapChainImageViews {};
    DynamicArray<VkFramebuffer>  m_frameBuffers {};
    VkFormat                     m_imageFormat {};

    DynamicArray<VulkanGfxImage> m_depthImages {};
    DynamicArray<VkImageView>    m_depthImageViews {};
    VkFormat                     m_depthImageFormat {};

    VkExtent2D                   m_currentExtent {};

    DynamicArray<VkSemaphore>    m_imageAvailableSemaphores {};
    DynamicArray<VkSemaphore>    m_renderFinishedSemaphores {};

    DynamicArray<VkFence>        m_inFlightFences {};
    DynamicArray<VkFence>        m_imagesInFlight {};

    uint32_t                     m_currentFrame  = 0u;

    VkQueue                      m_graphicsQueue = VK_NULL_HANDLE;
    VkQueue                      m_presentQueue  = VK_NULL_HANDLE;
    VkQueue                      m_computeQueue  = VK_NULL_HANDLE;
    VkQueue                      m_transferQueue = VK_NULL_HANDLE;
};
} // namespace dusk