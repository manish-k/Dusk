#include "engine.h"

#include "core/application.h"
#include "platform/window.h"
#include "platform/file_system.h"
#include "utils/utils.h"
#include "ui/ui.h"
#include "debug/profiler.h"

#include "scene/scene.h"
#include "scene/components/camera.h"

#include "events/event.h"
#include "events/app_event.h"

#include "backend/vulkan/vk_device.h"
#include "backend/vulkan/vk_renderer.h"
#include "backend/vulkan/vk_descriptors.h"

#include "renderer/material.h"
#include "renderer/texture.h"
#include "renderer/systems/basic_render_system.h"
#include "renderer/systems/grid_render_system.h"
#include "renderer/systems/skybox_render_system.h"
#include "renderer/systems/lights_system.h"
#include "renderer/render_graph.h"
#include "renderer/passes/render_passes.h"

namespace dusk
{
Engine* Engine::s_instance = nullptr;

Engine::Engine(const Engine::Config& config) :
    m_config(config)
{
    DASSERT(!s_instance, "Engine instance already exists");
    s_instance = this;
}

Engine::~Engine() { }

bool Engine::start(Shared<Application> app)
{
    DASSERT(!m_app, "Application is null")

    m_app = app;

    // create window
    auto windowProps = Window::Properties::defaultWindowProperties();
    windowProps.mode = Window::Mode::Maximized;
    m_window         = std::move(Window::createWindow(windowProps));

    if (!m_window)
    {
        DUSK_ERROR("Window creation failed");
        return false;
    }
    m_window->setEventCallback([this](Event& ev)
                               { this->onEvent(ev); });

    // device creation
    m_gfxDevice = createUnique<VkGfxDevice>(*dynamic_cast<GLFWVulkanWindow*>(m_window.get()));
    if (m_gfxDevice->initGfxDevice() != Error::Ok)
    {
        DUSK_ERROR("Gfx device creation failed");
        return false;
    }

    m_renderer = createUnique<VulkanRenderer>(*dynamic_cast<GLFWVulkanWindow*>(m_window.get()));
    if (!m_renderer->init())
    {
        DUSK_ERROR("Renderer initialization failed");
        return false;
    }

    m_running          = true;
    m_paused           = false;

    bool globalsStatus = setupGlobals();
    if (!globalsStatus) return false;

    m_lightsSystem      = createUnique<LightsSystem>();

    m_basicRenderSystem = createUnique<BasicRenderSystem>(
        *m_gfxDevice,
        *m_globalDescriptorSetLayout,
        *m_materialDescriptorSetLayout,
        m_lightsSystem->getLightsDescriptorSetLayout());

    m_gridRenderSystem = createUnique<GridRenderSystem>(
        *m_gfxDevice,
        *m_globalDescriptorSetLayout);

    m_skyboxRenderSystem = createUnique<SkyboxRenderSystem>(
        *m_gfxDevice,
        *m_globalDescriptorSetLayout);

    prepareRenderGraphResources();

    m_ui = createUnique<UI>();
    if (!m_ui->init(*m_window))
    {
        DUSK_ERROR("Unable to init UI");
        return false;
    }

    return true;
}

void Engine::run()
{
    DUSK_PROFILE_FUNCTION;

    TimePoint m_lastFrameTime = Time::now();

    while (m_running)
    {
        DUSK_PROFILE_FRAME;

        TimePoint newTime = Time::now();
        m_deltaTime       = newTime - m_lastFrameTime;
        m_lastFrameTime   = newTime;

        // poll events from window
        m_window->onUpdate(m_deltaTime);

        onUpdate(m_deltaTime);

        m_app->onUpdate(m_deltaTime);
    }
}

void Engine::stop() { m_running = false; }

void Engine::shutdown()
{
    m_basicRenderSystem  = nullptr;
    m_gridRenderSystem   = nullptr;
    m_skyboxRenderSystem = nullptr;

    m_lightsSystem       = nullptr;

    m_ui->shutdown();
    m_ui = nullptr;

    cleanupGlobals();

    m_renderer->cleanup();
    m_gfxDevice->cleanupGfxDevice();
}

void Engine::onUpdate(TimeStep dt)
{
    DUSK_PROFILE_FUNCTION;

    uint32_t currentFrameIndex = m_renderer->getCurrentFrameIndex();

    if (VkCommandBuffer commandBuffer = m_renderer->beginFrame())
    {
        auto      extent = m_renderer->getSwapChain().getCurrentExtent();

        FrameData frameData {
            currentFrameIndex,
            dt,
            commandBuffer,
            m_currentScene,
            extent.width,
            extent.height,
            m_globalDescriptorSet->set,
            m_lightsSystem->getLightsDescriptorSet().set,
            m_materialsDescriptorSet->set
        };

        // m_renderer->beginRendering(commandBuffer);

        // m_ui->beginRendering();

        if (m_currentScene)
        {
            DUSK_PROFILE_SECTION("scene_updates");

            m_currentScene->onUpdate(dt);

            CameraComponent& camera = m_currentScene->getMainCamera();
            camera.setAspectRatio(m_renderer->getAspectRatio());

            GlobalUbo ubo {};
            ubo.view        = camera.viewMatrix;
            ubo.prjoection  = camera.projectionMatrix;
            ubo.inverseView = camera.inverseViewMatrix;

            m_lightsSystem->updateLights(*m_currentScene, ubo);

            m_globalUbos.writeAndFlushAtIndex(currentFrameIndex, &ubo, sizeof(GlobalUbo));

            updateMaterialsBuffer(m_currentScene->getMaterials());

            // TODO:: maybe scene onUpdate is the right place for this
            // m_ui->renderSceneWidgets(*m_currentScene);
        }

        // m_basicRenderSystem->renderGameObjects(frameData);

        // m_gridRenderSystem->renderGrid(frameData);
        // m_skyboxRenderSystem->renderSkybox(frameData);

        // m_ui->endRendering(commandBuffer);

        renderFrame(frameData);

        // m_renderer->endRendering(commandBuffer);

        m_renderer->endFrame();
    }

    {
        DUSK_PROFILE_SECTION("wait_idle");
        m_renderer->deviceWaitIdle();
    }
}

void Engine::onEvent(Event& ev)
{
    EventDispatcher dispatcher(ev);

    dispatcher.dispatch<WindowCloseEvent>(
        [this](WindowCloseEvent ev)
        {
            DUSK_INFO("WindowCloseEvent received");
            this->stop();
            return false;
        });

    dispatcher.dispatch<WindowIconifiedEvent>(
        [this](WindowIconifiedEvent ev)
        {
            DUSK_INFO("WindowIconifiedEvent received");
            if (ev.isIconified())
            {
                // DUSK_INFO("Window minimized, pausing engine");
                m_paused = true;
            }
            else
            {
                // DUSK_INFO("Window restored, resuming engine");
                m_paused = false;
            }

            return false;
        });

    // pass event to UI layer
    m_ui->onEvent(ev);

    // pass event to debug layer

    // pass event to the scene
    if (m_currentScene && !ev.isHandled())
    {
        m_currentScene->onEvent(ev);
    }

    // pass unhandled event to application
    if (!ev.isHandled())
    {
        m_app->onEvent(ev);
    }
}

void Engine::loadScene(Scene* scene)
{
    m_currentScene = scene;
    registerTextures(scene->getTextures());
    registerMaterials(scene->getMaterials());

    m_lightsSystem->registerAllLights(*scene);
}

void Engine::renderFrame(FrameData& frameData)
{
    DUSK_PROFILE_FUNCTION;

    RenderGraph renderGraph;

    // create g-buffer pass
    auto gbuffCtx = VkGfxRenderPassContext {
        .colorTargets = m_rgResources.gbuffRenderTargets, // TODO: avoidable copy
        .depthTarget  = m_rgResources.gbuffDepthTexture,
        .useDepth     = true
    };

    gbuffCtx.insertTransitionBarrier(
        { gbuffCtx.colorTargets[0].image.image,
          VK_IMAGE_LAYOUT_UNDEFINED,
          VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
          0,
          VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
          VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
          VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT });
    gbuffCtx.insertTransitionBarrier(
        { gbuffCtx.colorTargets[1].image.image,
          VK_IMAGE_LAYOUT_UNDEFINED,
          VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
          0,
          VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
          VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
          VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT });

    renderGraph.setPassContext("gbuffer_pass", gbuffCtx);
    renderGraph.addPass("gbuffer_pass", recordGBufferCmds);

    // create lighting pass

    // create presentation pass
    VulkanRenderTarget swapImageTarget = {
        .image     = { .image = m_renderer->getSwapChain().getImage(frameData.frameIndex) },
        .imageView = m_renderer->getSwapChain().getImageView(frameData.frameIndex),
        .format    = m_renderer->getSwapChain().getImageFormat()
    };

    auto presentCtx = VkGfxRenderPassContext {
        .colorTargets = { swapImageTarget },
        .useDepth     = false
    };

    presentCtx.insertTransitionBarrier(
        { gbuffCtx.colorTargets[0].image.image,
          VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
          VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
          VK_ACCESS_SHADER_READ_BIT,
          VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
          VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT });
    presentCtx.insertTransitionBarrier(
        { swapImageTarget.image.image,
          VK_IMAGE_LAYOUT_UNDEFINED,
          VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
          0,
          VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
          VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
          VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT });

    renderGraph.setPassContext("present_pass", presentCtx);
    renderGraph.addPass("present_pass", recordPresentationCmds);

    // execute render graph
    renderGraph.execute(frameData);
}

bool Engine::setupGlobals()
{
    uint32_t      maxFramesCount = m_renderer->getMaxFramesCount();
    VulkanContext ctx            = VkGfxDevice::getSharedVulkanContext();

    // create global descriptor pool
    m_globalDescriptorPool = VkGfxDescriptorPool::Builder(ctx)
                                 .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, maxFramesCount)
                                 .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 100) // TODO: make count configurable
                                 .setDebugName("global_desc_pool")
                                 .build(1, VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT);
    CHECK_AND_RETURN_FALSE(!m_globalDescriptorPool);

    m_globalDescriptorSetLayout = VkGfxDescriptorSetLayout::Builder(ctx)
                                      .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS, maxFramesCount, true)
                                      .addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1000, true) // TODO: make count configurable
                                      .setDebugName("global_desc_set_layout")
                                      .build();
    CHECK_AND_RETURN_FALSE(!m_globalDescriptorSetLayout);

    GfxBuffer::createHostWriteBuffer(
        GfxBufferUsageFlags::UniformBuffer,
        sizeof(GlobalUbo),
        maxFramesCount,
        "global_uniform_buffer",
        &m_globalUbos);
    CHECK_AND_RETURN_FALSE(!m_globalUbos.isAllocated());

    m_globalDescriptorSet = m_globalDescriptorPool->allocateDescriptorSet(*m_globalDescriptorSetLayout, "global_desc_set");
    CHECK_AND_RETURN_FALSE(!m_globalDescriptorSet);

    DynamicArray<VkDescriptorBufferInfo> buffersInfo;
    buffersInfo.reserve(maxFramesCount);

    for (uint32_t frameIndex = 0u; frameIndex < maxFramesCount; ++frameIndex)
    {
        buffersInfo.push_back(m_globalUbos.getDescriptorInfoAtIndex(frameIndex));
    }
    m_globalDescriptorSet->configureBuffer(
        0,
        0,
        buffersInfo.size(),
        buffersInfo.data());

    m_globalDescriptorSet->applyConfiguration();

    // create descriptor pool and layout for materials
    m_materialDescriptorPool = VkGfxDescriptorPool::Builder(ctx)
                                   .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, maxMaterialCount) // TODO: make count configurable
                                   .setDebugName("material_desc_pool")
                                   .build(1, VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT);
    CHECK_AND_RETURN_FALSE(!m_materialDescriptorPool);

    m_materialDescriptorSetLayout = VkGfxDescriptorSetLayout::Builder(ctx)
                                        .addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, maxMaterialCount, true) // TODO: make count configurable
                                        .setDebugName("material_desc_set_layout")
                                        .build();
    CHECK_AND_RETURN_FALSE(!m_materialDescriptorSetLayout);

    // create storage buffer of storing materials;
    GfxBuffer::createHostWriteBuffer(
        GfxBufferUsageFlags::StorageBuffer,
        sizeof(Material),
        maxMaterialCount,
        "material_buffer",
        &m_materialsBuffer);
    CHECK_AND_RETURN_FALSE(!m_materialsBuffer.isAllocated());

    m_materialsDescriptorSet = m_materialDescriptorPool->allocateDescriptorSet(*m_materialDescriptorSetLayout, "material_desc_set");
    CHECK_AND_RETURN_FALSE(!m_materialsDescriptorSet);

    return true;
}

