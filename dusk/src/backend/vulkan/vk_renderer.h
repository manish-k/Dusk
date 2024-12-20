#pragma once

#include "dusk.h"
#include "vk_device.h"
#include "vk_types.h"
#include "vk_swapchain.h"
#include "renderer/renderer.h"
#include "platform/glfw_vulkan_window.h"

namespace dusk
{
class VulkanRenderer final : public Renderer
{
public:
    VulkanRenderer(Shared<GLFWVulkanWindow> window);
    ~VulkanRenderer() override;

    bool                  init(const char* appName, uint32_t version) override;

    VkCommandBuffer       getCurrentCommandBuffer() const;
    VkCommandBuffer       beginFrame();
    Error                 endFrame();
    void                  beginSwapChainRenderPass(VkCommandBuffer commandBuffer, VkRenderPass renderPass);
    void                  endSwapChainRenderPass(VkCommandBuffer commandBuffer);

    VkGfxSwapChain&       getSwapChain() { return *m_swapChain; }

    static VulkanContext& getVulkanContext() { return s_context; }

private:
    Error recreateSwapChain();

    Error createCommandBuffers();
    void  freeCommandBuffers();
    Error recreateCommandBuffers();

private:
    Unique<VkGfxDevice>           m_gfxDevice = nullptr;
    Unique<VkGfxSwapChain>        m_swapChain = nullptr;

    Shared<GLFWVulkanWindow>      m_window    = nullptr;

    VkSurfaceKHR                  m_surface   = VK_NULL_HANDLE;

    DynamicArray<VkCommandBuffer> m_commandBuffers;

    bool                          m_isFrameStarted    = false;
    uint32_t                      m_currentImageIndex = 0u;
    uint32_t                      m_currentFrameIndex = 0u;

private:
    static VulkanContext s_context;
};
} // namespace dusk