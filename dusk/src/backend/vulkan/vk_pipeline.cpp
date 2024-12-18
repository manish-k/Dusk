#include "vk_pipeline.h"
#include "vk_pipeline.h"
#include "vk_pipeline.h"
#include "vk_pipeline.h"
#include "vk_pipeline.h"
#include "vk_pipeline.h"

namespace dusk
{
VkGfxRenderPipeline::Builder::Builder(VulkanContext& vkContext) :
    m_context(vkContext)
{
    // default blend attachment state
    m_renderConfig.colorBlendAttachment.colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    m_renderConfig.colorBlendAttachment.blendEnable         = VK_FALSE;
    m_renderConfig.colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    m_renderConfig.colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    m_renderConfig.colorBlendAttachment.colorBlendOp        = VK_BLEND_OP_ADD;
    m_renderConfig.colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    m_renderConfig.colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    m_renderConfig.colorBlendAttachment.alphaBlendOp        = VK_BLEND_OP_ADD;
}

VkGfxRenderPipeline::Builder& VkGfxRenderPipeline::Builder::addDynamicState(VkDynamicState state)
{
    m_renderConfig.dynamicStates.push_back(state);
    return *this;
}

VkGfxRenderPipeline::Builder& VkGfxRenderPipeline::Builder::setVertexShaderCode(Buffer* shaderCode)
{
    m_renderConfig.vertexShaderCode = shaderCode;
    return *this;
}

VkGfxRenderPipeline::Builder& VkGfxRenderPipeline::Builder::setFragmentShaderCode(Buffer* shaderCode)
{
    m_renderConfig.fragmentShaderCode = shaderCode;
    return *this;
}

VkGfxRenderPipeline::Builder& VkGfxRenderPipeline::Builder::setRenderPass(VkGfxRenderPass& renderPass)
{
    m_renderConfig.renderPass = renderPass.m_renderPass;
    return *this;
}

VkGfxRenderPipeline::Builder& VkGfxRenderPipeline::Builder::setSubPassIndex(uint32_t index)
{
    m_renderConfig.subpassIndex = index;
    return *this;
}

VkGfxRenderPipeline::Builder& VkGfxRenderPipeline::Builder::setPipelineLayout(VkGfxPipelineLayout& pipelineLayout)
{
    m_renderConfig.pipelineLayout = pipelineLayout.m_pipelineLayout;
    return *this;
}

Unique<VkGfxRenderPipeline> VkGfxRenderPipeline::Builder::build()
{
    DASSERT(m_renderConfig.renderPass != VK_NULL_HANDLE, "render pass is required for rendering");
    DASSERT(m_renderConfig.pipelineLayout != VK_NULL_HANDLE, "pipeline layout is required for rendering");

    auto pipeline = createUnique<VkGfxRenderPipeline>(m_context, m_renderConfig);

    if (pipeline->isValid())
    {
        return std::move(pipeline);
    }

    return nullptr;
}

VkGfxRenderPipeline::VkGfxRenderPipeline(VulkanContext& vkContext, VkGfxRenderPipelineConfig& renderConfig) :
    m_device(vkContext.device)
{
    // viewport and scissor state
    VkViewport viewport;
    viewport.x        = 0.0f;
    viewport.y        = 0.0f;
    viewport.width    = 0.0f;
    viewport.height   = 0.0f;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor;
    scissor.offset.x      = 0;
    scissor.offset.y      = 0;
    scissor.extent.width  = 0u;
    scissor.extent.height = 0u;

    VkPipelineViewportStateCreateInfo viewportState {};
    viewportState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports    = &viewport;
    viewportState.scissorCount  = 1;
    viewportState.pScissors     = &scissor;

    // color blend info
    VkPipelineColorBlendStateCreateInfo colorBlendInfo {};
    colorBlendInfo.sType             = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendInfo.logicOpEnable     = VK_FALSE;
    colorBlendInfo.logicOp           = VK_LOGIC_OP_COPY;
    colorBlendInfo.attachmentCount   = 1;
    colorBlendInfo.blendConstants[0] = 0.0f;
    colorBlendInfo.blendConstants[1] = 0.0f;
    colorBlendInfo.blendConstants[2] = 0.0f;
    colorBlendInfo.blendConstants[3] = 0.0f;
    colorBlendInfo.pAttachments      = &renderConfig.colorBlendAttachment;

    // input assembly state
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo {};
    inputAssemblyInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyInfo.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

    // Add missing default dynamic states
    if (renderConfig.dynamicStates.size() == 0)
    {
        renderConfig.dynamicStates.push_back(VK_DYNAMIC_STATE_VIEWPORT);
        renderConfig.dynamicStates.push_back(VK_DYNAMIC_STATE_SCISSOR);
    }

    VkPipelineDynamicStateCreateInfo dynamicStatesInfo {};
    dynamicStatesInfo.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicStatesInfo.dynamicStateCount = static_cast<uint32_t>(renderConfig.dynamicStates.size());
    dynamicStatesInfo.pDynamicStates    = renderConfig.dynamicStates.data();

    createShaderModule(renderConfig.vertexShaderCode, &m_vertexShaderModule);
    VkPipelineShaderStageCreateInfo vertShaderStageInfo {};
    vertShaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage  = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = m_vertexShaderModule;
    vertShaderStageInfo.pName  = "main";

    createShaderModule(renderConfig.fragmentShaderCode, &m_fragmentShaderModule);
    VkPipelineShaderStageCreateInfo fragShaderStageInfo {};
    fragShaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = m_fragmentShaderModule;
    fragShaderStageInfo.pName  = "main";

    // shaders list
    VkPipelineShaderStageCreateInfo shaderStages[] = {
        vertShaderStageInfo, fragShaderStageInfo
    };

    // vertex attributes info
    auto bindingDescriptions   = getVertexBindingDescription();
    auto attributeDescriptions = getVertexAtrributeDescription();

    // input state info
    VkPipelineVertexInputStateCreateInfo vertexInputInfo {};
    vertexInputInfo.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.vertexBindingDescriptionCount   = static_cast<uint32_t>(bindingDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions    = attributeDescriptions.data();
    vertexInputInfo.pVertexBindingDescriptions      = bindingDescriptions.data();

    // default rasterization info
    VkPipelineRasterizationStateCreateInfo rasterizationInfo {};
    rasterizationInfo.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizationInfo.depthClampEnable        = VK_FALSE;
    rasterizationInfo.rasterizerDiscardEnable = VK_FALSE;
    rasterizationInfo.polygonMode             = VK_POLYGON_MODE_FILL;
    rasterizationInfo.lineWidth               = 1.0f;
    rasterizationInfo.cullMode                = VK_CULL_MODE_NONE;
    rasterizationInfo.frontFace               = VK_FRONT_FACE_CLOCKWISE;
    rasterizationInfo.depthBiasEnable         = VK_FALSE;
    rasterizationInfo.depthBiasConstantFactor = 0.0f;
    rasterizationInfo.depthBiasClamp          = 0.0f;
    rasterizationInfo.depthBiasSlopeFactor    = 0.0f;

    // default multisampling info
    VkPipelineMultisampleStateCreateInfo multisampleInfo {};
    multisampleInfo.sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleInfo.sampleShadingEnable   = VK_FALSE;
    multisampleInfo.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT;
    multisampleInfo.minSampleShading      = 1.0f;
    multisampleInfo.pSampleMask           = nullptr;
    multisampleInfo.alphaToCoverageEnable = VK_FALSE;
    multisampleInfo.alphaToOneEnable      = VK_FALSE;

    // depth stencil info
    VkPipelineDepthStencilStateCreateInfo depthStencilInfo {};
    depthStencilInfo.sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencilInfo.depthTestEnable       = VK_TRUE;
    depthStencilInfo.depthWriteEnable      = VK_TRUE;
    depthStencilInfo.depthCompareOp        = VK_COMPARE_OP_LESS;
    depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
    depthStencilInfo.minDepthBounds        = 0.0f;
    depthStencilInfo.maxDepthBounds        = 1.0f;
    depthStencilInfo.stencilTestEnable     = VK_FALSE;
    depthStencilInfo.front                 = {};
    depthStencilInfo.back                  = {};

    VkGraphicsPipelineCreateInfo pipelineInfo {};
    pipelineInfo.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount          = 2;
    pipelineInfo.pStages             = shaderStages;
    pipelineInfo.pVertexInputState   = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
    pipelineInfo.pViewportState      = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizationInfo;
    pipelineInfo.pMultisampleState   = &multisampleInfo;
    pipelineInfo.pColorBlendState    = &colorBlendInfo;
    pipelineInfo.pDepthStencilState  = &depthStencilInfo;
    pipelineInfo.pDynamicState       = &dynamicStatesInfo;

    pipelineInfo.renderPass          = renderConfig.renderPass;
    pipelineInfo.subpass             = renderConfig.subpassIndex;
    pipelineInfo.layout              = renderConfig.pipelineLayout;

    // create pipeline
    VulkanResult result = vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pipeline);

    // TODO: need better error handling
    if (result.hasError())
    {
        DUSK_ERROR("Unable to create graphics pipeline {}", result.toString());
    }
}

VkGfxRenderPipeline::~VkGfxRenderPipeline()
{
    if (m_vertexShaderModule != VK_NULL_HANDLE)
    {
        vkDestroyShaderModule(m_device, m_vertexShaderModule, nullptr);
    }

    if (m_fragmentShaderModule != VK_NULL_HANDLE)
    {
        vkDestroyShaderModule(m_device, m_fragmentShaderModule, nullptr);
    }

    if (m_pipeline != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(m_device, m_pipeline, nullptr);
    }
}

void dusk::VkGfxRenderPipeline::createShaderModule(const Buffer* shaderCode, VkShaderModule* shaderModule) const
{
    VkShaderModuleCreateInfo createInfo {};
    createInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = shaderCode->size();
    createInfo.pCode    = reinterpret_cast<const uint32_t*>(shaderCode->data());

    VulkanResult result = vkCreateShaderModule(m_device, &createInfo, nullptr, shaderModule);

    if (result.hasError())
    {
        DUSK_ERROR("Unable to create shader module {}", result.toString());
    }
}

} // namespace dusk