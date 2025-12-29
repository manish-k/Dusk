#include "environment.h"

#include "texture_db.h"

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

    initCubeTextureResources(shaderPath, resPath, globalDescSetLayout);

    return true;
}

void Environment::initCubeTextureResources(
    std::string&              shaderPath,
    std::string&              resPath,
    VkGfxDescriptorSetLayout& globalDescSetLayout)
{
    auto& ctx = VkGfxDevice::getSharedVulkanContext();

    // env cube map
    const std::string skyboxTexturePath = resPath + "night_01_env.ktx2";
    m_skyTextureId                      = m_textureDB.createTextureAsync(
        skyboxTexturePath, 
        TextureType::Cube,
        PixelFormat::R32G32B32A32_sfloat);

    // irradiance cubemap
    const std::string skyboxIrradianceTexturePath = resPath + "night_01_env_irradiance.ktx2";
    m_skyIrradianceTexId                          = m_textureDB.createTextureAsync(
        skyboxIrradianceTexturePath, 
        TextureType::Cube,
        PixelFormat::R32G32B32A32_sfloat);

    // prefiltered cubemap
    const std::string skyboxPrefilteredTexturePath = resPath + "night_01_env_prefiltered.ktx2";
    m_skyPrefilteredTexId
        = m_textureDB.createTextureAsync(
            skyboxPrefilteredTexturePath, 
            TextureType::Cube,
            PixelFormat::R32G32B32A32_sfloat);

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
                           .removeVertexInputState()
                           .setDebugName("skybox_pipeline")
                           .build();
#ifdef VK_RENDERER_DEBUG
    vkdebug::setObjectName(
        ctx.device,
        VK_OBJECT_TYPE_PIPELINE,
        (uint64_t)m_skyBoxPipeline->get(),
        "skybox_pipeline");
#endif // VK_RENDERER_DEBUG
}

void Environment::cleanup()
{
    m_skyBoxPipeline       = nullptr;
    m_skyBoxPipelineLayout = nullptr;
}

} // namespace dusk