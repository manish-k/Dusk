#pragma once

#include "dusk.h"
#include "vk.h"
#include "vk_types.h"
#include "vk_device.h"
#include "renderer/gfx_types.h"
#include "debug/profiler.h"

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
    bool                                    useDepth = false;

    DynamicArray<VkRenderingAttachmentInfo> colorAttachmentInfos;
    VkRenderingAttachmentInfo               depthAttachmentInfo {};
    VkRenderingInfo                         renderingInfo {};

    uint32_t                                viewMask       = 0u; // only for multiview
    uint32_t                                layerCount     = 1u; // only for multiview
    DynamicArray<VkCommandBuffer>           secondaryCmdBuffers;

    bool                                    isComputePass = false;

    /**
     * @brief begin rendering of the pass
     */
    void begin(
        const DynamicArray<GfxTexture*>& readAttachments,
        const DynamicArray<GfxTexture*>& writeColorAttachments,
        std::optional<GfxTexture>&       depthAttachment)
    {
        DUSK_PROFILE_SECTION("begin_pass");

        for (const auto& attachment : readAttachments)
        {
            // transition to shader read layout
            attachment->recordTransitionLayout(cmdBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        }

        DynamicArray<VkFormat> colorFormats;

        colorAttachmentInfos.clear();
        for (const auto& attachment : writeColorAttachments)
        {
            DASSERT(attachment != nullptr, "valid texture is required");

            VkRenderingAttachmentInfo colorAttachment { VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
            colorAttachment.imageView   = attachment->imageView;
            colorAttachment.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
            colorAttachment.loadOp      = vulkan::getLoadOp(attachment.loadOp);
            colorAttachment.storeOp     = vulkan::getStoreOp(attachment.storeOp);
            colorAttachment.clearValue  = attachment.clearValue;
            colorAttachmentInfos.push_back(colorAttachment);

            colorFormats.push_back(attachment->format);

            // transition to color out layout
            attachment->recordTransitionLayout(cmdBuffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        }

        if (depthAttachment.has_value())
        {
            depthAttachmentInfo             = { VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
            depthAttachmentInfo.imageView   = depthAttachment->imageView;
            depthAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
            depthAttachmentInfo.loadOp      = vulkan::getLoadOp(depthAttachment.loadOp);
            depthAttachmentInfo.storeOp     = vulkan::getStoreOp(depthAttachment.storeOp);
            depthAttachmentInfo.clearValue  = depthAttachment.clearValue;

            depthAttachment->recordTransitionLayout(cmdBuffer, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
        }

        renderingInfo                      = { VK_STRUCTURE_TYPE_RENDERING_INFO };
        renderingInfo.layerCount           = 1;
        renderingInfo.renderArea           = { { 0, 0 }, extent };
        renderingInfo.colorAttachmentCount = static_cast<uint32_t>(colorAttachmentInfos.size());
        renderingInfo.pColorAttachments    = colorAttachmentInfos.data();
        renderingInfo.pDepthAttachment     = useDepth ? &depthAttachmentInfo : nullptr;
        renderingInfo.flags                = VK_RENDERING_CONTENTS_SECONDARY_COMMAND_BUFFERS_BIT;

        if (viewMask > 0)
        {
            renderingInfo.viewMask   = viewMask;
            renderingInfo.layerCount = layerCount;
        }

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
        DUSK_PROFILE_SECTION("end_pass");

        vkCmdEndRendering(cmdBuffer);
    }

    // TODO: compute passes can be better integrated
    void beginCompute()
    {
    }

    void endCompute()
    {
    }
};
} // namespace dusk