void Engine::cleanupGlobals()
{
    m_globalDescriptorPool->resetPool();
    m_globalDescriptorSetLayout = nullptr;
    m_globalDescriptorPool      = nullptr;

    m_materialDescriptorPool->resetPool();
    m_materialDescriptorSetLayout = nullptr;
    m_materialDescriptorPool      = nullptr;

    m_globalUbos.free();
    m_materialsBuffer.free();

    releaseRenderGraphResources();
}

void Engine::registerTextures(DynamicArray<Texture2D>& textures)
{
    for (uint32_t texIndex = 0u; texIndex < textures.size(); ++texIndex)
    {
        Texture2D&            tex = textures[texIndex];

        VkDescriptorImageInfo imageInfo {};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView   = tex.vkTexture.imageView;
        imageInfo.sampler     = tex.vkSampler.sampler;

        m_globalDescriptorSet->configureImage(
            1,
            tex.id,
            1,
            &imageInfo);
    }

    m_globalDescriptorSet->applyConfiguration();
}

void Engine::registerMaterials(DynamicArray<Material>& materials)
{
    DynamicArray<VkDescriptorBufferInfo> matInfo;
    matInfo.reserve(materials.size());

    for (uint32_t matIndex = 0u; matIndex < materials.size(); ++matIndex)
    {
        matInfo.push_back(m_materialsBuffer.getDescriptorInfoAtIndex(matIndex));
    }

    m_materialsDescriptorSet->configureBuffer(
        0,
        0,
        materials.size(),
        matInfo.data());

    m_materialsDescriptorSet->applyConfiguration();
}

