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
    m_swapChain = createUnique<VkGfxSwapChain>();
}

VulkanRenderer::~VulkanRenderer()
{
    auto& context = VulkanRenderer::s_context;

    freeCommandBuffers();

    m_swapChain->destroy();

    vkDestroySurfaceKHR(context.vulkanInstance, m_surface, nullptr);

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

    err = createCommandBuffers();
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

    return m_swapChain->create(context, params);
}

Error VulkanRenderer::createCommandBuffers()
{
    auto& context = VulkanRenderer::s_context;

    m_commandBuffers.resize(m_swapChain->getImagesCount());

    VkCommandBufferAllocateInfo allocInfo {};
    allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool        = context.commandPool;
    allocInfo.commandBufferCount = static_cast<uint32_t>(m_commandBuffers.size());

    // allocate command buffers for each swapchain image
    VulkanResult result = vkAllocateCommandBuffers(context.device, &allocInfo, m_commandBuffers.data());

    if (result.hasError())
    {
        DUSK_ERROR("Unable to allocate command buffers {}", result.toString());
        return result.getErrorId();
    }

    DUSK_INFO("Command buffers created = {}", m_commandBuffers.size());

    return Error::Ok;
}

void VulkanRenderer::freeCommandBuffers()
{
    auto& context = VulkanRenderer::s_context;
    vkFreeCommandBuffers(context.device, context.commandPool, static_cast<uint32_t>(m_commandBuffers.size()), m_commandBuffers.data());
    m_commandBuffers.clear();
}

Error VulkanRenderer::recreateCommandBuffers()
{
    DUSK_INFO("Recreating command buffers");
    
    freeCommandBuffers();

    return createCommandBuffers();
}

} // namespace dusk