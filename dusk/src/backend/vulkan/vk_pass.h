#pragma once

#include "dusk.h"
#include "vk.h"
#include "vk_types.h"
#include "vk_device.h"

namespace dusk
{

/**
 * @brief Encapsulation of a dynamic rednering pass. Not to be confused
 * with vkRenderPass.
 */
struct VkGfxRenderPassContext
{
    VkCommandBuffer                         cmdBuffer;
    VkExtent2D                              extent;
    DynamicArray<VulkanRenderTarget>        colorTargets;
    VulkanRenderTarget                      depthTarget;
    bool                                    useDepth = false;

    DynamicArray<VkRenderingAttachmentInfo> colorAttachmentInfos;
    VkRenderingAttachmentInfo               depthAttachmentInfo {};
    VkRenderingInfo                         renderingInfo {};

    DynamicArray<VulkanImageBarier>         preBarriers;

    uint32_t                                maxParallelism = 1u;
    DynamicArray<VkCommandPool>             secondaryCmdPools;
    DynamicArray<VkCommandBuffer>           secondaryCmdBuffers;

    /**
     * @brief
     * @param format
     * @param oldLayout
     * @param newLayout
     * @param mipLevelCount
     * @param layersCount
     */
    void insertTransitionBarrier(
        VulkanImageBarier barrier)
    {
        preBarriers.push_back(barrier);
    }

