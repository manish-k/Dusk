#pragma once

#include "dusk.h"
#include "vk.h"
#include "vk_types.h"

namespace dusk
{
struct VkGfxRenderPassConfig
{
    VkAttachmentDescription colorAttachment;
    VkSubpassDependency     subpassDependency;
};

class VkGfxRenderPass
{
public:
    class Builder
    {
    public:
        Builder(VulkanContext& vkContext);

        Builder&                setColorAttachmentFormat(VkFormat imageFormat);

        Unique<VkGfxRenderPass> build();

    private:
        VkGfxRenderPassConfig m_renderPassConfig;
        VulkanContext&        m_context;
    };

public:
    VkGfxRenderPass(const VulkanContext& vkContext, const VkGfxRenderPassConfig& renderPassConfig);
    ~VkGfxRenderPass();

    bool isValid() const { return m_renderPass != VK_NULL_HANDLE; }

private:
    VkDevice     m_device     = VK_NULL_HANDLE;
    VkRenderPass m_renderPass = VK_NULL_HANDLE;
};
} // namespace dusk