#include "vk_pipeline_layout.h"

namespace dusk
{
VkGfxPipelineLayout::Builder::Builder(VulkanContext& vkContext) :
    m_context(vkContext)
{
}

VkGfxPipelineLayout::Builder& VkGfxPipelineLayout::Builder::addPushConstantRange(VkShaderStageFlags flags, uint32_t offset, uint32_t size)
{
    DASSERT(!m_layoutConfig.pushConstantRangesMap.has(offset), "push constant offset already exists");

    m_layoutConfig.pushConstantRangesMap.emplace(offset, VkPushConstantRange { flags, offset, size });
    return *this;
}

VkGfxPipelineLayout::Builder& VkGfxPipelineLayout::Builder::addDescriptorSetLayout(VkDescriptorSetLayout layout)
{
    m_layoutConfig.descriptorSetLayouts.push_back(layout);
    return *this;
}

Unique<VkGfxPipelineLayout> VkGfxPipelineLayout::Builder::build() const
{
    auto layout = createUnique<VkGfxPipelineLayout>(m_context, m_layoutConfig);

    if (layout->isValid())
    {
        return std::move(layout);
    }

    return nullptr;
}

VkGfxPipelineLayout::VkGfxPipelineLayout(const VulkanContext& vkContext, const VkGfxPipelineLayoutConfig& layoutConfig)
{
    m_device = vkContext.device;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

    // descriptors
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(layoutConfig.descriptorSetLayouts.size());
    pipelineLayoutInfo.pSetLayouts    = layoutConfig.descriptorSetLayouts.data();

    // push constants
    DynamicArray<VkPushConstantRange> pushConstantRanges(layoutConfig.pushConstantRangesMap.size());

    for (auto pushConstantRange : layoutConfig.pushConstantRangesMap.values())
    {
        pushConstantRanges.push_back(pushConstantRange);
    }

    pipelineLayoutInfo.pushConstantRangeCount = static_cast<uint32_t>(pushConstantRanges.size());
    pipelineLayoutInfo.pPushConstantRanges    = pushConstantRanges.data();

    VulkanResult result                       = vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout);
    
    // TODO: need better error handling
    if (result.hasError())
    {
        DUSK_ERROR("Unable to create pipeline layout {}", result.toString());
    }
}

VkGfxPipelineLayout::~VkGfxPipelineLayout()
{
    if (m_pipelineLayout != VK_NULL_HANDLE)
    {
        vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
    }
}

} // namespace dusk