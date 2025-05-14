#include "basic_render_system.h"

#include "engine.h"
#include "platform/file_system.h"
#include "backend/vulkan/vk_allocator.h"
#include "utils/utils.h"

namespace dusk
{
BasicRenderSystem::BasicRenderSystem(
    VkGfxDevice&              device,
    VkGfxDescriptorSetLayout& globalSet,
    VkGfxDescriptorSetLayout& materialSet) :
    m_device(device)
{
    setupDescriptors();
    createPipelineLayout(globalSet, materialSet);
    createPipeLine();
}

BasicRenderSystem::~BasicRenderSystem()
{
    m_renderPipeline = nullptr;
    m_pipelineLayout = nullptr;

    m_device.unmapBuffer(&m_modelsBuffer);
    m_device.freeBuffer(&m_modelsBuffer);

    m_modelDescriptorPool->resetPool();

    m_modelDescriptorSetLayout = nullptr;
    m_modelDescriptorPool      = nullptr;
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

void BasicRenderSystem::createPipelineLayout(
    VkGfxDescriptorSetLayout& globalSet,
    VkGfxDescriptorSetLayout& materialSet)
{
    VulkanContext& vkContext = VkGfxDevice::getSharedVulkanContext();
    m_pipelineLayout         = VkGfxPipelineLayout::Builder(vkContext)
                           .addPushConstantRange(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(DrawData))
                           .addDescriptorSetLayout(globalSet.layout)
                           .addDescriptorSetLayout(materialSet.layout)
                           .addDescriptorSetLayout(m_modelDescriptorSetLayout->layout)
                           .build();
}

void BasicRenderSystem::setupDescriptors()
{
    auto& ctx             = m_device.getSharedVulkanContext();

    m_modelDescriptorPool = VkGfxDescriptorPool::Builder(ctx)
                                .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1)
                                .build(1);
    CHECK_AND_RETURN(!m_modelDescriptorPool);

    m_modelDescriptorSetLayout = VkGfxDescriptorSetLayout::Builder(ctx)
                                     .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT, 1)
                                     .build();
    CHECK_AND_RETURN(!m_modelDescriptorSetLayout);

    // create dynamic ubo
    size_t          alignmentSize = getAlignment(sizeof(ModelData), ctx.physicalDeviceProperties.limits.minUniformBufferOffsetAlignment);

    uint32_t        bufferSize    = maxRenderableMeshes * alignmentSize;

    GfxBufferParams dynamicUboParams {};
    dynamicUboParams.sizeInBytes   = bufferSize;
    dynamicUboParams.usage         = GfxBufferUsageFlags::UniformBuffer;
    dynamicUboParams.memoryType    = GfxBufferMemoryTypeFlags::HostSequentialWrite | GfxBufferMemoryTypeFlags::PersistentlyMapped;
    dynamicUboParams.alignmentSize = alignmentSize;

    VulkanResult result            = m_device.createBuffer(dynamicUboParams, &m_modelsBuffer);
    if (result.hasError())
    {
        DUSK_ERROR("Unbale to create uniform buffer for models data {}", result.toString());
        return;
    }

    m_device.mapBuffer(&m_modelsBuffer);

    // create descriptor set
    m_modelDescriptorSet = m_modelDescriptorPool->allocateDescriptorSet(*m_modelDescriptorSetLayout);
    CHECK_AND_RETURN(!m_modelDescriptorSet);

    DynamicArray<VkDescriptorBufferInfo> meshBufferInfo(maxRenderableMeshes);
    for (uint32_t meshIdx = 0u; meshIdx < maxRenderableMeshes; ++meshIdx)
    {
        VkDescriptorBufferInfo& bufferInfo = meshBufferInfo[meshIdx];
        bufferInfo.buffer                  = m_modelsBuffer.buffer;
        bufferInfo.offset                  = alignmentSize * meshIdx;
        bufferInfo.range                   = alignmentSize;
    }

    m_modelDescriptorSet->configureBuffer(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 0, 1, meshBufferInfo.data());

    m_modelDescriptorSet->applyConfiguration();
}

void BasicRenderSystem::renderGameObjects(const FrameData& frameData)
{
    if (!frameData.scene) return;

    Scene&           scene         = *frameData.scene;
    VkCommandBuffer  commandBuffer = frameData.commandBuffer;
    CameraComponent& camera        = scene.getMainCamera();

    m_renderPipeline->bind(commandBuffer);

    // TODO:: this might be required only once, check case
    // where new textures are added on the fly (streaming textures)
    vkCmdBindDescriptorSets(
        frameData.commandBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        m_pipelineLayout->get(),
        0,
        1,
        &frameData.globalDescriptorSet,
        0,
        nullptr);

    vkCmdBindDescriptorSets(
        frameData.commandBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        m_pipelineLayout->get(),
        1,
        1,
        &frameData.materialDescriptorSet,
        0,
        nullptr);

    auto renderablesView = scene.GetGameObjectsWith<TransformComponent, MeshComponent>();

    // possiblity of cache unfriendliness here. Only first component is
    // cache friendly. https://gamedev.stackexchange.com/a/212879
    // TODO: Profile below code
    for (auto [entity, transform, meshData] : renderablesView.each())
    {
        for (uint32_t meshId : meshData.meshes)
        {
            SubMesh&     mesh      = scene.getSubMesh(meshId);
            VkBuffer     buffers[] = { mesh.getVertexBuffer().vkBuffer.buffer };
            VkDeviceSize offsets[] = { 0 };

            DrawData     push {};
            push.cameraBufferIdx = frameData.frameIndex;
            push.materialIdx     = meshId;

            // update mesh transform data
            ModelData md { transform.mat4(), transform.normalMat4() };
            void*     dst = (char*)m_modelsBuffer.mappedMemory + sizeof(ModelData) * meshId;
            memcpy(dst, &md, sizeof(ModelData));

            uint32_t dynamicOffset = meshId * static_cast<uint32_t>(m_modelsBuffer.alignmentSize);
            vkCmdBindDescriptorSets(
                commandBuffer,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                m_pipelineLayout->get(),
                2,
                1,
                &m_modelDescriptorSet->set,
                1,
                &dynamicOffset);

            vkCmdPushConstants(
                commandBuffer,
                m_pipelineLayout->get(),
                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                0,
                sizeof(DrawData),
                &push);

            vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);

            vkCmdBindIndexBuffer(commandBuffer, mesh.getIndexBuffer().vkBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

            vkCmdDrawIndexed(commandBuffer, mesh.getIndexCount(), 1, 0, 0, 0);
        }
    }
}

} // namespace dusk