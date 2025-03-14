#include "basic_render_system.h"

#include "engine.h"
#include "platform/file_system.h"

namespace dusk
{
BasicRenderSystem::BasicRenderSystem(VkGfxDevice device) :
    m_device(device)
{
    createPipelineLayout();
    createPipeLine();
}

BasicRenderSystem::~BasicRenderSystem()
{
    m_renderPipeline = nullptr;
    m_pipelineLayout = nullptr;
}

void BasicRenderSystem::createPipeLine()
{
    VulkanContext&        vkContext  = VkGfxDevice::getSharedVulkanContext();
    Engine&               engine     = Engine::get();
    VulkanRenderer&       renderer   = engine.getRenderer();
    VkGfxSwapChain&       swapChain  = renderer.getSwapChain();

    std::filesystem::path buildPath  = STRING(DUSK_BUILD_PATH);
    std::filesystem::path shaderPath = buildPath / "shaders/";

    // load shaders
    auto vertShaderCode = FileSystem::readFileBinary(shaderPath / "basic_shader.vert.spv");

    auto fragShaderCode = FileSystem::readFileBinary(shaderPath / "basic_shader.frag.spv");

    // create pipeline
    m_renderPipeline = VkGfxRenderPipeline::Builder(vkContext)
                           .setVertexShaderCode(vertShaderCode)
                           .setFragmentShaderCode(fragShaderCode)
                           .setPipelineLayout(*m_pipelineLayout)
                           .addColorAttachmentFormat(swapChain.getImageFormat())
                           .build();
}

void BasicRenderSystem::createPipelineLayout()
{
    VulkanContext& vkContext = VkGfxDevice::getSharedVulkanContext();
    m_pipelineLayout         = VkGfxPipelineLayout::Builder(vkContext)
                           .addPushConstantRange(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(BasicPushConstantsData))
                           .build();
}

void BasicRenderSystem::renderGameObjects(const FrameData& frameData)
{
    Scene&           scene         = frameData.scene;
    VkCommandBuffer  commandBuffer = frameData.commandBuffer;
    CameraComponent& camera        = scene.getMainCamera();

    m_renderPipeline->bind(commandBuffer);

    auto renderablesView = scene.GetGameObjectsWith<TransformComponent, MeshComponent>();

    for (auto [entity, transform, meshData] : renderablesView.each())
    {
        BasicPushConstantsData push {};
        push.mvpMatrix = camera.projectionMatrix * camera.viewMatrix * transform.mat4();

        vkCmdPushConstants(commandBuffer,
                           m_pipelineLayout->get(),
                           VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                           0,
                           sizeof(BasicPushConstantsData),
                           &push);

        for (uint32_t meshId : meshData.meshes)
        {
            const SubMesh& mesh      = scene.getSubMesh(meshId);
            VkBuffer       buffers[] = { mesh.getVertexBuffer().buffer };
            VkDeviceSize   offsets[] = { 0 };

            vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);

            vkCmdBindIndexBuffer(commandBuffer, mesh.getIndexBuffer().buffer, 0, VK_INDEX_TYPE_UINT32);

            vkCmdDrawIndexed(commandBuffer, mesh.getIndexCount(), 1, 0, 0, 0);
        }
    }
}

} // namespace dusk