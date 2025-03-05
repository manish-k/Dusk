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
    VulkanRenderer(GLFWVulkanWindow& window);
    ~VulkanRenderer() override;

    bool            init() override;

    VkCommandBuffer getCurrentCommandBuffer() const;
    VkCommandBuffer beginFrame();
    Error           endFrame();
    void            deviceWaitIdle();

    void            beginSwapChainRenderPass(VkCommandBuffer commandBuffer);
    void            endSwapChainRenderPass(VkCommandBuffer commandBuffer);

    void            beginRendering(VkCommandBuffer commandBuffer);
    void            endRendering(VkCommandBuffer commandBuffer);

    VkGfxSwapChain& getSwapChain() const { return *m_swapChain; }

private:
    Error recreateSwapChain();

    Error createCommandBuffers();
    void  freeCommandBuffers();
    Error recreateCommandBuffers();

private:
    Unique<VkGfxSwapChain>        m_swapChain = nullptr;
    GLFWVulkanWindow&             m_window;

    DynamicArray<VkCommandBuffer> m_commandBuffers;

    bool                          m_isFrameStarted    = false;
    uint32_t                      m_currentImageIndex = 0u;
    uint32_t                      m_currentFrameIndex = 0u;
};
} // namespace dusk