#include "environment.h"

#include "texture_db.h"
#include "sub_mesh.h"

#include "platform/file_system.h"

#include "passes/render_passes.h"

#include "backend/vulkan/vk_device.h"
#include "backend/vulkan/vk_pipeline.h"
#include "backend/vulkan/vk_pipeline_layout.h"
#include "backend/vulkan/vk_descriptors.h"

namespace dusk
{

bool Environment::init(VkGfxDescriptorSetLayout& globalDescSetLayout)
{
    auto&                           ctx            = VkGfxDevice::getSharedVulkanContext();
    std::filesystem::path           buildPath      = STRING(DUSK_BUILD_PATH);
    std::filesystem::path           shaderPath     = buildPath / "shaders/";

    std::string                     basePath       = STRING(DUSK_BUILD_PATH);
    const DynamicArray<std::string> skyboxTextures = {
        basePath + "/textures/skybox/px.png",
        basePath + "/textures/skybox/nx.png",
        basePath + "/textures/skybox/ny.png",
        basePath + "/textures/skybox/py.png",
        basePath + "/textures/skybox/pz.png",
        basePath + "/textures/skybox/nz.png"
    };

    m_skyTextureId         = m_textureDB.createTextureAsync(skyboxTextures, TextureType::Cube);
    m_cubeMesh             = SubMesh::createCubeMesh();

    m_skyBoxPipelineLayout = VkGfxPipelineLayout::Builder(ctx)
                                 .addPushConstantRange(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(SkyBoxPushConstant))
                                 .addDescriptorSetLayout(globalDescSetLayout.layout)
                                 .addDescriptorSetLayout(m_textureDB.getTexturesDescriptorSetLayout().layout)
                                 .build();

#ifdef VK_RENDERER_DEBUG
    vkdebug::setObjectName(
        ctx.device,
        VK_OBJECT_TYPE_PIPELINE_LAYOUT,
        (uint64_t)m_skyBoxPipelineLayout->get(),
        "skybox_pipeline_layout");
#endif // VK_RENDERER_DEBUG

    auto skyBoxVertShaderCode    = FileSystem::readFileBinary(shaderPath / "skybox.vert.spv");

    auto skyBoxFragShaderCode    = FileSystem::readFileBinary(shaderPath / "skybox.frag.spv");

    m_skyBoxPipeline = VkGfxRenderPipeline::Builder(ctx)
                                       .setVertexShaderCode(skyBoxVertShaderCode)
                                       .setFragmentShaderCode(skyBoxFragShaderCode)
                                       .setPipelineLayout(*m_skyBoxPipelineLayout)
                                       .addColorAttachmentFormat(VK_FORMAT_R8G8B8A8_SRGB)
                                       .setDepthWrite(false)
                                       .setCullMode(VK_CULL_MODE_FRONT_BIT)
                                       .build();
#ifdef VK_RENDERER_DEBUG
    vkdebug::setObjectName(
        ctx.device,
        VK_OBJECT_TYPE_PIPELINE,
        (uint64_t)m_skyBoxPipeline->get(),
        "skybox_pipeline");
#endif // VK_RENDERER_DEBUG

    return true;
}

void Environment::cleanup()
{
    m_skyBoxPipeline                   = nullptr;
    m_skyBoxPipelineLayout = nullptr;
    m_cubeMesh->free();
}


} // namespace dusk