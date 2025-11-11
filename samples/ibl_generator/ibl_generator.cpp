#include "ibl_generator.h"

#include "core/entrypoint.h"
#include "platform/file_system.h"
#include "renderer/texture_db.h"
#include "backend/vulkan/vk_device.h"

#include <glm/gtc/matrix_transform.hpp>
/*
Pielines required:
    #1 HDR to cubemap
    #2 Irradiance map
    #3 Prefiltered map

TODO:
    * Create a render graph for all cubemaps generation
    * UI options to select environment hdr file, generate and
      save environment's cubemap and ibl assets (prefiltered and irradiance maps).
    * Explore compute based irradiance and prefiltered map generation
*/

const int                 ENV_RENDER_WIDTH          = 1024;
const int                 ENV_RENDER_HEIGHT         = 1024;
const int                 IRRADIANCE_RENDER_WIDTH   = 64;
const int                 IRRADIANCE_RENDER_HEIGHT  = 64;
const int                 PREFILTERED_RENDER_WIDTH  = 512;
const int                 PREFILTERED_RENDER_HEIGHT = 512;

Unique<dusk::Application> dusk::createApplication(int argc, char** argv)
{
    return createUnique<IBLGenerator>();
}

IBLGenerator::IBLGenerator()
{
}

IBLGenerator::~IBLGenerator()
{
}

bool IBLGenerator::start()
{
    m_hdrEnvTextureId              = TextureDB::cache()->createTextureAsync("assets/textures/env.hdr", TextureType::Texture2D);

    auto cubemapVertShaderCode     = FileSystem::readFileBinary("assets/shaders/cubemap.vert.spv");

    auto cubemapFragShaderCode     = FileSystem::readFileBinary("assets/shaders/cubemap.frag.spv");

    auto irradianceFragShaderCode  = FileSystem::readFileBinary("assets/shaders/irradiance.frag.spv");

    auto prefilteredFragShaderCode = FileSystem::readFileBinary("assets/shaders/prefiltered.frag.spv");

    setupCubeProjViewBuffer();
    setupHDRToCubeMapPipeline(cubemapVertShaderCode, cubemapFragShaderCode);
    setupIrradiancePipeline(cubemapVertShaderCode, irradianceFragShaderCode);
    setupPrefilteredPipeline(cubemapVertShaderCode, prefilteredFragShaderCode);

    return true;
}

void IBLGenerator::shutdown()
{
    m_cubeProjViewBuffer.free();
    m_cubeProjViewDescPool->resetPool();
    m_cubeProjViewDescLayout     = nullptr;
    m_cubeProjViewDescPool       = nullptr;

    m_hdrToCubeMapPipeline       = nullptr;
    m_hdrToCubeMapPipelineLayout = nullptr;

    m_irradiancePipeline         = nullptr;
    m_irradiancePipelineLayout   = nullptr;

    m_prefilteredPipeline        = nullptr;
    m_prefilteredPipelineLayout  = nullptr;
}

void IBLGenerator::onUpdate(TimeStep dt)
{
    if (TextureDB::cache()->isTextureUploaded(m_hdrEnvTextureId)
        && !m_executedOnce)
    {
        auto&           vkCtx     = VkGfxDevice::getSharedVulkanContext();
        VkGfxDevice     device    = Engine::get().getGfxDevice();
        VkCommandBuffer cmdBuffer = device.beginSingleTimeCommands();

        executeHDRToCubeMapPipeline(cmdBuffer);
        executeIrradiancePipeline(cmdBuffer);
        executePrefilteredPipeline(cmdBuffer);

        device.endSingleTimeCommands(cmdBuffer);

        m_executedOnce = true;
    }
}

void IBLGenerator::onEvent(dusk::Event& ev)
{
}

