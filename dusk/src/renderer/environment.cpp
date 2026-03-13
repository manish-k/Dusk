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
    std::string       basePath          = STRING(DUSK_BUILD_PATH);

    std::string       shaderPath        = basePath + "/shaders/";
    std::string       resPath           = basePath + "/textures/skybox/";

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

    initSkyBoxPipeline(shaderPath, globalDescSetLayout);

    return true;
}

bool Environment::initHW(VkGfxDescriptorSetLayout& globalDescSetLayout, uint32_t maxFramesCount)
{
    std::string basePath   = STRING(DUSK_BUILD_PATH);

    std::string shaderPath = basePath + "/shaders/";

    initSkyBoxPipeline(shaderPath, globalDescSetLayout);

    // Initialize Hosek-Wilkie parameters with default values for day time
    computeHosekWilkieParams(2.0f, 0.1f, DEFAULT_DAY_SUN_DIRECTION);

    setupHWSkyResources(shaderPath, maxFramesCount);

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

void Environment::setupHWSkyResources(const std::string& shaderPath, uint32_t maxFramesCount)
{
    auto& vkCtx = VkGfxDevice::getSharedVulkanContext();

    // setup textures for Hosek-Wilkie sky model
    uint32_t numMipLevels = static_cast<uint32_t>(
                                std::floor(std::log2(
                                    std::max(
                                        ENV_RENDER_WIDTH,
                                        ENV_RENDER_HEIGHT))))
        + 1;

    m_skyTextureId
        = m_textureDB.createCubeStorageTexture(
            "skybox_texture",
            ENV_RENDER_WIDTH,
            ENV_RENDER_HEIGHT,
            numMipLevels,
            VK_FORMAT_R16G16B16A16_SFLOAT);

    m_skyIrradianceTexId = m_textureDB.createCubeStorageTexture(
        "skybox_irradiance_texture",
        IRRADIANCE_RENDER_WIDTH,
        IRRADIANCE_RENDER_HEIGHT,
        1,
        VK_FORMAT_R16G16B16A16_SFLOAT);

    m_skyPrefilteredTexId = m_textureDB.createCubeStorageTexture(
        "skybox_prefiltered_texture",
        PREFILTERED_RENDER_WIDTH,
        PREFILTERED_RENDER_HEIGHT,
        5,
        VK_FORMAT_R16G16B16A16_SFLOAT);

    // setup descriptors
    m_genCubeDescPool = VkGfxDescriptorPool::Builder(vkCtx)
                            .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3)
                            .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1)
                            .setDebugName("hw_sky_desc_pool")
                            .build(1);

    m_genCubeDescLayout = VkGfxDescriptorSetLayout::Builder(vkCtx)
                              .addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, 1)
                              .addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 1)
                              .addBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 1)
                              .addBinding(3, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 1)
                              .setDebugName("hw_sky_desc_layout")
                              .build();

    m_genCubeDescSet = m_genCubeDescPool->allocateDescriptorSet(*m_genCubeDescLayout, "hw_sky_desc_set");

    GfxBuffer::createHostWriteBuffer(
        GfxBufferUsageFlags::StorageBuffer,
        sizeof(HosekWilkieSkyParams),
        maxFramesCount,
        "hw_sky_params_buffer",
        &m_hwParamsBuffer);

    auto bufferInfo = m_hwParamsBuffer.getDescriptorInfo();
    m_genCubeDescSet->configureBuffer(0, 0, 1, &bufferInfo);

    auto                  tex = m_textureDB.getTexture(m_skyTextureId);

    VkDescriptorImageInfo texDescInfos {};
    texDescInfos.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    texDescInfos.imageView   = tex->imageView;

    m_genCubeDescSet->configureImage(
        1,
        0,
        1,
        &texDescInfos);

    tex                      = m_textureDB.getTexture(m_skyIrradianceTexId);
    texDescInfos.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    texDescInfos.imageView   = tex->imageView;
    m_genCubeDescSet->configureImage(
        2,
        0,
        1,
        &texDescInfos);

    tex                      = m_textureDB.getTexture(m_skyPrefilteredTexId);
    texDescInfos.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    texDescInfos.imageView   = tex->imageView;
    m_genCubeDescSet->configureImage(
        3,
        0,
        1,
        &texDescInfos);

    m_genCubeDescSet->applyConfiguration();

    // setup environment map generation pipeline
    m_genEnvCubeMapPipelineLayout = VkGfxPipelineLayout::Builder(vkCtx)
                                        .addPushConstantRange(VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(IBLPushConstant))
                                        .addDescriptorSetLayout(m_genCubeDescLayout->layout)
                                        .build();

#ifdef VK_RENDERER_DEBUG
    vkdebug::setObjectName(
        vkCtx.device,
        VK_OBJECT_TYPE_PIPELINE_LAYOUT,
        (uint64_t)m_genEnvCubeMapPipelineLayout->get(),
        "gen_env_cubemap_pipeline_layout");
#endif // VK_RENDERER_DEBUG

    auto genEnvShader       = FileSystem::readFileBinary(shaderPath + "/" + "gen_env_cubemap.comp.spv");

    m_genEnvCubeMapPipeline = VkGfxComputePipeline::Builder(vkCtx)
                                  .setComputeShaderCode(genEnvShader)
                                  .setPipelineLayout(*m_genEnvCubeMapPipelineLayout)
                                  .setDebugName("gen_env_cubemap_pipeline")
                                  .build();

