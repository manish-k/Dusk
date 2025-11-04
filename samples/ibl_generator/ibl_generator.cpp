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
    * Create a render graph for generation
    * UI options to select environment hdr file, generate and
      save environment's cubemap and ibl assets (prefiltered and irradiance maps).
*/

const int                 RENDER_WIDTH  = 1024;
const int                 RENDER_HEIGHT = 1024;

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
    auto& ctx         = VkGfxDevice::getSharedVulkanContext();

    m_hdrEnvTextureId = TextureDB::cache()->createTextureAsync("assets/textures/env.hdr", TextureType::Texture2D);

    setupHDRToCubeMapPipeline();

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
}

void IBLGenerator::onUpdate(TimeStep dt)
{
    if (TextureDB::cache()->isTextureUploaded(m_hdrEnvTextureId)
        && !m_executedOnce)
    {
        // executePipelines();

        m_executedOnce = true;
    }
    executePipelines();
}

void IBLGenerator::onEvent(dusk::Event& ev)
{
}

void IBLGenerator::executePipelines()
{
    auto&           vkCtx     = VkGfxDevice::getSharedVulkanContext();
    VkGfxDevice     device    = Engine::get().getGfxDevice();
    VkCommandBuffer cmdBuffer = device.beginSingleTimeCommands();

    // hdr to cubemap pipeline
    auto& cubeMapAttachment = TextureDB::cache()->getTexture2D(m_hdrCubeMapTextureId);

    // APP_DEBUG("starting cube gen pipeline");
    vkdebug::cmdBeginLabel(cmdBuffer, "gen_cubemap", glm::vec4(0.7f, 0.7f, 0.f, 0.f));

    cubeMapAttachment.recordTransitionLayout(cmdBuffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    VkRenderingAttachmentInfo attachmentInfo = { VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
    attachmentInfo.imageLayout               = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachmentInfo.imageView                 = cubeMapAttachment.cubeImageView;
    attachmentInfo.loadOp                    = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachmentInfo.storeOp                   = VK_ATTACHMENT_STORE_OP_STORE;
    attachmentInfo.clearValue                = { 0.f, 0.f, 0.f, 1.f };

    VkRenderingInfo renderingInfo            = { VK_STRUCTURE_TYPE_RENDERING_INFO };
    renderingInfo.renderArea                 = { { 0, 0 }, { RENDER_WIDTH, RENDER_HEIGHT } };
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
    viewport.width    = static_cast<float>(RENDER_WIDTH);
    viewport.height   = static_cast<float>(RENDER_HEIGHT);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);

    VkRect2D scissor {};
    scissor.offset = { 0, 0 };
    scissor.extent = { RENDER_WIDTH, RENDER_HEIGHT };
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

    glm::mat4 projection = glm::perspective(
        glm::radians(90.0f), // fov
        1.0f,                // aspect ratio
        0.1f,                // near plane
        10.0f);              // far plane

    dusk::Array<glm::mat4, 6> projView = {
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

    m_cubeProjViewBuffer.writeAndFlush(0, projView.data(), sizeof(CubeProjView) * 6);

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

    vkdebug::cmdEndLabel(cmdBuffer);

    device.endSingleTimeCommands(cmdBuffer);
}

void IBLGenerator::setupHDRToCubeMapPipeline()
{
    auto& vkCtx           = VkGfxDevice::getSharedVulkanContext();

    m_hdrCubeMapTextureId = TextureDB::cache()->createCubeColorTexture(
        "environment_cubemap",
        1024,
        1024,
        VK_FORMAT_R32G32B32A32_SFLOAT);

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

    auto vertShaderCode          = FileSystem::readFileBinary("assets/shaders/cubemap.vert.spv");

    auto fragShaderCode          = FileSystem::readFileBinary("assets/shaders/cubemap.frag.spv");

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
