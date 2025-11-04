#pragma once

#include "vk_renderer.h"
#include "debug/profiler.h"

#define VOLK_IMPLEMENTATION
#include <volk.h>
#include <thread>

namespace dusk
{
VulkanRenderer::VulkanRenderer(GLFWVulkanWindow& window) :
    m_window(window)
{
}

VulkanRenderer::~VulkanRenderer()
{
}

bool VulkanRenderer::init()
{
    DUSK_INFO("initializing vulkan renderer");

    Error err = recreateSwapChain();
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

void VulkanRenderer::cleanup()
{
    DUSK_INFO("cleaning up vulkan renderer");

    freeCommandBuffers();

    m_swapChain->destroy();
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
        return VK_NULL_HANDLE;
    }

    if (result.hasError() && result.vkResult != VK_SUBOPTIMAL_KHR)
    {
        DUSK_ERROR("beginFrame Failed to acquire swap chain image! {}", result.toString());
        return VK_NULL_HANDLE;
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

    // profiler stuff
#ifdef DUSK_ENABLE_PROFILING
    TracyVkCollect(VkGfxDevice::getProfilerContext(), commandBuffer);
#endif

    return commandBuffer;
}

Error VulkanRenderer::endFrame()
{
    DASSERT(m_isFrameStarted, "Can't call endFrame while frame is not in progress");
    VkCommandBuffer commandBuffer = getCurrentCommandBuffer();

    // dynamic rendering is being used so transitioning image layout for presentation.
    VkImageMemoryBarrier imageBarrier { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
    imageBarrier.srcAccessMask    = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    imageBarrier.oldLayout        = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    imageBarrier.newLayout        = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    imageBarrier.image            = m_swapChain->getImage(m_currentImageIndex);
    imageBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

    vkCmdPipelineBarrier(
        commandBuffer,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, // srcStageMask
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,          // dstStageMask
        0,
        0,
        nullptr,
        0,
        nullptr,
        1,
        &imageBarrier);

    // finish recording command buffer
    VulkanResult result = vkEndCommandBuffer(commandBuffer);
    if (result.hasError())
    {
        DUSK_ERROR("endFrame failed to record command buffer! {}", result.toString());
        return result.getErrorId();
    }

    result = m_swapChain->submitCommandBuffers(&commandBuffer, &m_currentImageIndex);

    if (result.vkResult == VK_ERROR_OUT_OF_DATE_KHR || result.vkResult == VK_SUBOPTIMAL_KHR || m_window.isResized())
    {
        DUSK_INFO("recreating swap chain and frame buffers");
        m_window.resetResizedState();
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

    return Error::Ok;
}

void VulkanRenderer::deviceWaitIdle()
{
    auto& context = VkGfxDevice::getSharedVulkanContext();
    vkDeviceWaitIdle(context.device);
}

void VulkanRenderer::beginSwapChainRenderPass(VkCommandBuffer commandBuffer)
{
    DASSERT(m_isFrameStarted, "Can't begin render pass while frame is not in progress");
    DASSERT(commandBuffer == getCurrentCommandBuffer(), "Can't begin render pass on command buffer from a different frame");

    VkExtent2D            currentExtent = m_swapChain->getCurrentExtent();

    VkRenderPassBeginInfo renderPassInfo {};
    renderPassInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass        = m_swapChain->getRenderPass();
    renderPassInfo.framebuffer       = m_swapChain->getFrameBuffer(m_currentImageIndex);

    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = currentExtent;

    Array<VkClearValue, 2> clearValues {};
    clearValues[0].color           = { 0.f, 0.f, 0.f, 1.0f };
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

void VulkanRenderer::beginRendering(VkCommandBuffer commandBuffer)
{
    DASSERT(m_isFrameStarted, "Can't begin rendering while frame is not in progress");

    VkExtent2D currentExtent = m_swapChain->getCurrentExtent();

    // add image barrier to transition color attachment to optimal layout
    VkImageMemoryBarrier imageBarrier { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
    imageBarrier.dstAccessMask    = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    imageBarrier.oldLayout        = VK_IMAGE_LAYOUT_UNDEFINED;
    imageBarrier.newLayout        = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    imageBarrier.image            = m_swapChain->getImage(m_currentImageIndex);
    imageBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

    vkCmdPipelineBarrier(
        commandBuffer,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,             // srcStageMask
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, // dstStageMask
        0,
        0,
        nullptr,
        0,
        nullptr,
        1,
        &imageBarrier);

    // add image barrier to transition depth attachment to optimal layout
    VkImageMemoryBarrier depthBarrier { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
    depthBarrier.dstAccessMask    = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    depthBarrier.oldLayout        = VK_IMAGE_LAYOUT_UNDEFINED;
    depthBarrier.newLayout        = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    depthBarrier.image            = m_swapChain->getDepthImage(m_currentImageIndex);
    depthBarrier.subresourceRange = { VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT, 0, 1, 0, 1 };

    vkCmdPipelineBarrier(
        commandBuffer,
        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, // srcStageMask
        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, // dstStageMask
        0,
        0,
        nullptr,
        0,
        nullptr,
        1,
        &depthBarrier);

    // define color attachments info
    VkRenderingAttachmentInfo colorAttachmentInfo { VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
    colorAttachmentInfo.imageView   = m_swapChain->getImageView(m_currentImageIndex);
    colorAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;
    colorAttachmentInfo.loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachmentInfo.storeOp     = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachmentInfo.clearValue  = { 0.f, 0.f, 0.f, 1.0f };

    VkRenderingAttachmentInfoKHR depthStencilAttachment { VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR };
    depthStencilAttachment.imageView               = m_swapChain->getDepthImageView(m_currentImageIndex);
    depthStencilAttachment.imageLayout             = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    depthStencilAttachment.loadOp                  = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthStencilAttachment.storeOp                 = VK_ATTACHMENT_STORE_OP_STORE;
    depthStencilAttachment.clearValue.depthStencil = { 1.0f, 0 };

    VkRenderingInfo renderingInfo { VK_STRUCTURE_TYPE_RENDERING_INFO };
    renderingInfo.renderArea.offset    = { 0u, 0u };
    renderingInfo.renderArea.extent    = currentExtent;
    renderingInfo.layerCount           = 1u;
    renderingInfo.colorAttachmentCount = 1u;
    renderingInfo.pColorAttachments    = &colorAttachmentInfo;
    renderingInfo.pDepthAttachment     = &depthStencilAttachment;
    renderingInfo.pStencilAttachment   = &depthStencilAttachment;

    vkCmdBeginRendering(commandBuffer, &renderingInfo);

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

void VulkanRenderer::endRendering(VkCommandBuffer commandBuffer)
{
    DASSERT(m_isFrameStarted, "Can't end rendering while frame is not in progress");

    vkCmdEndRendering(commandBuffer);
}

float VulkanRenderer::getAspectRatio() const
{
    auto extent = m_swapChain->getCurrentExtent();
    return static_cast<float>(extent.width) / static_cast<float>(extent.height);
}

Error VulkanRenderer::recreateSwapChain()
{
    auto&      context      = VkGfxDevice::getSharedVulkanContext();
    VkExtent2D windowExtent = m_window.getFramebufferExtent();

    while (windowExtent.width == 0 || windowExtent.height == 0)
    {
        windowExtent = m_window.getFramebufferExtent();
        m_window.waitEvents();
    }

    vkDeviceWaitIdle(context.device);

    VkGfxSwapChainParams params {};
    params.windowWidth                  = windowExtent.width;
    params.windowHeight                 = windowExtent.height;

    Shared<VkGfxSwapChain> oldSwapChain = nullptr;
    if (m_swapChain != nullptr)
    {
        oldSwapChain = std::move(m_swapChain);
    }

    m_swapChain = createUnique<VkGfxSwapChain>();

    return m_swapChain->create(context, params, oldSwapChain);
}

Error VulkanRenderer::createCommandBuffers()
{
    auto& context = VkGfxDevice::getSharedVulkanContext();

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

    for (uint32_t i = 0; i < m_commandBuffers.size(); ++i)
    {
#ifdef VK_RENDERER_DEBUG
        vkdebug::setObjectName(
            context.device,
            VK_OBJECT_TYPE_COMMAND_BUFFER,
            (uint64_t)m_commandBuffers[i],
            "renderer_cmd_buff");
#endif // VK_RENDERER_DEBUG
    }

    DUSK_INFO("Command buffers created = {}", m_commandBuffers.size());

    Error err = createSecondaryCmdPoolsAndBuffers();
    if (err != Error::Ok) return err;

    return Error::Ok;
}

void VulkanRenderer::freeCommandBuffers()
{
    auto& context = VkGfxDevice::getSharedVulkanContext();
    vkFreeCommandBuffers(context.device, context.commandPool, static_cast<uint32_t>(m_commandBuffers.size()), m_commandBuffers.data());
    m_commandBuffers.clear();

    freeSecondaryCmdPoolsAndBuffers();
}

Error VulkanRenderer::recreateCommandBuffers()
{
    DUSK_INFO("Recreating command buffers");

    freeCommandBuffers();

    return createCommandBuffers();
}

Error VulkanRenderer::createSecondaryCmdPoolsAndBuffers()
{
    auto&          context    = VkGfxDevice::getSharedVulkanContext();
    const uint32_t poolsCount = std::thread::hardware_concurrency();
    const uint32_t maxFrames  = m_swapChain->getImagesCount();

    m_secondaryCmdPools.resize(poolsCount);
    m_secondaryCmdBuffers.resize(maxFrames);
    for (uint32_t i = 0u; i < maxFrames; ++i)
    {
        m_secondaryCmdBuffers[i].resize(poolsCount);
    }

    for (uint32_t i = 0u; i < poolsCount; ++i)
    {
        VkCommandPoolCreateInfo poolInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
        poolInfo.flags                   = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex        = context.graphicsQueueFamilyIndex;

        VulkanResult result              = vkCreateCommandPool(context.device, &poolInfo, nullptr, &m_secondaryCmdPools[i]);
        if (result.hasError())
        {
            DUSK_ERROR("Unable to create secondary command pools {}", result.toString());
            return Error::InitializationFailed;
        }

#ifdef VK_RENDERER_DEBUG
        vkdebug::setObjectName(
            context.device,
            VK_OBJECT_TYPE_COMMAND_POOL,
            (uint64_t)m_secondaryCmdPools[i],
            ("seconday_cmd_pool_" + std::to_string(i)).c_str());

#endif

        // allocate secondary cmd buffers
        for (uint32_t frameIdx = 0u; frameIdx < maxFrames; ++frameIdx)
        {
            VkCommandBufferAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
            allocInfo.commandPool                 = m_secondaryCmdPools[i];
            allocInfo.level                       = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
            allocInfo.commandBufferCount          = 1;

            result                                = vkAllocateCommandBuffers(context.device, &allocInfo, &m_secondaryCmdBuffers[frameIdx][i]);
            if (result.hasError())
            {
                DUSK_ERROR("Unable to create secondary command buffers {}", result.toString());
                return Error::InitializationFailed;
            }
#ifdef VK_RENDERER_DEBUG

            vkdebug::setObjectName(
                context.device,
                VK_OBJECT_TYPE_COMMAND_BUFFER,
                (uint64_t)m_secondaryCmdBuffers[frameIdx][i],
                ("seconday_cmd_buffer_" + std::to_string(frameIdx) + "_" + std::to_string(i)).c_str());
#endif
        }
    }

    return Error::Ok;
}

void VulkanRenderer::freeSecondaryCmdPoolsAndBuffers()
{
    auto&          context   = VkGfxDevice::getSharedVulkanContext();
    const uint32_t maxFrames = m_swapChain->getImagesCount();

    for (uint32_t frameIdx = 0u; frameIdx < maxFrames; ++frameIdx)
    {
        for (uint32_t cmdBuffIdx = 0u; cmdBuffIdx < m_secondaryCmdBuffers.size(); ++cmdBuffIdx)
        {
            vkFreeCommandBuffers(context.device, m_secondaryCmdPools[cmdBuffIdx], 1, &m_secondaryCmdBuffers[frameIdx][cmdBuffIdx]);
        }
    }

    for (uint32_t cmdPoolIdx = 0u; cmdPoolIdx < m_secondaryCmdPools.size(); ++cmdPoolIdx)
    {
        vkDestroyCommandPool(context.device, m_secondaryCmdPools[cmdPoolIdx], nullptr);
    }

    m_secondaryCmdPools.clear();
    m_secondaryCmdBuffers.clear();
}

void VulkanRenderer::resetSecondaryCmdBuffers(uint32_t frameIdx)
{
    auto& context = VkGfxDevice::getSharedVulkanContext();

    for (uint32_t cmdBuffIdx = 0u; cmdBuffIdx < m_secondaryCmdBuffers.size(); ++cmdBuffIdx)
    {
        vkFreeCommandBuffers(context.device, m_secondaryCmdPools[cmdBuffIdx], 1, &m_secondaryCmdBuffers[frameIdx][cmdBuffIdx]);
    }
}

} // namespace dusk