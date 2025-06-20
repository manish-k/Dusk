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

    bool                           init() override;
    void                           cleanup() override;

    VkCommandBuffer                getCurrentCommandBuffer() const;
    VkCommandBuffer                beginFrame();
    Error                          endFrame();
    void                           deviceWaitIdle();

    void                           beginSwapChainRenderPass(VkCommandBuffer commandBuffer);
    void                           endSwapChainRenderPass(VkCommandBuffer commandBuffer);

    void                           beginRendering(VkCommandBuffer commandBuffer);
    void                           endRendering(VkCommandBuffer commandBuffer);

    VkGfxSwapChain&                getSwapChain() const { return *m_swapChain; }

    uint32_t                       getCurrentFrameIndex() const { return m_currentFrameIndex; }
    float                          getAspectRatio() const;
    uint32_t                       getMaxFramesCount() const { return m_swapChain->getImagesCount(); }

    DynamicArray<VkCommandBuffer>& getSecondayCmdBuffers(uint32_t frameIndex) { return m_secondaryCmdBuffers[frameIndex]; }

private:
    Error recreateSwapChain();

    Error createCommandBuffers();
    void  freeCommandBuffers();
    Error recreateCommandBuffers();

    Error createSecondaryCmdPoolsAndBuffers();
    void  resetSecondaryCmdBuffers(uint32_t frameIdx);
    void  freeSecondaryCmdPoolsAndBuffers();

private:
    Unique<VkGfxSwapChain>        m_swapChain = nullptr;
    GLFWVulkanWindow&             m_window;

    DynamicArray<VkCommandBuffer> m_commandBuffers;
    DynamicArray<VkCommandPool>   m_secondaryCmdPools {};
    // TODO: currently 1 buff per frame per secondary pool
    DynamicArray<DynamicArray<VkCommandBuffer>> m_secondaryCmdBuffers {};

    bool                                        m_isFrameStarted    = false;
    uint32_t                                    m_currentImageIndex = 0u;
    uint32_t                                    m_currentFrameIndex = 0u;
};
} // namespace dusk