    /**
     * @brief begin rendering of the pass
     */
    void begin()
    {
        if (!preBarriers.empty())
        {
            for (const auto& barrierInfo : preBarriers)
            {
                VkImageMemoryBarrier barrier            = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
                barrier.oldLayout                       = barrierInfo.oldLayout;
                barrier.newLayout                       = barrierInfo.newLayout;
                barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
                barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
                barrier.image                           = barrierInfo.image;
                barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
                barrier.subresourceRange.baseMipLevel   = 0;
                barrier.subresourceRange.levelCount     = 1;
                barrier.subresourceRange.baseArrayLayer = 0;
                barrier.subresourceRange.layerCount     = 1;

                vkCmdPipelineBarrier(
                    cmdBuffer,
                    barrierInfo.srcStage,
                    barrierInfo.dstStage,
                    0,
                    0,
                    nullptr,
                    0,
                    nullptr,
                    1,
                    &barrier);
            }
        }

        DynamicArray<VkFormat> colorFormats;

        colorAttachmentInfos.clear();
        for (const auto& target : colorTargets)
        {
            VkRenderingAttachmentInfo colorAttachment { VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
            colorAttachment.imageView   = target.imageView;
            colorAttachment.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
            colorAttachment.loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR;
            colorAttachment.storeOp     = VK_ATTACHMENT_STORE_OP_STORE;
            colorAttachment.clearValue  = target.clearValue;
            colorAttachmentInfos.push_back(colorAttachment);

            colorFormats.push_back(target.format);
        }

        if (useDepth)
        {
            depthAttachmentInfo             = { VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
            depthAttachmentInfo.imageView   = depthTarget.imageView;
            depthAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
            depthAttachmentInfo.loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR;
            depthAttachmentInfo.storeOp     = VK_ATTACHMENT_STORE_OP_STORE;
            depthAttachmentInfo.clearValue  = { 1.0f, 0 };

            VkImageMemoryBarrier depthBarrier { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
            depthBarrier.dstAccessMask    = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            depthBarrier.oldLayout        = VK_IMAGE_LAYOUT_UNDEFINED;
            depthBarrier.newLayout        = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            depthBarrier.image            = depthTarget.image.image;
            depthBarrier.subresourceRange = { VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT, 0, 1, 0, 1 };

            vkCmdPipelineBarrier(
                cmdBuffer,
                VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, // srcStageMask
                VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, // dstStageMask
                0,
                0,
                nullptr,
                0,
                nullptr,
                1,
                &depthBarrier);
        }

        renderingInfo                      = { VK_STRUCTURE_TYPE_RENDERING_INFO };
        renderingInfo.renderArea           = { { 0, 0 }, extent };
        renderingInfo.layerCount           = 1;
        renderingInfo.colorAttachmentCount = static_cast<uint32_t>(colorAttachmentInfos.size());
        renderingInfo.pColorAttachments    = colorAttachmentInfos.data();
        renderingInfo.pDepthAttachment     = useDepth ? &depthAttachmentInfo : nullptr;
        renderingInfo.flags                = VK_RENDERING_CONTENTS_SECONDARY_COMMAND_BUFFERS_BIT;

        vkCmdBeginRendering(cmdBuffer, &renderingInfo);

        VkViewport viewport {};
        viewport.x        = 0.0f;
        viewport.y        = 0.0f;
        viewport.width    = static_cast<float>(extent.width);
        viewport.height   = static_cast<float>(extent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);

        VkRect2D scissor {};
        scissor.offset = { 0, 0 };
        scissor.extent = extent;
        vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

        if (maxParallelism > 1u)
        {
            auto& ctx = VkGfxDevice::getSharedVulkanContext();

            // allocate pools per thread
            secondaryCmdPools.resize(maxParallelism);
            secondaryCmdBuffers.resize(maxParallelism);

            VkCommandBufferInheritanceRenderingInfo renderingInheritanceInfo { VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_RENDERING_INFO };
            renderingInheritanceInfo.colorAttachmentCount    = static_cast<uint32_t>(colorFormats.size());
            renderingInheritanceInfo.pColorAttachmentFormats = colorFormats.data();
            renderingInheritanceInfo.rasterizationSamples    = VK_SAMPLE_COUNT_1_BIT;

            if (useDepth)
            {
                renderingInheritanceInfo.depthAttachmentFormat = depthTarget.format;
                // renderingInheritanceInfo.stencilAttachmentFormat = depthTarget.format;
            }

            VkCommandBufferInheritanceInfo inheritance { VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO };
            inheritance.pNext = &renderingInheritanceInfo;

            for (uint32_t i = 0u; i < maxParallelism; ++i)
            {
                VkCommandPoolCreateInfo poolInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
                poolInfo.flags                   = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
                poolInfo.queueFamilyIndex        = ctx.graphicsQueueFamilyIndex;

                VulkanResult result              = vkCreateCommandPool(ctx.device, &poolInfo, nullptr, &secondaryCmdPools[i]);

                DASSERT(result.isOk());

                // allocate secondary cmd buffers
                VkCommandBufferAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
                allocInfo.commandPool                 = secondaryCmdPools[i];
                allocInfo.level                       = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
                allocInfo.commandBufferCount          = 1;

                result                                = vkAllocateCommandBuffers(ctx.device, &allocInfo, &secondaryCmdBuffers[i]);

                DASSERT(result.isOk());

#ifdef VK_RENDERER_DEBUG
                vkdebug::setObjectName(
                    ctx.device,
                    VK_OBJECT_TYPE_COMMAND_BUFFER,
                    (uint64_t)secondaryCmdBuffers[i],
                    ("seconday_cmd_buffer_" + std::to_string(i)).c_str());
#endif

                VkCommandBufferBeginInfo beginInfo { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
                beginInfo.flags            = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT | VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
                beginInfo.pInheritanceInfo = &inheritance;

                vkBeginCommandBuffer(secondaryCmdBuffers[i], &beginInfo);

                VkViewport viewport {};
                viewport.x        = 0.0f;
                viewport.y        = 0.0f;
                viewport.width    = static_cast<float>(extent.width);
                viewport.height   = static_cast<float>(extent.height);
                viewport.minDepth = 0.0f;
                viewport.maxDepth = 1.0f;
                vkCmdSetViewport(secondaryCmdBuffers[i], 0, 1, &viewport);

                VkRect2D scissor {};
                scissor.offset = { 0, 0 };
                scissor.extent = extent;
                vkCmdSetScissor(secondaryCmdBuffers[i], 0, 1, &scissor);
            }
        }
    }

    /**
     * @brief finish rendering of the pass
     */
    void end()
    {
        if (maxParallelism > 1u)
        {
            for (uint32_t i = 0u; i < maxParallelism; ++i)
            {
                vkEndCommandBuffer(secondaryCmdBuffers[i]);
            }

            vkCmdExecuteCommands(cmdBuffer, secondaryCmdBuffers.size(), secondaryCmdBuffers.data());
        }

        vkCmdEndRendering(cmdBuffer);

        if (maxParallelism > 1u)
        {
            // TODO:: creating and destroying pools and cmd buffers every frame is not optimal
            //  Ideally need a global sytem/mngr to provide pools to allocate from
            auto& ctx = VkGfxDevice::getSharedVulkanContext();
            for (uint32_t i = 0u; i < maxParallelism; ++i)
            {
                /*vkFreeCommandBuffers(ctx.device, secondaryCmdPools[i], 1, &secondaryCmdBuffers[i]);
                vkResetCommandPool(ctx.device, secondaryCmdPools[i], 0);*/
            }
        }
    }
};
} // namespace dusk