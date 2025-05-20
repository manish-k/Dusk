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
    VkCommandBuffer                         cmd;
    VkExtent2D                              extent;
    DynamicArray<VulkanRenderTarget>        colorTargets;
    VulkanRenderTarget                      depthTarget;
    bool                                    useDepth = false;

    DynamicArray<VkRenderingAttachmentInfo> colorAttachmentInfos;
    VkRenderingAttachmentInfo               depthAttachmentInfo {};
    VkRenderingInfo                         renderingInfo {};

    /**
     * @brief begin rendering of the pass
     */
    void begin()
    {
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
            depthAttachmentInfo.clearValue  = depthTarget.clearValue;
        };

        renderingInfo                      = { VK_STRUCTURE_TYPE_RENDERING_INFO };
        renderingInfo.renderArea           = { { 0, 0 }, extent };
        renderingInfo.layerCount           = 1;
        renderingInfo.colorAttachmentCount = static_cast<uint32_t>(colorAttachmentInfos.size());
        renderingInfo.pColorAttachments    = colorAttachmentInfos.data();
        renderingInfo.pDepthAttachment     = useDepth ? &depthAttachmentInfo : nullptr;

        vkCmdBeginRendering(cmd, &renderingInfo);
    }

    /**
     * @brief finish rendering of the pass
     */
    void end()
    {
        vkCmdEndRendering(cmd);
    }
};
} // namespace dusk