void IBLGenerator::executeHDRToCubeMapPipeline(VkCommandBuffer cmdBuffer)
{
    auto& cubeMapAttachment = TextureDB::cache()->getTexture2D(m_hdrCubeMapTextureId);

    vkdebug::cmdBeginLabel(cmdBuffer, "gen_cubemap", glm::vec4(0.7f, 0.7f, 0.f, 0.f));

    cubeMapAttachment.recordTransitionLayout(cmdBuffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    VkRenderingAttachmentInfo attachmentInfo = { VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
    attachmentInfo.imageLayout               = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachmentInfo.imageView                 = cubeMapAttachment.cubeMipImageViews[0];
    attachmentInfo.loadOp                    = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachmentInfo.storeOp                   = VK_ATTACHMENT_STORE_OP_STORE;
    attachmentInfo.clearValue                = { 0.f, 0.f, 0.f, 1.f };

    VkRenderingInfo renderingInfo            = { VK_STRUCTURE_TYPE_RENDERING_INFO };
    renderingInfo.renderArea                 = { { 0, 0 }, { ENV_RENDER_WIDTH, ENV_RENDER_HEIGHT } };
    renderingInfo.layerCount                 = 1;    // multiview
    renderingInfo.viewMask                   = 0x3F; // multiview
    renderingInfo.colorAttachmentCount       = 1;
    renderingInfo.pColorAttachments          = &attachmentInfo;
    renderingInfo.pDepthAttachment           = nullptr;
    renderingInfo.flags                      = VK_RENDERING_CONTENTS_SECONDARY_COMMAND_BUFFERS_BIT;

    vkCmdBeginRendering(cmdBuffer, &renderingInfo);

    VkViewport viewport {};
    viewport.x        = 0.0f;
    viewport.y        = 0.0f;
    viewport.width    = static_cast<float>(ENV_RENDER_WIDTH);
    viewport.height   = static_cast<float>(ENV_RENDER_HEIGHT);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);

    VkRect2D scissor {};
    scissor.offset = { 0, 0 };
    scissor.extent = { ENV_RENDER_WIDTH, ENV_RENDER_HEIGHT };
    vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

    m_hdrToCubeMapPipeline->bind(cmdBuffer);

    vkCmdBindDescriptorSets(
        cmdBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        m_hdrToCubeMapPipelineLayout->get(),
        0,
        1,
        &TextureDB::cache()->getTexturesDescriptorSet().set,
        0,
        nullptr);

    vkCmdBindDescriptorSets(
        cmdBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        m_hdrToCubeMapPipelineLayout->get(),
        1,
        1,
        &m_cubeProjViewDescSet->set,
        0,
        nullptr);

    CubeMapPushConstant push;
    push.equiRectTextureId = m_hdrEnvTextureId;

    vkCmdPushConstants(
        cmdBuffer,
        m_hdrToCubeMapPipelineLayout->get(),
        VK_SHADER_STAGE_FRAGMENT_BIT,
        0,
        sizeof(CubeMapPushConstant),
        &push);

    vkCmdDraw(cmdBuffer, 36, 1, 0, 0);

    vkCmdEndRendering(cmdBuffer);

    cubeMapAttachment.recordMipGenerationCmds(cmdBuffer);

    vkdebug::cmdEndLabel(cmdBuffer);
}

void IBLGenerator::executeIrradiancePipeline(VkCommandBuffer cmdBuffer)
{
    auto& cubeMapAttachment    = TextureDB::cache()->getTexture2D(m_hdrCubeMapTextureId);
    auto& irradianceAttachment = TextureDB::cache()->getTexture2D(m_irradianceTextureId);

    vkdebug::cmdBeginLabel(cmdBuffer, "gen_irradiance", glm::vec4(0.7f, 0.7f, 0.f, 0.f));

    cubeMapAttachment.recordTransitionLayout(cmdBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    irradianceAttachment.recordTransitionLayout(cmdBuffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    VkRenderingAttachmentInfo attachmentInfo = { VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
    attachmentInfo.imageLayout               = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachmentInfo.imageView                 = irradianceAttachment.cubeMipImageViews[0];
    attachmentInfo.loadOp                    = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachmentInfo.storeOp                   = VK_ATTACHMENT_STORE_OP_STORE;
    attachmentInfo.clearValue                = { 0.f, 0.f, 0.f, 1.f };

    VkRenderingInfo renderingInfo            = { VK_STRUCTURE_TYPE_RENDERING_INFO };
    renderingInfo.renderArea                 = { { 0, 0 }, { IRRADIANCE_RENDER_WIDTH, IRRADIANCE_RENDER_HEIGHT } };
    renderingInfo.layerCount                 = 1;    // multiview
    renderingInfo.viewMask                   = 0x3F; // multiview
    renderingInfo.colorAttachmentCount       = 1;
    renderingInfo.pColorAttachments          = &attachmentInfo;
    renderingInfo.pDepthAttachment           = nullptr;
    renderingInfo.flags                      = VK_RENDERING_CONTENTS_SECONDARY_COMMAND_BUFFERS_BIT;

    vkCmdBeginRendering(cmdBuffer, &renderingInfo);

    VkViewport viewport {};
    viewport.x        = 0.0f;
    viewport.y        = 0.0f;
    viewport.width    = static_cast<float>(IRRADIANCE_RENDER_WIDTH);
    viewport.height   = static_cast<float>(IRRADIANCE_RENDER_HEIGHT);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);

    VkRect2D scissor {};
    scissor.offset = { 0, 0 };
    scissor.extent = { IRRADIANCE_RENDER_WIDTH, IRRADIANCE_RENDER_HEIGHT };
    vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

    m_irradiancePipeline->bind(cmdBuffer);

    vkCmdBindDescriptorSets(
        cmdBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        m_irradiancePipelineLayout->get(),
        0,
        1,
        &TextureDB::cache()->getTexturesDescriptorSet().set,
        0,
        nullptr);

    vkCmdBindDescriptorSets(
        cmdBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        m_irradiancePipelineLayout->get(),
        1,
        1,
        &m_cubeProjViewDescSet->set,
        0,
        nullptr);

    IBLPushConstant push;
    push.envCubeMapTextureId = m_hdrCubeMapTextureId;

    vkCmdPushConstants(
        cmdBuffer,
        m_irradiancePipelineLayout->get(),
        VK_SHADER_STAGE_FRAGMENT_BIT,
        0,
        sizeof(IBLPushConstant),
        &push);

    vkCmdDraw(cmdBuffer, 36, 1, 0, 0);

    vkCmdEndRendering(cmdBuffer);

    vkdebug::cmdEndLabel(cmdBuffer);
}

void IBLGenerator::executePrefilteredPipeline(VkCommandBuffer cmdBuffer)
{
    auto&    cubeMapAttachment     = TextureDB::cache()->getTexture2D(m_hdrCubeMapTextureId);
    auto&    prefilteredAttachment = TextureDB::cache()->getTexture2D(m_prefilteredTextureId);

    uint32_t numMipLevels          = prefilteredAttachment.numMipLevels;

    vkdebug::cmdBeginLabel(cmdBuffer, "gen_prefileterd", glm::vec4(0.7f, 0.7f, 0.f, 0.f));

    cubeMapAttachment.recordTransitionLayout(cmdBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    prefilteredAttachment.recordTransitionLayout(cmdBuffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    for (uint32_t level = 0u; level < numMipLevels; ++level)
    {
        std::string labelName = "gen_mip_" + std::to_string(level);
        vkdebug::cmdBeginLabel(cmdBuffer, labelName.c_str(), glm::vec4(0.7f, 0.7f, 0.f, 0.f));

        // Compute per-mip resolution
        uint32_t mipWidth  = PREFILTERED_RENDER_WIDTH >> level;
        uint32_t mipHeight = PREFILTERED_RENDER_HEIGHT >> level;

        // prepare rendering attachment and pipeline configuration
        VkRenderingAttachmentInfo attachmentInfo = { VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
        attachmentInfo.imageLayout               = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        attachmentInfo.imageView                 = prefilteredAttachment.cubeMipImageViews[level];
        attachmentInfo.loadOp                    = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachmentInfo.storeOp                   = VK_ATTACHMENT_STORE_OP_STORE;
        attachmentInfo.clearValue                = { 0.f, 0.f, 0.f, 1.f };

        VkRenderingInfo renderingInfo            = { VK_STRUCTURE_TYPE_RENDERING_INFO };
        renderingInfo.renderArea                 = { { 0, 0 }, { mipWidth, mipHeight } };
        renderingInfo.layerCount                 = 1;    // multiview
        renderingInfo.viewMask                   = 0x3F; // multiview
        renderingInfo.colorAttachmentCount       = 1;
        renderingInfo.pColorAttachments          = &attachmentInfo;
        renderingInfo.pDepthAttachment           = nullptr;
        renderingInfo.flags                      = VK_RENDERING_CONTENTS_SECONDARY_COMMAND_BUFFERS_BIT;

        vkCmdBeginRendering(cmdBuffer, &renderingInfo);

        VkViewport viewport {};
        viewport.x        = 0.0f;
        viewport.y        = 0.0f;
        viewport.width    = static_cast<float>(mipWidth);
        viewport.height   = static_cast<float>(mipHeight);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);

        VkRect2D scissor {};
        scissor.offset = { 0, 0 };
        scissor.extent = { mipWidth, mipHeight };
        vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

        m_prefilteredPipeline->bind(cmdBuffer);

        vkCmdBindDescriptorSets(
            cmdBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            m_prefilteredPipelineLayout->get(),
            0,
            1,
            &TextureDB::cache()->getTexturesDescriptorSet().set,
            0,
            nullptr);

        vkCmdBindDescriptorSets(
            cmdBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            m_prefilteredPipelineLayout->get(),
            1,
            1,
            &m_cubeProjViewDescSet->set,
            0,
            nullptr);

        // Roughness mapped [0, 1]
        float           roughness = static_cast<float>(level) / static_cast<float>(numMipLevels - 1);

        IBLPushConstant push;
        push.envCubeMapTextureId = m_hdrCubeMapTextureId;
        push.roughness           = roughness;
        push.sampleCount         = 1024u;
        push.resolution          = std::max(PREFILTERED_RENDER_WIDTH, PREFILTERED_RENDER_HEIGHT);

        vkCmdPushConstants(
            cmdBuffer,
            m_prefilteredPipelineLayout->get(),
            VK_SHADER_STAGE_FRAGMENT_BIT,
            0,
            sizeof(IBLPushConstant),
            &push);

        vkCmdDraw(cmdBuffer, 36, 1, 0, 0);

        vkCmdEndRendering(cmdBuffer);

        vkdebug::cmdEndLabel(cmdBuffer);
    }

    vkdebug::cmdEndLabel(cmdBuffer);
}

void IBLGenerator::setupCubeProjViewBuffer()
{
    auto& vkCtx            = VkGfxDevice::getSharedVulkanContext();

    m_cubeProjViewDescPool = VkGfxDescriptorPool::Builder(vkCtx)
                                 .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 6) // 6 proj view matrices
                                 .setDebugName("cube_proj_view_desc_pool")
                                 .build(1);

    m_cubeProjViewDescLayout = VkGfxDescriptorSetLayout::Builder(vkCtx)
                                   .addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 6, false)
                                   .setDebugName("cube_proj_view_desc_layout")
                                   .build();

    m_cubeProjViewDescSet = m_cubeProjViewDescPool->allocateDescriptorSet(
        *m_cubeProjViewDescLayout,
        "cube_proj_view_desc_set");

    GfxBuffer::createHostWriteBuffer(
        GfxBufferUsageFlags::StorageBuffer,
        sizeof(CubeProjView),
        6,
        "cube_proj_view_buffer",
        &m_cubeProjViewBuffer);

    DynamicArray<VkDescriptorBufferInfo> pvBufferInfo;
    pvBufferInfo.reserve(6);
    for (uint32_t faceId = 0u; faceId < 6; ++faceId)
    {
        pvBufferInfo.push_back(m_cubeProjViewBuffer.getDescriptorInfoAtIndex(faceId));
    }

    m_cubeProjViewDescSet->configureBuffer(
        0,
        0,
        pvBufferInfo.size(),
        pvBufferInfo.data());

    m_cubeProjViewDescSet->applyConfiguration();

    glm::mat4 projection = glm::perspective(
        glm::radians(90.0f), // fov
        1.0f,                // aspect ratio
        0.1f,                // near plane
        10.0f);              // far plane

    m_cubeProjView = {
        // Right face +X
        projection * glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f),   // Camera at origin
                                 glm::vec3(1.0f, 0.0f, 0.0f),   // Look towards +X
                                 glm::vec3(0.0f, -1.0f, 0.0f)), // Up is -Y
        // Left face -X
        projection * glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f),   // Camera at origin
                                 glm::vec3(-1.0f, 0.0f, 0.0f),  // Look towards -X
                                 glm::vec3(0.0f, -1.0f, 0.0f)), // Up is -Y
                                                                // Top face +Y
        projection * glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f),   // Camera at origin
                                 glm::vec3(0.0f, 1.0f, 0.0f),   // Look towards +Y
                                 glm::vec3(0.0f, 0.0f, 1.0f)),  // Up is +Z
        // Bottom face -Y
        projection * glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f),   // Camera at origin
                                 glm::vec3(0.0f, -1.0f, 0.0f),  // Look towards -Y
                                 glm::vec3(0.0f, 0.0f, -1.0f)), // Up is -Z
        // Front face +Z
        projection * glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f),   // Camera at origin
                                 glm::vec3(0.0f, 0.0f, 1.0f),   // Look towards +Z
                                 glm::vec3(0.0f, -1.0f, 0.0f)), // Up is -Y
        // Back Face -Z
        projection * glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f),   // Camera at origin
                                 glm::vec3(0.0f, 0.0f, -1.0f),  // Look towards -Z
                                 glm::vec3(0.0f, -1.0f, 0.0f)), // Up is -Y
    };

    m_cubeProjViewBuffer.writeAndFlush(0, m_cubeProjView.data(), sizeof(CubeProjView) * 6);
}

