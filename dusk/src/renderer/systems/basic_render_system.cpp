#include "basic_render_system.h"

#include "engine.h"
#include "platform/file_system.h"

namespace dusk
{
BasicRenderSystem::BasicRenderSystem(VkGfxDevice& device, VkGfxDescriptorSetLayout& globalSetLayout) :
    m_device(device)
{
    createPipelineLayout(globalSetLayout.layout);
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

void BasicRenderSystem::createPipelineLayout(VkDescriptorSetLayout globalSetLayout)
{
    VulkanContext& vkContext = VkGfxDevice::getSharedVulkanContext();
    m_pipelineLayout         = VkGfxPipelineLayout::Builder(vkContext)
                           .addPushConstantRange(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(BasicPushConstantsData))
                           .addDescriptorSetLayout(globalSetLayout)
                           .build();
}

void BasicRenderSystem::setupDescriptors()
{
    auto& ctx             = m_device.getSharedVulkanContext();

    m_modelDescriptorPool = createUnique<VkGfxDescriptorPool>(ctx);
    VulkanResult result   = m_modelDescriptorPool
                              ->addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1)
                              .create(1);
    if (result.hasError())
    {
        DUSK_ERROR("Unable to create descriptor pool for model descriptor {}", result.toString());
        m_modelDescriptorPool = nullptr;
        return; // TODO: need proper error handling
    }

    m_modelDescritptorSetLayout = createUnique<VkGfxDescriptorSetLayout>(ctx);
    result                      = m_modelDescritptorSetLayout
                 ->addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT, 1)
                 .create();

    if (result.hasError())
    {
        DUSK_ERROR("Unable to create descriptor pool layout for model descriptor {}", result.toString());
        m_modelDescriptorPool = nullptr;
        return; // TODO: need proper error handling
    }

    // create dynamic ubo 

    // create descriptor set
}

void BasicRenderSystem::renderGameObjects(const FrameData& frameData)
{
    Scene&           scene         = frameData.scene;
    VkCommandBuffer  commandBuffer = frameData.commandBuffer;
    CameraComponent& camera        = scene.getMainCamera();

    m_renderPipeline->bind(commandBuffer);

    // TODO:: this might be required only once, check case
    // where new textures are added on the fly
    vkCmdBindDescriptorSets(
        frameData.commandBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        m_pipelineLayout->get(),
        0,
        1,
        &frameData.globalDescriptorSet,
        0,
        nullptr);

    auto renderablesView = scene.GetGameObjectsWith<TransformComponent, MeshComponent>();

    for (auto [entity, transform, meshData] : renderablesView.each())
    {
        BasicPushConstantsData push {};
        push.model  = transform.mat4();
        push.normal = transform.normalMat4();

        vkCmdPushConstants(commandBuffer,
                           m_pipelineLayout->get(),
                           VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                           0,
                           sizeof(BasicPushConstantsData),
                           &push);

        for (uint32_t meshId : meshData.meshes)
        {
            SubMesh&     mesh      = scene.getSubMesh(meshId);
            VkBuffer     buffers[] = { mesh.getVertexBuffer().buffer };
            VkDeviceSize offsets[] = { 0 };

            vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);

            vkCmdBindIndexBuffer(commandBuffer, mesh.getIndexBuffer().buffer, 0, VK_INDEX_TYPE_UINT32);

            vkCmdDrawIndexed(commandBuffer, mesh.getIndexCount(), 1, 0, 0, 0);
        }
    }
}

} // namespace dusk