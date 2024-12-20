#include "vk_renderpass.h"
#include "vk_pipeline.h"

namespace dusk
{
VkGfxRenderPass::Builder::Builder(VulkanContext& vkContext) :
    m_context(vkContext)
{
    m_renderPassConfig.colorAttachment.flags             = VK_ATTACHMENT_DESCRIPTION_MAY_ALIAS_BIT;
    m_renderPassConfig.colorAttachment.format            = VK_FORMAT_UNDEFINED;
    m_renderPassConfig.colorAttachment.samples           = VK_SAMPLE_COUNT_1_BIT;
    m_renderPassConfig.colorAttachment.loadOp            = VK_ATTACHMENT_LOAD_OP_CLEAR;
    m_renderPassConfig.colorAttachment.storeOp           = VK_ATTACHMENT_STORE_OP_STORE;
    m_renderPassConfig.colorAttachment.stencilLoadOp     = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    m_renderPassConfig.colorAttachment.stencilStoreOp    = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    m_renderPassConfig.colorAttachment.initialLayout     = VK_IMAGE_LAYOUT_UNDEFINED;
    m_renderPassConfig.colorAttachment.finalLayout       = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    m_renderPassConfig.subpassDependency.dependencyFlags = VK_DEPENDENCY_DEVICE_GROUP_BIT;
    m_renderPassConfig.subpassDependency.srcSubpass      = VK_SUBPASS_EXTERNAL;
    m_renderPassConfig.subpassDependency.dstSubpass      = 0;
    m_renderPassConfig.subpassDependency.srcStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    m_renderPassConfig.subpassDependency.srcAccessMask   = 0;
    m_renderPassConfig.subpassDependency.dstStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    m_renderPassConfig.subpassDependency.dstAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
}

VkGfxRenderPass::Builder& VkGfxRenderPass::Builder::setColorAttachmentFormat(VkFormat imageFormat)
{
    m_renderPassConfig.colorAttachment.format = imageFormat;
    return *this;
}

Unique<VkGfxRenderPass> VkGfxRenderPass::Builder::build()
{
    DASSERT(m_renderPassConfig.colorAttachment.format != VK_FORMAT_UNDEFINED, "color attachment format not specified");

    auto renderPass = createUnique<VkGfxRenderPass>(m_context, m_renderPassConfig);

    if (renderPass->isValid())
    {
        return std::move(renderPass);
    }

    return nullptr;
}

VkGfxRenderPass::VkGfxRenderPass(const VulkanContext& vkContext, const VkGfxRenderPassConfig& renderPassConfig) :
    m_device(vkContext.device)
{
    VkAttachmentReference colorAttachmentRef {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass {};
    subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount    = 1;
    subpass.pColorAttachments       = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = nullptr;

    VkRenderPassCreateInfo renderPassInfo {};
    renderPassInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments    = &renderPassConfig.colorAttachment;
    renderPassInfo.subpassCount    = 1;
    renderPassInfo.pSubpasses      = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies   = &renderPassConfig.subpassDependency;

    // create render pass
    VulkanResult result = vkCreateRenderPass(m_device, &renderPassInfo, nullptr, &m_renderPass);

    // TODO: need better error handling
    if (result.hasError())
    {
        DUSK_ERROR("Unable to create render pass {}", result.toString());
    }
}

VkGfxRenderPass::~VkGfxRenderPass()
{
    if (m_renderPass != VK_NULL_HANDLE)
    {
        vkDestroyRenderPass(m_device, m_renderPass, nullptr);
    }
}

} // namespace dusk