void Engine::updateMaterialsBuffer(DynamicArray<Material>& materials)
{
    for (uint32_t index = 0u; index < materials.size(); ++index)
    {
        DASSERT(materials[index].id != -1);
        m_materialsBuffer.writeAndFlushAtIndex(materials[index].id, &materials[index], sizeof(Material));
    }
}

void Engine::prepareRenderGraphResources()
{
    auto& ctx    = VkGfxDevice::getSharedVulkanContext();
    auto  extent = m_renderer->getSwapChain().getCurrentExtent();

    // g-buffer resources
    // Allocate g-buffer render targets
    m_rgResources.gbuffRenderTargets.push_back(m_gfxDevice->createRenderTarget(
        "gbuffer_albedo",
        extent.width,
        extent.height,
        VK_FORMAT_R8G8B8A8_UNORM,
        { 0.f, 0.f, 0.f, 1.f }));
    m_rgResources.gbuffRenderTargets.push_back(m_gfxDevice->createRenderTarget(
        "gbuffer_normal",
        extent.width,
        extent.height,
        VK_FORMAT_R16G16B16A16_SFLOAT,
        { 0.f, 0.f, 0.f, 1.f }));

    // Allocate g-buffer depth texture
    m_rgResources.gbuffDepthTexture = m_gfxDevice->createDepthTarget(
        "gbuffer_depth",
        extent.width,
        extent.height,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        { 0.f, 0.f, 0.f, 1.f });

    m_rgResources.gbuffModelDescriptorPool = VkGfxDescriptorPool::Builder(ctx)
                                                 .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1)
                                                 .setDebugName("model_desc_pool")
                                                 .build(1);

    m_rgResources.gbuffModelDescriptorSetLayout = VkGfxDescriptorSetLayout::Builder(ctx)
                                                      .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT, 1)
                                                      .setDebugName("model_desc_set_layout")
                                                      .build();

    // create dynamic ubo
    GfxBuffer::createHostWriteBuffer(
        GfxBufferUsageFlags::UniformBuffer,
        sizeof(ModelData),
        maxRenderableMeshes,
        "model_buffer",
        &m_rgResources.gbuffModelsBuffer);

    // create model descriptor set
    m_rgResources.gbuffModelDescriptorSet = m_rgResources.gbuffModelDescriptorPool->allocateDescriptorSet(
        *m_rgResources.gbuffModelDescriptorSetLayout, "model_desc_set");

    DynamicArray<VkDescriptorBufferInfo> meshBufferInfo;
    meshBufferInfo.reserve(maxRenderableMeshes);
    for (uint32_t meshIdx = 0u; meshIdx < maxRenderableMeshes; ++meshIdx)
    {
        meshBufferInfo.push_back(m_rgResources.gbuffModelsBuffer.getDescriptorInfoAtIndex(meshIdx));
    }

    m_rgResources.gbuffModelDescriptorSet->configureBuffer(
        0,
        0,
        1,
        meshBufferInfo.data());

    m_rgResources.gbuffModelDescriptorSet->applyConfiguration();

    // create g-buff pipeline layout
    m_rgResources.gbuffPipelineLayout = VkGfxPipelineLayout::Builder(ctx)
                                            .addPushConstantRange(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(DrawData))
                                            .addDescriptorSetLayout(m_globalDescriptorSetLayout->layout)
                                            .addDescriptorSetLayout(m_materialDescriptorSetLayout->layout)
                                            .addDescriptorSetLayout(m_lightsSystem->getLightsDescriptorSetLayout().layout)
                                            .addDescriptorSetLayout(m_rgResources.gbuffModelDescriptorSetLayout->layout)

                                            .build();

    // create g-buffer pipeline
    std::filesystem::path buildPath  = STRING(DUSK_BUILD_PATH);
    std::filesystem::path shaderPath = buildPath / "shaders/";

    // load shaders modules
    auto vertShaderCode         = FileSystem::readFileBinary(shaderPath / "g_buffer.vert.spv");

    auto fragShaderCode         = FileSystem::readFileBinary(shaderPath / "g_buffer.frag.spv");

    m_rgResources.gbuffPipeline = VkGfxRenderPipeline::Builder(ctx)
                                      .setVertexShaderCode(vertShaderCode)
                                      .setFragmentShaderCode(fragShaderCode)
                                      .setPipelineLayout(*m_rgResources.gbuffPipelineLayout)
                                      .addColorAttachmentFormat(VK_FORMAT_R8G8B8A8_UNORM)      // albedo
                                      .addColorAttachmentFormat(VK_FORMAT_R16G16B16A16_SFLOAT) // normal
                                      .build();

    // presentation pass

    // input descriptor
    m_rgResources.presentTexDescriptorPool = VkGfxDescriptorPool::Builder(ctx)
                                                 .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1)
                                                 .setDebugName("present_desc_pool")
                                                 .build(1);

    m_rgResources.presentTexDescriptorSetLayout = VkGfxDescriptorSetLayout::Builder(ctx)
                                                      .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1)
                                                      .setDebugName("present_desc_set_layout")
                                                      .build();

    m_rgResources.presentTexDescriptorSet = m_rgResources.presentTexDescriptorPool->allocateDescriptorSet(
        *m_rgResources.presentTexDescriptorSetLayout,
        "present_desc_set");

    m_gfxDevice->createImageSampler(&m_rgResources.presentTexSampler);

    VkDescriptorImageInfo imageInfo {};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView   = m_rgResources.gbuffRenderTargets[0].imageView;
    imageInfo.sampler     = m_rgResources.presentTexSampler.sampler;

    m_rgResources.presentTexDescriptorSet->configureImage(
        0,
        0,
        1,
        &imageInfo);
    m_rgResources.presentTexDescriptorSet->applyConfiguration();

    // pipeline
    m_rgResources.presentPipelineLayout = VkGfxPipelineLayout::Builder(ctx)
                                              .addDescriptorSetLayout(m_rgResources.presentTexDescriptorSetLayout->layout)
                                              .build();

    auto presentVertShaderCode    = FileSystem::readFileBinary(shaderPath / "present.vert.spv");

    auto presentFragShaderCode    = FileSystem::readFileBinary(shaderPath / "present.frag.spv");

    m_rgResources.presentPipeline = VkGfxRenderPipeline::Builder(ctx)
                                        .setVertexShaderCode(presentVertShaderCode)
                                        .setFragmentShaderCode(presentFragShaderCode)
                                        .setPipelineLayout(*m_rgResources.presentPipelineLayout)
                                        .addColorAttachmentFormat(VK_FORMAT_B8G8R8A8_SRGB)
                                        .build();
}

