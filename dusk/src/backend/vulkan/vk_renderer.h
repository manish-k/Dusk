#pragma once

#include "dusk.h"
#include "vk_device.h"
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

    VkResult createSurface();
    void destroySurface();

private:
    Unique<VkGfxDevice>      m_gfxDevice = nullptr;

    Shared<GLFWVulkanWindow> m_window;

    VkSurfaceKHR             m_surface;
};
} // namespace dusk