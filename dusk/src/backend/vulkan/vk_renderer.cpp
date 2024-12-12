#pragma once

#include "vk_renderer.h"

#define VOLK_IMPLEMENTATION
#include <volk.h>

namespace dusk
{
VulkanRenderer::VulkanRenderer(Shared<GLFWVulkanWindow> window) :
    m_window(window)
{
    m_gfxDevice = createUnique<VkGfxDevice>();
}

VulkanRenderer::~VulkanRenderer()
{
    destroySurface();

    m_gfxDevice->destroyDevice();
    m_gfxDevice->destroyInstance();
}

bool VulkanRenderer::init(const char* appName, uint32_t version)
{
    VkResult result = volkInitialize();
    if (result != VK_SUCCESS)
    {
        DUSK_ERROR("Volk initialization failed. Vulkan loader might not be present");
        return false;
    }

    DynamicArray<const char*> extensions = m_window->getRequiredWindowExtensions();
    DUSK_INFO("Required {} vulkan extensions for GLFW", extensions.size());
    for (int i = 0; i < extensions.size(); ++i)
    {
        DUSK_INFO(" - {}", extensions[i]);
    }

    result = m_gfxDevice->createInstance(appName, version, extensions);

    if (result != VK_SUCCESS)
    {
        DUSK_ERROR("Unable to create vk instance");
        return false;
    }
    DUSK_INFO("VkInstance created successfully");

    result = createSurface();
    if (result != VK_SUCCESS)
    {
        DUSK_ERROR("Unable to create window surface");
        return false;
    }
    DUSK_INFO("Window surface created successfully");

    m_gfxDevice->createDevice(m_surface);

    // create command pool

    return true;
}
VkResult VulkanRenderer::createSurface()
{
    return m_window->createWindowSurface(m_gfxDevice->getVkInstance(), &m_surface);
}
void VulkanRenderer::destroySurface()
{
    vkDestroySurfaceKHR(m_gfxDevice->getVkInstance(), m_surface, nullptr);
}
} // namespace dusk