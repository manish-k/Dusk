#include "skybox_render_system.h"

#include "engine.h"
#include "platform/file_system.h"
#include "loaders/stb_image_loader.h"
#include "ui/ui.h"

#include <string>

namespace dusk
{
// refer ordering https://docs.vulkan.org/spec/latest/chapters/textures.html#_cube_map_face_selection_and_transformations
const DynamicArray<std::string> skyboxTextures = {
    "textures/skybox/px.png",
    "textures/skybox/nx.png",
    "textures/skybox/ny.png",
    "textures/skybox/py.png",
    "textures/skybox/pz.png",
    "textures/skybox/nz.png"
};

SkyboxRenderSystem::SkyboxRenderSystem(VkGfxDevice& device, VkGfxDescriptorSetLayout& globalSetLayout) :
    m_device(device)
{
    VulkanContext& vkContext = VkGfxDevice::getSharedVulkanContext();

    // load cube mesh
    DynamicArray<Vertex> skyboxVertices = {
        // positions
        { glm::vec3(-1.0f, -1.0f, -1.0f) }, // 0
        { glm::vec3(1.0f, -1.0f, -1.0f) },  // 1
        { glm::vec3(1.0f, 1.0f, -1.0f) },   // 2
        { glm::vec3(-1.0f, 1.0f, -1.0f) },  // 3
        { glm::vec3(-1.0f, -1.0f, 1.0f) },  // 4
        { glm::vec3(1.0f, -1.0f, 1.0f) },   // 5
        { glm::vec3(1.0f, 1.0f, 1.0f) },    // 6
        { glm::vec3(-1.0f, 1.0f, 1.0f) }    // 7
    };

    DynamicArray<uint32_t> skyboxIndices = {
        // Back face
        0,
        1,
        2,
        2,
        3,
        0,
        // Front face
        4,
        7,
        6,
        6,
        5,
        4,
        // Left face
        0,
        3,
        7,
        7,
        4,
        0,
        // Right face
        1,
        5,
        6,
        6,
        2,
        1,
        // Top face
        3,
        2,
        6,
        6,
        7,
        3,
        // Bottom face
        0,
        4,
        5,
        5,
        1,
        0
    };

    m_cubeMesh.init(skyboxVertices, skyboxIndices);

    // load skybox textures
    std::string         basePath = STRING(DUSK_BUILD_PATH);
    DynamicArray<Image> images;
    images.reserve(skyboxTextures.size());
    for (uint32_t texIndex = 0u; texIndex < skyboxTextures.size(); ++texIndex)
    {
        auto image = ImageLoader::readImage(basePath + "/" + skyboxTextures[texIndex]);

        DASSERT(image != nullptr);

        images.push_back(*image);
    }

    m_skyboxTexture.init(images);

    setupDescriptors();
    createPipelineLayout(globalSetLayout);
    createPipeLine();

    // enable flag in ui
    UI::state().rendererState.useSkybox = true;

    // free texture images
    for (uint32_t texIndex = 0u; texIndex < skyboxTextures.size(); ++texIndex)
    {
        ImageLoader::freeImage(images[texIndex]);
    }
}

SkyboxRenderSystem::~SkyboxRenderSystem()
{
    m_renderPipeline = nullptr;
    m_pipelineLayout = nullptr;

    m_texDescriptorPool->resetPool();
    m_texDescriptorSetLayout = nullptr;
    m_texDescriptorPool      = nullptr;

    m_skyboxTexture.free();
    m_cubeMesh.free();
}

void SkyboxRenderSystem::renderSkybox(const FrameData& frameData)
{
    // enable/disable from UI
    if (!UI::state().rendererState.useSkybox) return;

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

    vkCmdBindDescriptorSets(
        frameData.commandBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        m_pipelineLayout->get(),
        1,
        1,
        &m_texDescriptorSet->set,
        0,
        nullptr);

    SkyboxData push {};
    push.cameraBufferIdx = frameData.frameIndex;

    vkCmdPushConstants(
        commandBuffer,
        m_pipelineLayout->get(),
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        0,
        sizeof(SkyboxData),
        &push);

    VkBuffer     buffers[1] = { m_cubeMesh.getVertexBuffer().vkBuffer.buffer };
    VkDeviceSize offsets[1] = { 0 };

    vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);

    vkCmdBindIndexBuffer(commandBuffer, m_cubeMesh.getIndexBuffer().vkBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

    vkCmdDrawIndexed(commandBuffer, m_cubeMesh.getIndexCount(), 1, 0, 0, 0);
}

void SkyboxRenderSystem::createPipeLine()
{
    VulkanContext&        vkContext      = VkGfxDevice::getSharedVulkanContext();
    Engine&               engine         = Engine::get();
    VulkanRenderer&       renderer       = engine.getRenderer();
    VkGfxSwapChain&       swapChain      = renderer.getSwapChain();

    std::filesystem::path buildPath      = STRING(DUSK_BUILD_PATH);
    std::filesystem::path shaderPath     = buildPath / "shaders/";
    auto                  vertShaderCode = FileSystem::readFileBinary(shaderPath / "skybox.vert.spv");

    auto                  fragShaderCode = FileSystem::readFileBinary(shaderPath / "skybox.frag.spv");

    // create pipeline
    m_renderPipeline = VkGfxRenderPipeline::Builder(vkContext)
                           .setVertexShaderCode(vertShaderCode)
                           .setFragmentShaderCode(fragShaderCode)
                           .setPipelineLayout(*m_pipelineLayout)
                           .addColorAttachmentFormat(swapChain.getImageFormat())
                           .setCullMode(VK_CULL_MODE_FRONT_BIT)
                           .build();
}
void SkyboxRenderSystem::createPipelineLayout(VkGfxDescriptorSetLayout& globalSetLayout)
{
    VulkanContext& vkContext = VkGfxDevice::getSharedVulkanContext();

    m_pipelineLayout         = VkGfxPipelineLayout::Builder(vkContext)
                           .addPushConstantRange(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(SkyboxData))
                           .addDescriptorSetLayout(globalSetLayout.layout)
                           .addDescriptorSetLayout(m_texDescriptorSetLayout->layout)
                           .build();
}

void SkyboxRenderSystem::setupDescriptors()
{
    VulkanContext& vkContext = VkGfxDevice::getSharedVulkanContext();

    m_texDescriptorPool      = VkGfxDescriptorPool::Builder(vkContext)
                              .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1)
                              .build(1);
    CHECK_AND_RETURN(!m_texDescriptorPool);

    m_texDescriptorSetLayout = VkGfxDescriptorSetLayout::Builder(vkContext)
                                   .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1)
                                   .build();
    CHECK_AND_RETURN(!m_texDescriptorSetLayout);

    m_texDescriptorSet = m_texDescriptorPool->allocateDescriptorSet(*m_texDescriptorSetLayout);
    CHECK_AND_RETURN(!m_texDescriptorSet);

    // write image info to descriptor
    VkDescriptorImageInfo imageInfo {};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView   = m_skyboxTexture.vkTexture.imageView;
    imageInfo.sampler     = m_skyboxTexture.vkSampler.sampler;

    m_texDescriptorSet->configureImage(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, 1, &imageInfo);

    m_texDescriptorSet->applyConfiguration();
}

} // namespace dusk
