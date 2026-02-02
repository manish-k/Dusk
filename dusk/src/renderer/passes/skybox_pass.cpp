#include "render_passes.h"

#include "dusk.h"
#include "vk.h"
#include "frame_data.h"
#include "engine.h"
#include "debug/profiler.h"
#include "ui/editor_ui.h"
#include "environment.h"

namespace dusk
{
void recordSkyBoxCmds(const FrameData& frameData)

{
    DUSK_PROFILE_FUNCTION;

    if (!EditorUI::state().rendererState.useSkybox) return;

    if (!frameData.scene) return;

    Scene& scene       = *frameData.scene;
    auto&  environment = Engine::get().getEnvironment();

    environment.getPipeline().bind(frameData.commandBuffer);

    vkCmdBindDescriptorSets(
        frameData.commandBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        environment.getPipelineLayout().get(),
        0,
        1,
        &frameData.globalDescriptorSet,
        0,
        nullptr);

    vkCmdBindDescriptorSets(
        frameData.commandBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        environment.getPipelineLayout().get(),
        1,
        1,
        &frameData.textureDescriptorSet,
        0,
        nullptr);

    SkyBoxPushConstant push {};
    push.globalUboIdx       = frameData.frameIndex;
    push.skyColorTextureIdx = environment.getSkyTextureId();

    vkCmdPushConstants(
        frameData.commandBuffer,
        environment.getPipelineLayout().get(),
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        0,
        sizeof(SkyBoxPushConstant),
        &push);

    vkCmdDraw(frameData.commandBuffer, 36, 1, 0, 0);
}

} // namespace dusk