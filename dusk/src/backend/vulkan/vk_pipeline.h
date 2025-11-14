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
    DynamicArray<VkDynamicState>                      dynamicStates;
    DynamicArray<VkPipelineColorBlendAttachmentState> colorBlendAttachment {};

    DynamicArray<char>                                vertexShaderCode;   // TODO: avoid copying buffer
    DynamicArray<char>                                fragmentShaderCode; // TODO: avoid copying buffer

    DynamicArray<VkFormat>                            colorAttachmentFormats;

    VkRenderPass                                      renderPass        = VK_NULL_HANDLE;
    uint32_t                                          subpassIndex      = 0u;

    VkPipelineLayout                                  pipelineLayout    = VK_NULL_HANDLE;

    VkCullModeFlagBits                                cullMode          = VK_CULL_MODE_NONE;
    bool                                              noInputState      = false;
    bool                                              enableDepthTest   = true;
    bool                                              enableDepthWrites = true;
    int                                               viewMask          = 0;

    std::string                                       debugName         = "";
};

bool createShaderModule(
    VkDevice                  device,
    const DynamicArray<char>& shaderCode,
    VkShaderModule*           pShaderModule);

class VkGfxRenderPipeline
{
public:
    class Builder
    {
    public:
        Builder(VulkanContext& vkContext);

        Builder& addDynamicState(VkDynamicState state);
        Builder& setVertexShaderCode(DynamicArray<char>& shaderCode);
        Builder& setFragmentShaderCode(DynamicArray<char>& shaderCode);
        Builder& setSubPassIndex(uint32_t index);
        Builder& setPipelineLayout(VkGfxPipelineLayout& pipelineLayout);
        Builder& addColorAttachmentFormat(VkFormat format);
        Builder& setCullMode(VkCullModeFlagBits flags);
        Builder& setDepthTest(bool state);
        Builder& setDepthWrite(bool state);
        Builder& removeVertexInputState();
        Builder& setViewMask(int mask);
        Builder& setDebugName(const std::string& name);

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

    bool       isValid() const { return m_pipeline != VK_NULL_HANDLE; };
    void       bind(VkCommandBuffer commandBuffer) const;
    VkPipeline get() const { return m_pipeline; }

private:
    VkDevice       m_device               = VK_NULL_HANDLE;
    VkPipeline     m_pipeline             = VK_NULL_HANDLE;

    VkShaderModule m_vertexShaderModule   = VK_NULL_HANDLE;
    VkShaderModule m_fragmentShaderModule = VK_NULL_HANDLE;
};

struct VkGfxComputePipelineConfig
{
    DynamicArray<char> computeShaderCode; // TODO: avoid copying buffer
    VkPipelineLayout   pipelineLayout = VK_NULL_HANDLE;
    std::string        debugName      = "";
};

class VkGfxComputePipeline
{
public:
    class Builder
    {
    public:
        Builder(VulkanContext& vkContext);

        Builder& setComputeShaderCode(DynamicArray<char>& shaderCode);
        Builder& setPipelineLayout(VkGfxPipelineLayout& pipelineLayout);
        Builder& setDebugName(const std::string& name);

        /**
         * @brief build VkGfxPipeline object with given compute config
         * @return unique pointer to pipeline object
         */
        Unique<VkGfxComputePipeline> build();

    private:
        VkGfxComputePipelineConfig m_computeConfig {};
        VulkanContext&             m_context;
    };

public:
    VkGfxComputePipeline(VulkanContext& vkContext, VkGfxComputePipelineConfig& computeConfig);
    ~VkGfxComputePipeline();

    CLASS_UNCOPYABLE(VkGfxComputePipeline);

    bool       isValid() const { return m_pipeline != VK_NULL_HANDLE; };
    void       bind(VkCommandBuffer commandBuffer) const;
    VkPipeline get() const { return m_pipeline; }

private:
    VkDevice       m_device              = VK_NULL_HANDLE;
    VkPipeline     m_pipeline            = VK_NULL_HANDLE;

    VkShaderModule m_computeShaderModule = VK_NULL_HANDLE;
};
} // namespace dusk