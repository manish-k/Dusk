#include "render_passes.h"

#include "engine.h"
#include "debug/profiler.h"

#include "renderer/environment.h"
#include "renderer/texture_db.h"

namespace dusk
{

void dispatchGenEnvCubeMapPass(const FrameData& frameData)
{
    DUSK_PROFILE_FUNCTION;

    if (!frameData.scene) return;

    auto& env      = Engine::get().getEnvironment();

    auto& pipeline = env.getEnvCubeMapGenPipeline();

    pipeline.bind(frameData.commandBuffer);

    vkCmdBindDescriptorSets(
        frameData.commandBuffer,
        VK_PIPELINE_BIND_POINT_COMPUTE,
        env.getEnvCubeMapGenPipelineLayout().get(),
        0,
        1,
        &env.getHWSkyDescriptorSet().set,
        0,
        nullptr);

    uint32_t        cubeSize = std::max(ENV_RENDER_WIDTH, ENV_RENDER_HEIGHT);

    IBLPushConstant push     = {};
    push.frameIndex          = frameData.frameIndex;
    push.resolution          = cubeSize;

    vkCmdPushConstants(
        frameData.commandBuffer,
        env.getEnvCubeMapGenPipelineLayout().get(),
        VK_SHADER_STAGE_COMPUTE_BIT,
        0,
        sizeof(IBLPushConstant),
        &push);

    // dispatch compute shader with 6 workgroups, one for each cube map face
    vkCmdDispatch(
        frameData.commandBuffer,
        cubeSize / 8,
        cubeSize / 8,
        6);
}

void dispatchGenEnvIrradiancePass(const FrameData& frameData)
{
    DUSK_PROFILE_FUNCTION;

    if (!frameData.scene) return;

    auto& env      = Engine::get().getEnvironment();

    auto& pipeline = env.getEnvIrradianceGenPipeline();

    pipeline.bind(frameData.commandBuffer);

    vkCmdBindDescriptorSets(
        frameData.commandBuffer,
        VK_PIPELINE_BIND_POINT_COMPUTE,
        env.getEnvIrradianceGenPipelineLayout().get(),
        0,
        1,
        &frameData.textureDescriptorSet,
        0,
        nullptr);

    vkCmdBindDescriptorSets(
        frameData.commandBuffer,
        VK_PIPELINE_BIND_POINT_COMPUTE,
        env.getEnvIrradianceGenPipelineLayout().get(),
        1,
        1,
        &env.getHWSkyDescriptorSet().set,
        0,
        nullptr);

    uint32_t        cubeSize = std::max(IRRADIANCE_RENDER_WIDTH, IRRADIANCE_RENDER_HEIGHT);

    IBLPushConstant push     = {};
    push.frameIndex          = frameData.frameIndex;
    push.envCubeMapTexId     = env.getSkyTextureId();
    push.resolution          = cubeSize;

    vkCmdPushConstants(
        frameData.commandBuffer,
        env.getEnvIrradianceGenPipelineLayout().get(),
        VK_SHADER_STAGE_COMPUTE_BIT,
        0,
        sizeof(IBLPushConstant),
        &push);

    // dispatch compute shader with 6 workgroups, one for each cube map face
    vkCmdDispatch(
        frameData.commandBuffer,
        cubeSize / 8,
        cubeSize / 8,
        6);
}

void dispatchGenEnvPrefilteredPass(const FrameData& frameData)
{
    DUSK_PROFILE_FUNCTION;

    if (!frameData.scene) return;

    auto& env      = Engine::get().getEnvironment();

    auto& pipeline = env.getEnvPrefilteredGenPipeline();

    pipeline.bind(frameData.commandBuffer);
    vkCmdBindDescriptorSets(
        frameData.commandBuffer,
        VK_PIPELINE_BIND_POINT_COMPUTE,
        env.getEnvPrefilteredGenPipelineLayout().get(),
        0,
        1,
        &frameData.textureDescriptorSet,
        0,
        nullptr);

    vkCmdBindDescriptorSets(
        frameData.commandBuffer,
        VK_PIPELINE_BIND_POINT_COMPUTE,
        env.getEnvPrefilteredGenPipelineLayout().get(),
        1,
        1,
        &env.getHWSkyDescriptorSet().set,
        0,
        nullptr);

    uint32_t        cubeSize     = std::max(PREFILTERED_RENDER_WIDTH, PREFILTERED_RENDER_HEIGHT);
    uint32_t        numMipLevels = TextureDB::cache()->getTexture(env.getSkyPrefilteredTextureId())->numMipLevels;

    IBLPushConstant push         = {};
    push.frameIndex              = frameData.frameIndex;
    push.envCubeMapTexId         = env.getSkyTextureId();
    push.sampleCount             = 1024u;
    push.resolution              = cubeSize;

    for (uint32_t level = 0u; level < numMipLevels; ++level)
    {
        push.roughness = static_cast<float>(level) / static_cast<float>(numMipLevels - 1);

        vkCmdPushConstants(
            frameData.commandBuffer,
            env.getEnvPrefilteredGenPipelineLayout().get(),
            VK_SHADER_STAGE_COMPUTE_BIT,
            0,
            sizeof(IBLPushConstant),
            &push);

        // dispatch compute shader with 6 workgroups, one for each cube map face
        vkCmdDispatch(
            frameData.commandBuffer,
            (cubeSize >> level) / 8,
            (cubeSize >> level) / 8,
            6);
    }
}

} // namespace dusk