#ifdef VK_RENDERER_DEBUG
    vkdebug::setObjectName(
        vkCtx.device,
        VK_OBJECT_TYPE_PIPELINE,
        (uint64_t)m_genEnvCubeMapPipeline->get(),
        "gen_env_cubemap_pipeline");
#endif // VK_RENDERER_DEBUG

    // setup irradiance map generation pipeline
    m_genEnvIrradiancePipelineLayout = VkGfxPipelineLayout::Builder(vkCtx)
                                           .addPushConstantRange(VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(IBLPushConstant))
                                           .addDescriptorSetLayout(m_textureDB.getTexturesDescriptorSetLayout().layout)
                                           .addDescriptorSetLayout(m_genCubeDescLayout->layout)
                                           .build();

#ifdef VK_RENDERER_DEBUG
    vkdebug::setObjectName(
        vkCtx.device,
        VK_OBJECT_TYPE_PIPELINE_LAYOUT,
        (uint64_t)m_genEnvIrradiancePipelineLayout->get(),
        "gen_env_irrad_cubemap_pipeline_layout");
#endif // VK_RENDERER_DEBUG

    auto genEnvIrradShader     = FileSystem::readFileBinary(shaderPath + "/" + "gen_env_irrad_cubemap.comp.spv");

    m_genEnvIrradiancePipeline = VkGfxComputePipeline::Builder(vkCtx)
                                     .setComputeShaderCode(genEnvIrradShader)
                                     .setPipelineLayout(*m_genEnvIrradiancePipelineLayout)
                                     .setDebugName("gen_env_irrad_cubemap_pipeline")
                                     .build();

#ifdef VK_RENDERER_DEBUG
    vkdebug::setObjectName(
        vkCtx.device,
        VK_OBJECT_TYPE_PIPELINE,
        (uint64_t)m_genEnvIrradiancePipeline->get(),
        "gen_env_irrad_cubemap_pipeline");
#endif // VK_RENDERER_DEBUG

    // setup prefiltered map generation pipeline
    m_genEnvPrefilteredPipelineLayout = VkGfxPipelineLayout::Builder(vkCtx)
                                            .addPushConstantRange(VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(IBLPushConstant))
                                            .addDescriptorSetLayout(m_textureDB.getTexturesDescriptorSetLayout().layout)
                                            .addDescriptorSetLayout(m_genCubeDescLayout->layout)
                                            .build();

#ifdef VK_RENDERER_DEBUG
    vkdebug::setObjectName(
        vkCtx.device,
        VK_OBJECT_TYPE_PIPELINE_LAYOUT,
        (uint64_t)m_genEnvPrefilteredPipelineLayout->get(),
        "gen_env_prefil_cubemap_pipeline_layout");
#endif // VK_RENDERER_DEBUG

    auto genEnvPrefilterShader  = FileSystem::readFileBinary(shaderPath + "/" + "gen_env_prefil_cubemap.comp.spv");

    m_genEnvPrefilteredPipeline = VkGfxComputePipeline::Builder(vkCtx)
                                      .setComputeShaderCode(genEnvPrefilterShader)
                                      .setPipelineLayout(*m_genEnvPrefilteredPipelineLayout)
                                      .setDebugName("gen_env_prefil_cubemap_pipeline")
                                      .build();

#ifdef VK_RENDERER_DEBUG
    vkdebug::setObjectName(
        vkCtx.device,
        VK_OBJECT_TYPE_PIPELINE,
        (uint64_t)m_genEnvPrefilteredPipeline->get(),
        "gen_env_prefil_cubemap_pipeline");
#endif // VK_RENDERER_DEBUG
}

void Environment::cleanupHWSkyResources()
{
    m_hwParamsBuffer.cleanup();

    m_genEnvCubeMapPipeline           = nullptr;
    m_genEnvCubeMapPipelineLayout     = nullptr;

    m_genEnvIrradiancePipeline        = nullptr;
    m_genEnvIrradiancePipelineLayout  = nullptr;

    m_genEnvPrefilteredPipeline       = nullptr;
    m_genEnvPrefilteredPipelineLayout = nullptr;
}

void Environment::initSkyBoxPipeline(
    std::string&              shaderPath,
    VkGfxDescriptorSetLayout& globalDescSetLayout)
{
    auto& ctx                    = VkGfxDevice::getSharedVulkanContext();
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

    cleanupHWSkyResources();
}

void Environment::updateHosekWilkieSkyParams(float turbidity, float albedo, glm::vec3 sunDirection)
{
    computeHosekWilkieParams(turbidity, albedo, sunDirection);
}

void Environment::update(FrameData& frameData)
{
    // Update the Hosek-Wilkie parameters buffer if they are dirty
    if (m_hwParamsDirty)
    {
        m_hwParamsBuffer.writeAndFlushAtIndex(frameData.frameIndex, &m_hwParams, sizeof(HosekWilkieSkyParams));
        markHosekWilkieParamsClean();
        m_needToGenerateEnvMaps = true;
    }
}

} // namespace dusk