void Engine::releaseRenderGraphResources()
{
    // release g-buffer resources
    for (auto& target : m_rgResources.gbuffRenderTargets)
    {
        m_gfxDevice->freeRenderTarget(target);
    }
    m_gfxDevice->freeRenderTarget(m_rgResources.gbuffDepthTexture);

    m_rgResources.gbuffPipeline       = nullptr;
    m_rgResources.gbuffPipelineLayout = nullptr;

    m_rgResources.gbuffModelDescriptorPool->resetPool();
    m_rgResources.gbuffModelDescriptorSetLayout = nullptr;
    m_rgResources.gbuffModelDescriptorPool      = nullptr;

    m_rgResources.gbuffModelsBuffer.free();

    // release present pass resources
    m_rgResources.presentPipeline       = nullptr;
    m_rgResources.presentPipelineLayout = nullptr;

    m_rgResources.presentTexDescriptorPool->resetPool();
    m_rgResources.presentTexDescriptorSetLayout = nullptr;
    m_rgResources.presentTexDescriptorPool      = nullptr;

    m_gfxDevice->freeImageSampler(&m_rgResources.presentTexSampler);
}

void Engine::setSkyboxVisibility(bool state)
{
    if (m_skyboxRenderSystem) m_skyboxRenderSystem->setVisble(state);
}

} // namespace dusk