void IBLGenerator::setupHDRToCubeMapPipeline(
    DynamicArray<char>& vertShaderCode,
    DynamicArray<char>& fragShaderCode)
{
    auto&    vkCtx        = VkGfxDevice::getSharedVulkanContext();

    uint32_t numMipLevels = static_cast<uint32_t>(
                                std::floor(std::log2(
                                    std::max(
                                        ENV_RENDER_WIDTH,
                                        ENV_RENDER_HEIGHT))))
        + 1;

    m_hdrCubeMapTextureId = TextureDB::cache()->createCubeColorTexture(
        "environment_cubemap",
        ENV_RENDER_WIDTH,
        ENV_RENDER_HEIGHT,
        numMipLevels,
        VK_FORMAT_R32G32B32A32_SFLOAT);

    m_hdrToCubeMapPipelineLayout = VkGfxPipelineLayout::Builder(vkCtx)
                                       .addPushConstantRange(VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(CubeMapPushConstant))
                                       .addDescriptorSetLayout(TextureDB::cache()->getTexturesDescriptorSetLayout().layout)
                                       .addDescriptorSetLayout(m_cubeProjViewDescLayout->layout)
                                       .build();

    m_hdrToCubeMapPipeline = VkGfxRenderPipeline::Builder(vkCtx)
                                 .setVertexShaderCode(vertShaderCode)
                                 .setFragmentShaderCode(fragShaderCode)
                                 .setPipelineLayout(*m_hdrToCubeMapPipelineLayout)
                                 .addColorAttachmentFormat(VK_FORMAT_R32G32B32A32_SFLOAT) // cube color attachment
                                 .setViewMask(0x3F)
                                 .removeVertexInputState()
                                 .setDebugName("cubemap_pipeline")
                                 .build();
}

