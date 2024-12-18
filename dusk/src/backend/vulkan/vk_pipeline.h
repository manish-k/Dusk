#pragma once

#include "dusk.h"
#include "vk.h"
#include "vk_types.h"
#include "vk_renderpass.h"
#include "vk_pipeline_layout.h"

namespace dusk
{
// TODO: Add following configurations
// 1. Src and Dst blend factors and blend operation. Enable/disable of blending
// 2. Depth stencil enable/disable, compare op and front/back ops
// 3. Multi sample count
// 4. Rasteriazation polygon and cullmode, linewidth for wireframes, maybe winding order
struct VkGfxRenderPipelineConfig
{
    DynamicArray<VkDynamicState>        dynamicStates;
    VkPipelineColorBlendAttachmentState colorBlendAttachment {};

    Buffer*                             vertexShaderCode   = nullptr;
    Buffer*                             fragmentShaderCode = nullptr;

    VkRenderPass                        renderPass         = VK_NULL_HANDLE;
    uint32_t                            subpassIndex       = 0u;

    VkPipelineLayout                    pipelineLayout     = VK_NULL_HANDLE;
};

class VkGfxRenderPipeline
{
public:
    class Builder
    {
    public:
        Builder(VulkanContext& vkContext);

        Builder& addDynamicState(VkDynamicState state);
        Builder& setVertexShaderCode(Buffer* shaderCode);
        Builder& setFragmentShaderCode(Buffer* shaderCode);
        Builder& setRenderPass(VkGfxRenderPass& renderPass);
        Builder& setSubPassIndex(uint32_t index);
        Builder& setPipelineLayout(VkGfxPipelineLayout& pipelineLayout);

        /**
         * @brief build VkGfxPipeline object with given config
         * @return unique pointer to pipeline object
         */
        Unique<VkGfxRenderPipeline> build();

    private:
        VkGfxRenderPipelineConfig m_renderConfig {};
        VulkanContext&            m_context;
    };

public:
    VkGfxRenderPipeline(VulkanContext& vkContext, VkGfxRenderPipelineConfig& renderConfig);
    ~VkGfxRenderPipeline();

    CLASS_UNCOPYABLE(VkGfxRenderPipeline);

    bool isValid() const { return m_pipeline != VK_NULL_HANDLE; };

private:
    void createShaderModule(const Buffer* shaderCode, VkShaderModule* shaderModule) const;

private:
    VkDevice       m_device               = VK_NULL_HANDLE;
    VkPipeline     m_pipeline             = VK_NULL_HANDLE;

    VkShaderModule m_vertexShaderModule   = VK_NULL_HANDLE;
    VkShaderModule m_fragmentShaderModule = VK_NULL_HANDLE;
};
} // namespace dusk