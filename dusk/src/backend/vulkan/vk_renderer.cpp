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
    vkDestroySurfaceKHR(m_gfxDevice->getVkInstance(), m_surface, nullptr);

    m_gfxDevice->destroyDevice();
    m_gfxDevice->destroyInstance();
}

bool VulkanRenderer::init(const char* appName, uint32_t version)
{
    VulkanResult result = volkInitialize();
    if (result.hasError())
    {
        DUSK_ERROR("Volk initialization failed. Vulkan loader might not be present {}", result.toString());
        return false;
    }

    DynamicArray<const char*> extensions = m_window->getRequiredWindowExtensions();
    DUSK_INFO("Required {} vulkan extensions for GLFW", extensions.size());
    for (int i = 0; i < extensions.size(); ++i)
    {
        DUSK_INFO(" - {}", extensions[i]);
    }

    Error err = m_gfxDevice->createInstance(appName, version, extensions);

    if (err != Error::Ok)
    {
        return false;
    }

    err = m_window->createWindowSurface(m_gfxDevice->getVkInstance(), &m_surface);
    if (err != Error::Ok)
    {
        
        return false;
    }

    err = m_gfxDevice->createDevice(m_surface);
    if (err != Error::Ok)
    {
        return false;
    }

    return true;
}

} // namespace dusk