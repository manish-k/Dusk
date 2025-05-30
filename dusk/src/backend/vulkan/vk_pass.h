#pragma once

#include "dusk.h"
#include "vk.h"
#include "vk_types.h"

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
    }

    /**
     * @brief finish rendering of the pass
     */
    void end()
    {
        vkCmdEndRendering(cmdBuffer);
    }
};
} // namespace dusk