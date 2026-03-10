#include "environment.h"

#include "texture_db.h"

#include "platform/file_system.h"

#include "passes/render_passes.h"

#include "backend/vulkan/vk_device.h"
#include "backend/vulkan/vk_pipeline.h"
#include "backend/vulkan/vk_pipeline_layout.h"
#include "backend/vulkan/vk_descriptors.h"

#include "extras/hw_skymodel/ArHosekSkyModel.h"

namespace dusk
{

bool Environment::init(VkGfxDescriptorSetLayout& globalDescSetLayout)
{
    std::string basePath   = STRING(DUSK_BUILD_PATH);

    std::string shaderPath = basePath + "/shaders/";
    std::string resPath    = basePath + "/textures/skybox/";

    initCubeTextureResources(shaderPath, resPath, globalDescSetLayout);

    // Initialize Hosek-Wilkie parameters with default values for day time
    computeHosekWilkieParams(2.0f, 0.1f, DEFAULT_DAY_SUN_DIRECTION);

    return true;
}

void Environment::computeHosekWilkieParams(float turbidity, float albedo, glm::vec3 sunDirection)
{
    float solar_elevation = std::asin(
        std::clamp(
            sunDirection.y,
            -1.0f,
            1.0f));

    m_hwParams.sunDirection.xyz = glm::normalize(DEFAULT_DAY_SUN_DIRECTION);

    auto* hwSkyState            = arhosek_rgb_skymodelstate_alloc_init(
        turbidity,
        albedo,
        solar_elevation);

    // Copy the Hosek-Wilkie parameters from the C struct to our HosekWilkieSkyParams struct
    m_hwParams.A = glm::vec3(hwSkyState->configs[0][0], hwSkyState->configs[1][0], hwSkyState->configs[2][0]);
    m_hwParams.B = glm::vec3(hwSkyState->configs[0][1], hwSkyState->configs[1][1], hwSkyState->configs[2][1]);
    m_hwParams.C = glm::vec3(hwSkyState->configs[0][2], hwSkyState->configs[1][2], hwSkyState->configs[2][2]);
    m_hwParams.D = glm::vec3(hwSkyState->configs[0][3], hwSkyState->configs[1][3], hwSkyState->configs[2][3]);
    m_hwParams.E = glm::vec3(hwSkyState->configs[0][4], hwSkyState->configs[1][4], hwSkyState->configs[2][4]);
    m_hwParams.F = glm::vec3(hwSkyState->configs[0][5], hwSkyState->configs[1][5], hwSkyState->configs[2][5]);
    m_hwParams.G = glm::vec3(hwSkyState->configs[0][6], hwSkyState->configs[1][6], hwSkyState->configs[2][6]);
    m_hwParams.H = glm::vec3(hwSkyState->configs[0][7], hwSkyState->configs[1][7], hwSkyState->configs[2][7]);
    m_hwParams.I = glm::vec3(hwSkyState->configs[0][8], hwSkyState->configs[1][8], hwSkyState->configs[2][8]);

    // copy radiance values for zenith color
    m_hwParams.zenithColor = glm::vec3(hwSkyState->radiances[0], hwSkyState->radiances[1], hwSkyState->radiances[2]);

    m_hwParamsDirty        = true;

    arhosekskymodelstate_free(hwSkyState);
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

    m_skyBoxRenderPipelineLayout = VkGfxPipelineLayout::Builder(ctx)
                                       .addPushConstantRange(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(SkyBoxPushConstant))
                                       .addDescriptorSetLayout(globalDescSetLayout.layout)
                                       .addDescriptorSetLayout(m_textureDB.getTexturesDescriptorSetLayout().layout)
                                       .build();

#ifdef VK_RENDERER_DEBUG
    vkdebug::setObjectName(
        ctx.device,
        VK_OBJECT_TYPE_PIPELINE_LAYOUT,
        (uint64_t)m_skyBoxRenderPipelineLayout->get(),
        "skybox_pipeline_layout");
#endif // VK_RENDERER_DEBUG

    auto skyBoxVertShaderCode = FileSystem::readFileBinary(shaderPath + "/" + "skybox.vert.spv");

    auto skyBoxFragShaderCode = FileSystem::readFileBinary(shaderPath + "/" + "skybox.frag.spv");

    m_skyBoxRenderPipeline    = VkGfxRenderPipeline::Builder(ctx)
                                 .setVertexShaderCode(skyBoxVertShaderCode)
                                 .setFragmentShaderCode(skyBoxFragShaderCode)
                                 .setPipelineLayout(*m_skyBoxRenderPipelineLayout)
                                 .addColorAttachmentFormat(VK_FORMAT_R16G16B16A16_SFLOAT)
                                 .setDepthWrite(false)
                                 .setCullMode(VK_CULL_MODE_FRONT_BIT)
                                 .removeVertexInputState()
                                 .setDebugName("skybox_pipeline")
                                 .build();
#ifdef VK_RENDERER_DEBUG
    vkdebug::setObjectName(
        ctx.device,
        VK_OBJECT_TYPE_PIPELINE,
        (uint64_t)m_skyBoxRenderPipeline->get(),
        "skybox_pipeline");
#endif // VK_RENDERER_DEBUG
}

void Environment::cleanup()
{
    m_skyBoxRenderPipeline       = nullptr;
    m_skyBoxRenderPipelineLayout = nullptr;
}

} // namespace dusk