#pragma once

#include "stats_recorder.h"

#include "vk_device.h"
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

CommandBufferPools VulkanRenderer::beginFrame()
{
    DUSK_PROFILE_FUNCTION;

    DASSERT(!m_isFrameStarted, "Can't call begin frame when already in progress");

    VulkanResult result = m_swapChain->acquireNextImage(m_currentFrameIndex, &m_currentImageIndex);

    // reset pools before use. Acquiring image is guarded by a fence so it is a good place to reset pools.
    vulkan::resetCmdBufferPool(&m_graphicCommandBufferPools[m_currentFrameIndex]);
    vulkan::resetCmdBufferPool(&m_computeCommandBufferPools[m_currentFrameIndex]);

    if (result.vkResult == VK_ERROR_OUT_OF_DATE_KHR)
    {
        recreateSwapChain();
        recreateCommandBuffers();
        return { nullptr, nullptr };
    }

    if (result.hasError() && result.vkResult != VK_SUBOPTIMAL_KHR)
    {
        DUSK_ERROR("beginFrame Failed to acquire swap chain image! {}", result.toString());
        return { nullptr, nullptr };
    }

    // collect stats from N - MAX_FRAMES_IN_FLIGHT frames ago, as those should have finished GPU execution by now
    StatsRecorder::get()->retrieveQueryStats();

    m_isFrameStarted = true;

    return { &m_graphicCommandBufferPools[m_currentFrameIndex], &m_computeCommandBufferPools[m_currentFrameIndex] };
}

Error VulkanRenderer::endFrame(DynamicArray<VulkanSubmitBatch>& batches)
{
    DUSK_PROFILE_FUNCTION;

    DASSERT(m_isFrameStarted, "Can't call endFrame while frame is not in progress");

    VulkanResult result = m_swapChain->submitCommandBuffers(batches, m_currentFrameIndex, m_currentImageIndex);

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
    m_currentFrameIndex = (m_currentFrameIndex + 1) % MAX_FRAMES_IN_FLIGHT;

    return Error::Ok;
}

void VulkanRenderer::deviceWaitIdle()
{
    auto& context = VkGfxDevice::getSharedVulkanContext();
    vkDeviceWaitIdle(context.device);
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

    m_graphicCommandBufferPools.resize(MAX_FRAMES_IN_FLIGHT);
    m_computeCommandBufferPools.resize(MAX_FRAMES_IN_FLIGHT);

    for (uint32_t i = 0u; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        vulkan::createCmdBufferPool(
            context.device,
            context.graphicsQueueFamilyIndex,
            8u, // starting with 8 command buffers
            &m_graphicCommandBufferPools[i]);

        vulkan::createCmdBufferPool(
            context.device,
            context.computeQueueFamilyIndex,
            8u, // starting with 8 command buffers
            &m_computeCommandBufferPools[i]);
    }

    Error err = createSecondaryCmdPoolsAndBuffers();
    if (err != Error::Ok) return err;

    return Error::Ok;
}

void VulkanRenderer::freeCommandBuffers()
{
    auto& context = VkGfxDevice::getSharedVulkanContext();

    for (uint32_t i = 0u; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        vulkan::destroyCmdBufferPool(&m_graphicCommandBufferPools[i]);
        vulkan::destroyCmdBufferPool(&m_computeCommandBufferPools[i]);
    }
    m_graphicCommandBufferPools.clear();
    m_computeCommandBufferPools.clear();

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

    m_secondaryCmdPools.resize(poolsCount);
    m_secondaryCmdBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    for (uint32_t i = 0u; i < MAX_FRAMES_IN_FLIGHT; ++i)
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
        for (uint32_t frameIdx = 0u; frameIdx < MAX_FRAMES_IN_FLIGHT; ++frameIdx)
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
    auto& context = VkGfxDevice::getSharedVulkanContext();

    for (uint32_t frameIdx = 0u; frameIdx < MAX_FRAMES_IN_FLIGHT; ++frameIdx)
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

GfxTexture VulkanRenderer::getCurrentSwapImageTexture()
{
    GfxTexture tex(10000 + m_currentImageIndex); // some random id
    tex.image.vkImage = m_swapChain->getImage(m_currentImageIndex);
    tex.imageView     = m_swapChain->getImageView(m_currentImageIndex);
    tex.usage         = ColorTexture | TransferDstTexture;
    tex.format        = m_swapChain->getImageFormat();
    tex.numLayers     = 1;
    tex.numMipLevels  = 1;

    return tex;
}

} // namespace dusk