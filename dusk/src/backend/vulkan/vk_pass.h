#pragma once

#include "dusk.h"
#include "vk.h"
#include "vk_types.h"
#include "vk_device.h"
#include "renderer/gfx_types.h"

#define DEFAULT_COLOR_CLEAR_VALUE   { 0.f, 0.f, 0.f, 1.f }
#define DEFAULT_DEPTH_STENCIL_VALUE { 1.0f, 0 }

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
    DynamicArray<GfxRenderingAttachment>    readAttachments;
    DynamicArray<GfxRenderingAttachment>    writeColorAttachments;
    GfxRenderingAttachment                  depthAttachment;
    bool                                    useDepth = false;

    DynamicArray<VkRenderingAttachmentInfo> colorAttachmentInfos;
    VkRenderingAttachmentInfo               depthAttachmentInfo {};
    VkRenderingInfo                         renderingInfo {};

    DynamicArray<VulkanImageBarier>         preBarriers;

    uint32_t                                maxParallelism = 1u;
    DynamicArray<VkCommandBuffer>           secondaryCmdBuffers;

    /**
     * @brief begin rendering of the pass
     */
    void begin()
    {
        for (const auto& attachment : readAttachments)
        {
            // transition to shader read layout
            attachment.texture->recordTransitionLayout(cmdBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        }

        DynamicArray<VkFormat> colorFormats;

        colorAttachmentInfos.clear();
        for (const auto& attachment : writeColorAttachments)
        {
            DASSERT(attachment.texture, "valid texture is required");

            VkRenderingAttachmentInfo colorAttachment { VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
            colorAttachment.imageView   = attachment.texture->imageView;
            colorAttachment.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
            colorAttachment.loadOp      = vulkan::getLoadOp(attachment.loadOp);
            colorAttachment.storeOp     = vulkan::getStoreOp(attachment.storeOp);
            colorAttachment.clearValue  = attachment.clearValue;
            colorAttachmentInfos.push_back(colorAttachment);

            colorFormats.push_back(attachment.texture->format);

            // transition to color out layout
            attachment.texture->recordTransitionLayout(cmdBuffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        }

        if (useDepth)
        {
            depthAttachmentInfo             = { VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
            depthAttachmentInfo.imageView   = depthAttachment.texture->imageView;
            depthAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
            depthAttachmentInfo.loadOp      = vulkan::getLoadOp(depthAttachment.loadOp);
            depthAttachmentInfo.storeOp     = vulkan::getStoreOp(depthAttachment.storeOp);
            depthAttachmentInfo.clearValue  = depthAttachment.clearValue;

            depthAttachment.texture->recordTransitionLayout(cmdBuffer, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
        }

        renderingInfo                      = { VK_STRUCTURE_TYPE_RENDERING_INFO };
        renderingInfo.renderArea           = { { 0, 0 }, extent };
        renderingInfo.viewMask             = 1;
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
            VkCommandBufferInheritanceRenderingInfo renderingInheritanceInfo { VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_RENDERING_INFO };
            renderingInheritanceInfo.colorAttachmentCount    = static_cast<uint32_t>(colorFormats.size());
            renderingInheritanceInfo.pColorAttachmentFormats = colorFormats.data();
            renderingInheritanceInfo.rasterizationSamples    = VK_SAMPLE_COUNT_1_BIT;

            if (useDepth)
            {
                renderingInheritanceInfo.depthAttachmentFormat = depthAttachment.texture->format;
            }

            VkCommandBufferInheritanceInfo inheritance { VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO };
            inheritance.pNext = &renderingInheritanceInfo;

            for (uint32_t i = 0u; i < maxParallelism; ++i)
            {
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

            vkCmdExecuteCommands(cmdBuffer, maxParallelism, secondaryCmdBuffers.data());
        }

        vkCmdEndRendering(cmdBuffer);
    }
};
} // namespace dusk