void IBLGenerator::setupIrradiancePipeline(
    DynamicArray<char>& vertShaderCode,
    DynamicArray<char>& fragShaderCode)
{
    auto& vkCtx           = VkGfxDevice::getSharedVulkanContext();

    m_irradianceTextureId = TextureDB::cache()->createCubeColorTexture(
        "irradiance_cubemap",
        IRRADIANCE_RENDER_WIDTH,
        IRRADIANCE_RENDER_HEIGHT,
        1,
        VK_FORMAT_R32G32B32A32_SFLOAT);

    m_irradiancePipelineLayout = VkGfxPipelineLayout::Builder(vkCtx)
                                     .addPushConstantRange(VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(IBLPushConstant))
                                     .addDescriptorSetLayout(TextureDB::cache()->getTexturesDescriptorSetLayout().layout)
                                     .addDescriptorSetLayout(m_cubeProjViewDescLayout->layout)
                                     .build();

    m_irradiancePipeline = VkGfxRenderPipeline::Builder(vkCtx)
                               .setVertexShaderCode(vertShaderCode)
                               .setFragmentShaderCode(fragShaderCode)
                               .setPipelineLayout(*m_irradiancePipelineLayout)
                               .addColorAttachmentFormat(VK_FORMAT_R32G32B32A32_SFLOAT) // cube color attachment
                               .setViewMask(0x3F)
                               .removeVertexInputState()
                               .setDebugName("irradiance_pipeline")
                               .build();
}

