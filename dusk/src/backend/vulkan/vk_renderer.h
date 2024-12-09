#pragma once

#include "dusk.h"
#include "vk_device.h"
#include "renderer/renderer.h"
#include "platform/glfw_vulkan_window.h"

#define VOLK_IMPLEMENTATION
#include <volk.h>

namespace dusk
{
class VulkanRenderer final : public Renderer
{
public:
    VulkanRenderer(GLFWVulkanWindow& window);
    ~VulkanRenderer() override;

    bool init(const char* appName, uint32_t version) override;

private:
    Unique<VkGfxDevice> m_gfxDevice = nullptr;

    GLFWVulkanWindow& m_window;
};
} // namespace dusk