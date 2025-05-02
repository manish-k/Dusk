#include "grid_render_system.h"

#include "engine.h"
#include "platform/file_system.h"

namespace dusk
{

GridRenderSystem::GridRenderSystem(
    VkGfxDevice&              device,
    VkGfxDescriptorSetLayout& globalSetLayout) :
    m_device(device)
{
    VulkanContext&  vkContext = VkGfxDevice::getSharedVulkanContext();
    Engine&         engine    = Engine::get();
    VulkanRenderer& renderer  = engine.getRenderer();
    VkGfxSwapChain& swapChain = renderer.getSwapChain();

    // create pipeline layout
    m_pipelineLayout = VkGfxPipelineLayout::Builder(vkContext)
                           .addPushConstantRange(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(GridData))
                           .addDescriptorSetLayout(globalSetLayout.layout)
                           .build();

    // load shaders
    std::filesystem::path buildPath      = STRING(DUSK_BUILD_PATH);
    std::filesystem::path shaderPath     = buildPath / "shaders/";
    auto                  vertShaderCode = FileSystem::readFileBinary(shaderPath / "grid.vert.spv");

    auto                  fragShaderCode = FileSystem::readFileBinary(shaderPath / "grid.frag.spv");

    // create pipeline
    m_renderPipeline = VkGfxRenderPipeline::Builder(vkContext)
                           .setVertexShaderCode(vertShaderCode)
                           .setFragmentShaderCode(fragShaderCode)
                           .setPipelineLayout(*m_pipelineLayout)
                           .addColorAttachmentFormat(swapChain.getImageFormat())
                           .build();
}

GridRenderSystem::~GridRenderSystem()
{
    m_renderPipeline = nullptr;
    m_pipelineLayout = nullptr;
}

void GridRenderSystem::renderGrid(const FrameData& frameData)
{
    //TODO:: to remove scene dependency move main camera out of scene
    if (!frameData.scene) return;

    Scene&           scene         = *frameData.scene;
    VkCommandBuffer  commandBuffer = frameData.commandBuffer;
    CameraComponent& camera        = scene.getMainCamera();

    m_renderPipeline->bind(commandBuffer);

    vkCmdBindDescriptorSets(
        frameData.commandBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        m_pipelineLayout->get(),
        0,
        1,
        &frameData.globalDescriptorSet,
        0,
        nullptr);

    GridData push {};
    push.cameraBufferIdx = frameData.frameIndex;

    vkCmdPushConstants(
        commandBuffer,
        m_pipelineLayout->get(),
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        0,
        sizeof(GridData),
        &push);

    vkCmdDraw(commandBuffer, 6, 1, 0, 0);
}

} // namespace dusk