void IBLGenerator::setupPrefilteredPipeline(
    DynamicArray<char>& vertShaderCode,
    DynamicArray<char>& fragShaderCode)
{
    auto&    vkCtx        = VkGfxDevice::getSharedVulkanContext();

    uint32_t numMipLevels = static_cast<uint32_t>(
                                std::floor(std::log2(
                                    std::max(
                                        PREFILTERED_RENDER_WIDTH,
                                        PREFILTERED_RENDER_HEIGHT))))
        + 1;

    m_prefilteredTextureId = TextureDB::cache()->createCubeColorTexture(
        "prefiltered_cubemap",
        PREFILTERED_RENDER_WIDTH,
        PREFILTERED_RENDER_HEIGHT,
        numMipLevels,
        VK_FORMAT_R32G32B32A32_SFLOAT);

    m_prefilteredPipelineLayout = VkGfxPipelineLayout::Builder(vkCtx)
                                      .addPushConstantRange(VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(IBLPushConstant))
                                      .addDescriptorSetLayout(TextureDB::cache()->getTexturesDescriptorSetLayout().layout)
                                      .addDescriptorSetLayout(m_cubeProjViewDescLayout->layout)
                                      .build();

    m_prefilteredPipeline = VkGfxRenderPipeline::Builder(vkCtx)
                                .setVertexShaderCode(vertShaderCode)
                                .setFragmentShaderCode(fragShaderCode)
                                .setPipelineLayout(*m_prefilteredPipelineLayout)
                                .addColorAttachmentFormat(VK_FORMAT_R32G32B32A32_SFLOAT) // cube color attachment
                                .setViewMask(0x3F)
                                .removeVertexInputState()
                                .setDebugName("prefiltered_pipeline")
                                .build();
}
