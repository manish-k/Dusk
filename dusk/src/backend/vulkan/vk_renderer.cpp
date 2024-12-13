#pragma once

#include "vk_renderer.h"

#define VOLK_IMPLEMENTATION
#include <volk.h>

namespace dusk
{
VulkanContext VulkanRenderer::s_context = VulkanContext {};

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
    auto& context = VulkanRenderer::s_context;

    // Initialize instance function pointers
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

    Error err = m_gfxDevice->createInstance(appName, version, extensions, context);

    if (err != Error::Ok)
    {
        return false;
    }

    err = m_window->createWindowSurface(context.vulkanInstance, &m_surface);
    if (err != Error::Ok)
    {
        return false;
    }
    context.surface = m_surface;

    // pick physical device and create logical device
    err = m_gfxDevice->createDevice(context);
    if (err != Error::Ok)
    {
        return false;
    }

    err = recreateSwapChain();
    if (err != Error::Ok)
    {
        return false;
    }

    return true;
}

Error VulkanRenderer::recreateSwapChain()
{
    auto&                context      = VulkanRenderer::s_context;
    VkExtent2D           windowExtent = m_window->getFramebufferExtent();

    VkGfxSwapChainParams params {};
    params.windowWidth  = windowExtent.width;
    params.windowHeight = windowExtent.height;
    params.oldSwapChain = VK_NULL_HANDLE;

    m_swapChain         = createUnique<VkGfxSwapChain>();
    return m_swapChain->create(context, params);
}

} // namespace dusk