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
    std::string basePath   = STRING(DUSK_BUILD_PATH);

    std::string shaderPath = basePath + "/shaders/";
    std::string resPath    = basePath + "/textures/skybox/";
    
    initSphereTextureResources(shaderPath, resPath, globalDescSetLayout);

    return true;
}

void Environment::initCubeTextureResources(
    std::string&              shaderPath,
    std::string&              resPath,
    VkGfxDescriptorSetLayout& globalDescSetLayout)
{
    auto&                           ctx            = VkGfxDevice::getSharedVulkanContext();

    const DynamicArray<std::string> skyboxTextures = {
        resPath + "px.png",
        resPath + "nx.png",
        resPath + "ny.png",
        resPath + "py.png",
        resPath + "pz.png",
        resPath + "nz.png"
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

    auto skyBoxVertShaderCode = FileSystem::readFileBinary(shaderPath + "/" + "skybox.vert.spv");

    auto skyBoxFragShaderCode = FileSystem::readFileBinary(shaderPath + "/" + "skybox.frag.spv");

    m_skyBoxPipeline          = VkGfxRenderPipeline::Builder(ctx)
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
}

void Environment::initSphereTextureResources(
    std::string&              shaderPath,
    std::string&              resPath,
    VkGfxDescriptorSetLayout& globalDescSetLayout)
{
    auto& ctx = VkGfxDevice::getSharedVulkanContext();

    const DynamicArray<std::string> skyboxTextures = {
        resPath + "hdr_dusk_sky.hdr"
    };


    m_skyTextureId         = m_textureDB.createTextureAsync(skyboxTextures, TextureType::Texture2D);
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
        "hdr_skybox_pipeline_layout");
#endif // VK_RENDERER_DEBUG

    auto skyBoxVertShaderCode = FileSystem::readFileBinary(shaderPath + "/" + "skybox.vert.spv");

    auto skyBoxFragShaderCode = FileSystem::readFileBinary(shaderPath + "/" + "hdr_skybox.frag.spv");

    m_skyBoxPipeline          = VkGfxRenderPipeline::Builder(ctx)
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
        "hdr_skybox_pipeline");
#endif // VK_RENDERER_DEBUG
}

void Environment::cleanup()
{
    m_skyBoxPipeline       = nullptr;
    m_skyBoxPipelineLayout = nullptr;
    m_cubeMesh->free();
}

} // namespace dusk