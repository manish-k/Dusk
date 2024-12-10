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

    result = m_gfxDevice->createInstance(appName, version, extensions);

    if (result != VK_SUCCESS)
    {
        DUSK_ERROR("Unable to create vk instance");
        return false;
    }
    DUSK_INFO("VkInstance created successfully");

    // create vulkan instance
    // volk load instance
    // setup debug logger
    // create surface
    // pick physical device
    // create logical device
    // create command pool

    return true;
}
} // namespace dusk