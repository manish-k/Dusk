#pragma once

#include "dusk.h"
#include "vk_types.h"
#include "vk_swapchain.h"
#include "vk_cmdbuffer_pool.h"
#include "renderer/renderer.h"
#include "platform/glfw_vulkan_window.h"
#include "renderer/texture.h"

namespace dusk
{

struct CommandBufferPools
{
    VulkanCmdBufferPool* graphicsPool;
    VulkanCmdBufferPool* computePool;
};

class VulkanRenderer final : public Renderer
{
public:
    VulkanRenderer(GLFWVulkanWindow& window);
    ~VulkanRenderer() override;

    bool                           init() override;
    void                           cleanup() override;

    VkCommandBuffer                getCurrentCommandBuffer() const;

    CommandBufferPools             beginFrame();
    Error                          endFrame(DynamicArray<VulkanSubmitBatch>& batches);

    void                           deviceWaitIdle();

    VkGfxSwapChain&                getSwapChain() const { return *m_swapChain; }

    uint32_t                       getCurrentFrameIndex() const { return m_currentFrameIndex; }
    float                          getAspectRatio() const;

    DynamicArray<VkCommandBuffer>& getSecondayCmdBuffers(uint32_t frameIndex) { return m_secondaryCmdBuffers[frameIndex]; }

    GfxTexture                     getCurrentSwapImageTexture();

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

    DynamicArray<VulkanCmdBufferPool>           m_graphicCommandBufferPools = {};
    DynamicArray<VulkanCmdBufferPool>           m_computeCommandBufferPools = {};

    bool                                        m_isFrameStarted            = false;
    uint32_t                                    m_currentImageIndex         = 0u;
    uint32_t                                    m_currentFrameIndex         = 0u;
};
} // namespace dusk