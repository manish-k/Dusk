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

VkCommandBuffer VulkanRenderer::getCurrentCommandBuffer() const
{
    DASSERT(m_isFrameStarted, "Cannot get command buffer when frame not in progress");
    return m_commandBuffers[m_currentFrameIndex];
}

VkCommandBuffer VulkanRenderer::beginFrame()
{
    DASSERT(!m_isFrameStarted, "Can't call begin frame when already in progress");

    VulkanResult result = m_swapChain->acquireNextImage(&m_currentImageIndex);

    if (result.vkResult == VK_ERROR_OUT_OF_DATE_KHR)
    {
        recreateSwapChain();
        recreateCommandBuffers();
        return nullptr;
    }

    if (result.hasError() && result.vkResult != VK_SUBOPTIMAL_KHR)
    {
        DUSK_ERROR("beginFrame Failed to acquire swap chain image! {}", result.toString());
    }

    m_isFrameStarted              = true;
    VkCommandBuffer commandBuffer = getCurrentCommandBuffer();

    // start recording command buffer
    VkCommandBufferBeginInfo beginInfo {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    result          = vkBeginCommandBuffer(commandBuffer, &beginInfo);

    if (result.hasError())
    {
        DUSK_ERROR("beginFrame Failed to begin recording command buffer! {}", result.toString());
    }

    return commandBuffer;
}

Error VulkanRenderer::endFrame()
{
    DASSERT(m_isFrameStarted, "Can't call endFrame while frame is not in progress");
    auto         commandBuffer = getCurrentCommandBuffer();

    VulkanResult result        = vkEndCommandBuffer(commandBuffer);
    if (result.hasError())
    {
        DUSK_ERROR("endFrame failed to record command buffer! {}", result.toString());
        return result.getErrorId();
    }

    result = m_swapChain->submitCommandBuffers(&commandBuffer, &m_currentImageIndex);

    if (result.vkResult == VK_ERROR_OUT_OF_DATE_KHR || result.vkResult == VK_SUBOPTIMAL_KHR || m_window->isResized())
    {
        m_window->resetResizedState();
        recreateSwapChain();
        recreateCommandBuffers();
    }
    else if (result.hasError())
    {
        DUSK_ERROR("endFrame failed to present swap chain image! {}", result.toString());
        return result.getErrorId();
    }

    m_isFrameStarted    = false;
    m_currentFrameIndex = (m_currentFrameIndex + 1) % m_swapChain->getImagesCount();
}

void VulkanRenderer::beginSwapChainRenderPass(VkCommandBuffer commandBuffer, VkRenderPass renderPass)
{
    DASSERT(m_isFrameStarted, "Can't begin render pass while frame is not in progress");
    DASSERT(commandBuffer == getCurrentCommandBuffer(), "Can't begin render pass on command buffer from a different frame");

    VkExtent2D            currentExtent = m_swapChain->getCurrentExtent();

    VkRenderPassBeginInfo renderPassInfo {};
    renderPassInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass        = renderPass;
    renderPassInfo.framebuffer       = m_swapChain->getFrameBuffer(m_currentImageIndex);

    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = currentExtent;

    std::array<VkClearValue, 2> clearValues {};
    clearValues[0].color           = { 0.1f, 0.1f, 0.1f, 1.0f };
    clearValues[1].depthStencil    = { 1.0f, 0 };
    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues    = clearValues.data();

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport {};
    viewport.x        = 0.0f;
    viewport.y        = 0.0f;
    viewport.width    = static_cast<float>(currentExtent.width);
    viewport.height   = static_cast<float>(currentExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor {};
    scissor.offset = { 0, 0 };
    scissor.extent = currentExtent;
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}

void VulkanRenderer::endSwapChainRenderPass(VkCommandBuffer commandBuffer)
{
    DASSERT(m_isFrameStarted, "Can't call end swap chain while frame is not in progress");
    DASSERT(commandBuffer == getCurrentCommandBuffer(), "Can't end render pass on command buffer from a different frame");

    vkCmdEndRenderPass(commandBuffer);
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