#pragma once

#include "dusk.h"
#include "vk.h"
#include "vk_types.h"

namespace dusk
{
struct VkGfxPipelineLayoutConfig
{
    HashMap<uint32_t, VkPushConstantRange> pushConstantRangesMap;
    DynamicArray<VkDescriptorSetLayout>    descriptorSetLayouts;
};

class VkGfxPipelineLayout
{
public:
    class Builder
    {
    public:
        Builder(VulkanContext& vkContext);
        ~Builder() = default;

        Builder&                    addPushConstantRange(VkShaderStageFlags flags, uint32_t offset, uint32_t size);
        Builder&                    addDescriptorSetLayout(VkDescriptorSetLayout layout);

        Unique<VkGfxPipelineLayout> build() const;

    private:
        VulkanContext&            m_context;
        VkGfxPipelineLayoutConfig m_layoutConfig {};
    };

public:
    VkGfxPipelineLayout(const VulkanContext& vkContext, const VkGfxPipelineLayoutConfig& layoutConfig);
    ~VkGfxPipelineLayout();

    VkPipelineLayout get() const { return m_pipelineLayout; }
    bool             isValid() const { return m_pipelineLayout != VK_NULL_HANDLE; }

private:
    VkDevice         m_device         = VK_NULL_HANDLE;
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
};
} // namespace dusk