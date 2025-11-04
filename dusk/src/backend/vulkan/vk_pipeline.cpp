#include "vk_pipeline.h"

namespace dusk
{
bool createShaderModule(VkDevice device, const DynamicArray<char>& shaderCode, VkShaderModule* pShaderModule)
{
    VkShaderModuleCreateInfo createInfo {};
    createInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = shaderCode.size();
    createInfo.pCode    = reinterpret_cast<const uint32_t*>(shaderCode.data());

    VulkanResult result = vkCreateShaderModule(device, &createInfo, nullptr, pShaderModule);

    if (result.hasError())
    {
        return false;
    }

    return true;
}

VkGfxRenderPipeline::Builder::Builder(VulkanContext& vkContext) :
    m_context(vkContext)
{
}

VkGfxRenderPipeline::Builder& VkGfxRenderPipeline::Builder::addDynamicState(VkDynamicState state)
{
    m_renderConfig.dynamicStates.push_back(state);
    return *this;
}

VkGfxRenderPipeline::Builder& VkGfxRenderPipeline::Builder::setVertexShaderCode(DynamicArray<char>& shaderCode)
{
    m_renderConfig.vertexShaderCode = shaderCode;
    return *this;
}

VkGfxRenderPipeline::Builder& VkGfxRenderPipeline::Builder::setFragmentShaderCode(DynamicArray<char>& shaderCode)
{
    m_renderConfig.fragmentShaderCode = shaderCode;
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

VkGfxRenderPipeline::Builder& VkGfxRenderPipeline::Builder::addColorAttachmentFormat(VkFormat format)
{
    m_renderConfig.colorAttachmentFormats.push_back(format);

    VkPipelineColorBlendAttachmentState colorBlendAttachment {};
    colorBlendAttachment.colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable         = VK_FALSE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.colorBlendOp        = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp        = VK_BLEND_OP_ADD;
    m_renderConfig.colorBlendAttachment.push_back(colorBlendAttachment);

    return *this;
}

VkGfxRenderPipeline::Builder& VkGfxRenderPipeline::Builder::setCullMode(VkCullModeFlagBits flags)
{
    m_renderConfig.cullMode = flags;
    return *this;
}

VkGfxRenderPipeline::Builder& VkGfxRenderPipeline::Builder::setDepthTest(bool state)
{
    m_renderConfig.enableDepthTest = state;
    return *this;
}

VkGfxRenderPipeline::Builder& VkGfxRenderPipeline::Builder::setDepthWrite(bool state)
{
    m_renderConfig.enableDepthWrites = state;
    return *this;
}

VkGfxRenderPipeline::Builder& VkGfxRenderPipeline::Builder::removeVertexInputState()
{
    m_renderConfig.noInputState = true;
    return *this;
}

VkGfxRenderPipeline::Builder& VkGfxRenderPipeline::Builder::setViewMask(int mask)
{
    m_renderConfig.viewMask = mask;
    return *this;
}

VkGfxRenderPipeline::Builder& VkGfxRenderPipeline::Builder::setDebugName(const std::string& name)
{
    m_renderConfig.debugName = name;
    return *this;
}

Unique<VkGfxRenderPipeline> VkGfxRenderPipeline::Builder::build()
{
    // DASSERT(m_renderConfig.renderPass != VK_NULL_HANDLE, "render pass is required for rendering");
    DASSERT(m_renderConfig.pipelineLayout != VK_NULL_HANDLE, "pipeline layout is required for rendering");
    DASSERT(m_renderConfig.colorAttachmentFormats.size() != 0, "No color attachment format provided")

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
    colorBlendInfo.attachmentCount   = renderConfig.colorBlendAttachment.size();
    colorBlendInfo.blendConstants[0] = 0.0f;
    colorBlendInfo.blendConstants[1] = 0.0f;
    colorBlendInfo.blendConstants[2] = 0.0f;
    colorBlendInfo.blendConstants[3] = 0.0f;
    colorBlendInfo.pAttachments      = renderConfig.colorBlendAttachment.data();

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

    bool moduleResult                   = createShaderModule(
        m_device,
        renderConfig.vertexShaderCode,
        &m_vertexShaderModule);
    if (!moduleResult)
    {
        DUSK_ERROR("Unable to create vertex shader module");
        return;
    }
    vkdebug::setObjectName(
        m_device,
        VK_OBJECT_TYPE_SHADER_MODULE,
        (uint64_t)m_vertexShaderModule,
        "Vertex shader");

    VkPipelineShaderStageCreateInfo vertShaderStageInfo {};
    vertShaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage  = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = m_vertexShaderModule;
    vertShaderStageInfo.pName  = "main";

    moduleResult               = createShaderModule(
        m_device,
        renderConfig.fragmentShaderCode,
        &m_fragmentShaderModule);
    if (!moduleResult)
    {
        DUSK_ERROR("Unable to create fragment shader module");
        return;
    }
    vkdebug::setObjectName(
        m_device,
        VK_OBJECT_TYPE_SHADER_MODULE,
        (uint64_t)m_fragmentShaderModule,
        "Fragment shader");

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
    auto bindingDescriptions   = vulkan::getVertexBindingDescription();
    auto attributeDescriptions = vulkan::getVertexAtrributeDescription();

    // input state info
    VkPipelineVertexInputStateCreateInfo vertexInputInfo {};
    vertexInputInfo.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexAttributeDescriptionCount = attributeDescriptions.size();
    vertexInputInfo.vertexBindingDescriptionCount   = bindingDescriptions.size();
    vertexInputInfo.pVertexAttributeDescriptions    = attributeDescriptions.data();
    vertexInputInfo.pVertexBindingDescriptions      = bindingDescriptions.data();

    // default rasterization info
    VkPipelineRasterizationStateCreateInfo rasterizationInfo {};
    rasterizationInfo.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizationInfo.depthClampEnable        = VK_FALSE;
    rasterizationInfo.rasterizerDiscardEnable = VK_FALSE;
    rasterizationInfo.polygonMode             = VK_POLYGON_MODE_FILL;
    rasterizationInfo.lineWidth               = 1.0f;
    rasterizationInfo.cullMode                = renderConfig.cullMode;
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
    depthStencilInfo.depthTestEnable       = renderConfig.enableDepthTest;
    depthStencilInfo.depthWriteEnable      = renderConfig.enableDepthWrites;
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
    pipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
    pipelineInfo.pViewportState      = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizationInfo;
    pipelineInfo.pMultisampleState   = &multisampleInfo;
    pipelineInfo.pColorBlendState    = &colorBlendInfo;
    pipelineInfo.pDepthStencilState  = &depthStencilInfo;
    pipelineInfo.pDynamicState       = &dynamicStatesInfo;
    pipelineInfo.pVertexInputState   = nullptr;

    if (!renderConfig.noInputState)
    {
        pipelineInfo.pVertexInputState = &vertexInputInfo;
    }

    pipelineInfo.renderPass = VK_NULL_HANDLE;
    pipelineInfo.subpass    = renderConfig.subpassIndex;
    pipelineInfo.layout     = renderConfig.pipelineLayout;

    // dynamic rendering is enabled
    VkPipelineRenderingCreateInfo renderingCreateInfo { VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };
    renderingCreateInfo.colorAttachmentCount    = static_cast<size_t>(renderConfig.colorAttachmentFormats.size());
    renderingCreateInfo.pColorAttachmentFormats = renderConfig.colorAttachmentFormats.data();
    renderingCreateInfo.depthAttachmentFormat   = VK_FORMAT_D32_SFLOAT_S8_UINT;
    renderingCreateInfo.viewMask                = renderConfig.viewMask;

    pipelineInfo.pNext                          = &renderingCreateInfo;

    // create pipeline
    VulkanResult result = vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pipeline);

    // TODO: need better error handling
    if (result.hasError())
    {
        DUSK_ERROR("Unable to create graphics pipeline {}", result.toString());
        m_pipeline = VK_NULL_HANDLE;
    }

#ifdef VK_RENDERER_DEBUG
    vkdebug::setObjectName(
        m_device,
        VK_OBJECT_TYPE_PIPELINE,
        (uint64_t)m_pipeline,
        renderConfig.debugName.c_str());
#endif // VK_RENDERER_DEBUG
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

void VkGfxRenderPipeline::bind(VkCommandBuffer commandBuffer) const
{
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
}

//======================Compute Pipeline================================

VkGfxComputePipeline::Builder::Builder(VulkanContext& vkContext) :
    m_context(vkContext)
{
}

VkGfxComputePipeline::Builder& VkGfxComputePipeline::Builder::setComputeShaderCode(DynamicArray<char>& shaderCode)
{
    m_computeConfig.computeShaderCode = shaderCode;
    return *this;
}

VkGfxComputePipeline::Builder& VkGfxComputePipeline::Builder::setPipelineLayout(VkGfxPipelineLayout& pipelineLayout)
{
    m_computeConfig.pipelineLayout = pipelineLayout.get();
    return *this;
}

VkGfxComputePipeline::Builder& VkGfxComputePipeline::Builder::setDebugName(const std::string& name)
{
    m_computeConfig.debugName = name;
    return *this;
}

Unique<VkGfxComputePipeline> VkGfxComputePipeline::Builder::build()
{
    DASSERT(m_computeConfig.pipelineLayout != VK_NULL_HANDLE, "pipeline layout is required for compute pipeline");

    auto pipeline = createUnique<VkGfxComputePipeline>(m_context, m_computeConfig);

    if (pipeline->isValid())
    {
        return std::move(pipeline);
    }

    return nullptr;
}

VkGfxComputePipeline::VkGfxComputePipeline(VulkanContext& vkContext, VkGfxComputePipelineConfig& computeConfig) :
    m_device(vkContext.device)
{
    bool moduleResult = createShaderModule(
        m_device,
        computeConfig.computeShaderCode,
        &m_computeShaderModule);
    if (!moduleResult)
    {
        DUSK_ERROR("Unable to create compute shader");
        return;
    }
    vkdebug::setObjectName(
        m_device,
        VK_OBJECT_TYPE_SHADER_MODULE,
        (uint64_t)m_computeShaderModule,
        "BRDF LUT Compute shader");

    // Shader stage info
    VkPipelineShaderStageCreateInfo computeShaderStageInfo { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
    computeShaderStageInfo.stage  = VK_SHADER_STAGE_COMPUTE_BIT;
    computeShaderStageInfo.module = m_computeShaderModule;
    computeShaderStageInfo.pName  = "main";

    // Compute pipeline
    VkComputePipelineCreateInfo pipelineInfo {};
    pipelineInfo.sType  = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.layout = computeConfig.pipelineLayout;
    pipelineInfo.stage  = computeShaderStageInfo;

    VulkanResult result = vkCreateComputePipelines(
        m_device,
        VK_NULL_HANDLE,
        1,
        &pipelineInfo,
        nullptr,
        &m_pipeline);

    if (result.hasError())
    {
        DUSK_ERROR("Unable to create graphics pipeline {}", result.toString());
        m_pipeline = VK_NULL_HANDLE;
    }

#ifdef VK_RENDERER_DEBUG
    vkdebug::setObjectName(
        m_device,
        VK_OBJECT_TYPE_PIPELINE,
        (uint64_t)m_pipeline,
        computeConfig.debugName.c_str());
#endif // VK_RENDERER_DEBUG
}

VkGfxComputePipeline::~VkGfxComputePipeline()
{
    if (m_computeShaderModule != VK_NULL_HANDLE)
    {
        vkDestroyShaderModule(m_device, m_computeShaderModule, nullptr);
    }

    if (m_pipeline != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(m_device, m_pipeline, nullptr);
    }
}

void VkGfxComputePipeline::bind(VkCommandBuffer commandBuffer) const
{
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline);
}

} // namespace dusk