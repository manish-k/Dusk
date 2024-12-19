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

    bool init(const char* appName, uint32_t version) override;

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

private:
    static VulkanContext s_context;
};
} // namespace dusk