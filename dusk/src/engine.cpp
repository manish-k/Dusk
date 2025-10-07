#include "engine.h"

#include "core/application.h"
#include "platform/window.h"
#include "platform/file_system.h"
#include "utils/utils.h"
#include "ui/editor_ui.h"
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
#include "renderer/sub_mesh.h"
#include "renderer/texture_db.h"
#include "renderer/environment.h"
#include "renderer/systems/basic_render_system.h"
#include "renderer/systems/grid_render_system.h"
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
    DUSK_PROFILE_FUNCTION;

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

    m_running   = true;
    m_paused    = false;

    m_textureDB = createUnique<TextureDB>(*m_gfxDevice);
    if (!m_textureDB->init())
    {
        DUSK_ERROR("Texture DB initialization failed");
        return false;
    }

    bool globalsStatus = setupGlobals();
    if (!globalsStatus) return false;

    m_lightsSystem      = createUnique<LightsSystem>();

    m_basicRenderSystem = createUnique<BasicRenderSystem>(
        *m_gfxDevice,
        *m_globalDescriptorSetLayout,
        *m_materialDescriptorSetLayout,
        m_lightsSystem->getLightsDescriptorSetLayout());

    prepareRenderGraphResources();
    executeBRDFLUTcomputePipeline();

    m_editorUI = createUnique<EditorUI>();
    if (!m_editorUI->init(*m_window))
    {
        DUSK_ERROR("Unable to init UI");
        return false;
    }

    m_environment = createUnique<Environment>(*m_textureDB);
    if (!m_environment->init(*m_globalDescriptorSetLayout))
    {
        DUSK_ERROR("Unable to init environment");
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

        m_textureDB->onUpdate();

        onUpdate(m_deltaTime);

        m_app->onUpdate(m_deltaTime);
    }
}

void Engine::stop() { m_running = false; }

void Engine::shutdown()
{
    m_basicRenderSystem = nullptr;
    m_gridRenderSystem  = nullptr;

    m_lightsSystem      = nullptr;

    m_environment->cleanup();
    m_environment = nullptr;

    m_editorUI->shutdown();
    m_editorUI = nullptr;

    cleanupGlobals();

    m_textureDB->cleanup();
    m_textureDB = nullptr;

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
            m_textureDB->getTexturesDescriptorSet().set,
            m_lightsSystem->getLightsDescriptorSet().set,
            m_materialsDescriptorSet->set
        };

        if (m_currentScene)
        {
            DUSK_PROFILE_SECTION("scene_updates");

            m_currentScene->onUpdate(dt);

            m_currentScene->updateModelsBuffer(m_rgResources.gbuffModelsBuffer[currentFrameIndex]);

            CameraComponent& camera = m_currentScene->getMainCamera();
            camera.setAspectRatio(m_renderer->getAspectRatio());

            GlobalUbo ubo {};
            ubo.view              = camera.viewMatrix;
            ubo.prjoection        = camera.projectionMatrix;
            ubo.inverseView       = camera.inverseViewMatrix;
            ubo.inverseProjection = camera.inverseProjectionMatrix;

            m_lightsSystem->updateLights(*m_currentScene, ubo);

            m_globalUbos.writeAndFlushAtIndex(currentFrameIndex, &ubo, sizeof(GlobalUbo));

            updateMaterialsBuffer(m_currentScene->getMaterials());
        }

        renderFrame(frameData);

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
    m_editorUI->onEvent(ev);

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
    registerMaterials(scene->getMaterials());

    m_lightsSystem->registerAllLights(*scene);
}

void Engine::renderFrame(FrameData& frameData)
{
    DUSK_PROFILE_FUNCTION;

    RenderGraph renderGraph;

    GfxTexture  swapImageTexture = m_renderer->getSwapChain().getCurrentSwapImageTexture();

    // create g-buffer pass
    GfxRenderingAttachment depthAttachment = GfxRenderingAttachment {
        .texture    = &m_textureDB->getTexture2D(m_rgResources.gbuffDepthTextureId),
        .clearValue = DEFAULT_DEPTH_STENCIL_VALUE,
        .loadOp     = GfxLoadOperation::Clear,
        .storeOp    = GfxStoreOperation::Store
    };

    auto gbuffCtx
        = VkGfxRenderPassContext {
              .writeColorAttachments = {
                  GfxRenderingAttachment {
                      .texture    = &m_textureDB->getTexture2D(m_rgResources.gbuffRenderTextureIds[0]),
                      .clearValue = DEFAULT_COLOR_CLEAR_VALUE,
                      .loadOp     = GfxLoadOperation::Clear,
                      .storeOp    = GfxStoreOperation::Store },
                  GfxRenderingAttachment {
                      .texture    = &m_textureDB->getTexture2D(m_rgResources.gbuffRenderTextureIds[1]),
                      .clearValue = DEFAULT_COLOR_CLEAR_VALUE,
                      .loadOp     = GfxLoadOperation::Clear,
                      .storeOp    = GfxStoreOperation::Store },
                  GfxRenderingAttachment {
                      .texture    = &m_textureDB->getTexture2D(m_rgResources.gbuffRenderTextureIds[2]),
                      .clearValue = DEFAULT_COLOR_CLEAR_VALUE,
                      .loadOp     = GfxLoadOperation::Clear,
                      .storeOp    = GfxStoreOperation::Store },
                  GfxRenderingAttachment {
                      .texture    = &m_textureDB->getTexture2D(m_rgResources.gbuffRenderTextureIds[3]),
                      .clearValue = DEFAULT_COLOR_CLEAR_VALUE,
                      .loadOp     = GfxLoadOperation::Clear,
                      .storeOp    = GfxStoreOperation::Store } },
              .depthAttachment     = depthAttachment,
              .useDepth            = true,
              .maxParallelism      = 1,
              .secondaryCmdBuffers = m_renderer->getSecondayCmdBuffers(frameData.frameIndex)
          };

    renderGraph.setPassContext("gbuffer_pass", gbuffCtx);
    renderGraph.addPass("gbuffer_pass", recordGBufferCmds);

    // create lighting pass
    auto lightingPassReadAttachments = {
        // color attachment
        GfxRenderingAttachment {
            .texture    = &m_textureDB->getTexture2D(m_rgResources.gbuffRenderTextureIds[0]),
            .clearValue = DEFAULT_COLOR_CLEAR_VALUE,
            .loadOp     = GfxLoadOperation::Clear,
            .storeOp    = GfxStoreOperation::Store },
        // normal attachment
        GfxRenderingAttachment {
            .texture    = &m_textureDB->getTexture2D(m_rgResources.gbuffRenderTextureIds[1]),
            .clearValue = DEFAULT_COLOR_CLEAR_VALUE,
            .loadOp     = GfxLoadOperation::Clear,
            .storeOp    = GfxStoreOperation::Store },
        // ao-metallic-roughness attachment
        GfxRenderingAttachment {
            .texture    = &m_textureDB->getTexture2D(m_rgResources.gbuffRenderTextureIds[2]),
            .clearValue = DEFAULT_COLOR_CLEAR_VALUE,
            .loadOp     = GfxLoadOperation::Clear,
            .storeOp    = GfxStoreOperation::Store },
        // emissive color
        GfxRenderingAttachment {
            .texture    = &m_textureDB->getTexture2D(m_rgResources.gbuffRenderTextureIds[3]),
            .clearValue = DEFAULT_COLOR_CLEAR_VALUE,
            .loadOp     = GfxLoadOperation::Clear,
            .storeOp    = GfxStoreOperation::Store },
        // depth attachment
        GfxRenderingAttachment {
            .texture    = &m_textureDB->getTexture2D(m_rgResources.gbuffDepthTextureId),
            .clearValue = DEFAULT_DEPTH_STENCIL_VALUE,
            .loadOp     = GfxLoadOperation::Clear,
            .storeOp    = GfxStoreOperation::Store },
        // irradiance map
        GfxRenderingAttachment {
            .texture    = &m_textureDB->getTexture2D(m_environment->getSkyIrradianceTextureId()),
            .clearValue = DEFAULT_COLOR_CLEAR_VALUE,
            .loadOp     = GfxLoadOperation::Clear,
            .storeOp    = GfxStoreOperation::Store },
        // raidance map
        GfxRenderingAttachment {
            .texture    = &m_textureDB->getTexture2D(m_environment->getSkyRadianceTextureId()),
            .clearValue = DEFAULT_COLOR_CLEAR_VALUE,
            .loadOp     = GfxLoadOperation::Clear,
            .storeOp    = GfxStoreOperation::Store },
        // brdf lut map
        GfxRenderingAttachment {
            .texture    = &m_textureDB->getTexture2D(m_rgResources.brdfLUTextureId),
            .clearValue = DEFAULT_COLOR_CLEAR_VALUE,
            .loadOp     = GfxLoadOperation::Clear,
            .storeOp    = GfxStoreOperation::Store }
    };
    auto lighingPassWriteAttachments = {
        GfxRenderingAttachment {
            .texture    = &m_textureDB->getTexture2D(m_rgResources.lightingRenderTextureId),
            .clearValue = DEFAULT_COLOR_CLEAR_VALUE,
            .loadOp     = GfxLoadOperation::Clear,
            .storeOp    = GfxStoreOperation::Store }
    };

    auto lightingCtx = VkGfxRenderPassContext {
        .readAttachments       = lightingPassReadAttachments,
        .writeColorAttachments = lighingPassWriteAttachments,
        .useDepth              = false,
        .secondaryCmdBuffers   = m_renderer->getSecondayCmdBuffers(frameData.frameIndex)
    };

    renderGraph.setPassContext("lighting_pass", lightingCtx);
    renderGraph.addPass("lighting_pass", recordLightingCmds);

    // create skybox pass
    auto skyBoxPassWriteAttachments = {
        GfxRenderingAttachment {
            .texture    = &m_textureDB->getTexture2D(m_rgResources.lightingRenderTextureId),
            .clearValue = DEFAULT_COLOR_CLEAR_VALUE,
            .loadOp     = GfxLoadOperation::Load,
            .storeOp    = GfxStoreOperation::Store }
    };
    GfxRenderingAttachment skyDepthAttachment = GfxRenderingAttachment {
        .texture    = &m_textureDB->getTexture2D(m_rgResources.gbuffDepthTextureId),
        .clearValue = DEFAULT_DEPTH_STENCIL_VALUE,
        .loadOp     = GfxLoadOperation::Load,
        .storeOp    = GfxStoreOperation::Store
    };

    auto skyBoxCtx = VkGfxRenderPassContext {
        .readAttachments       = {},
        .writeColorAttachments = skyBoxPassWriteAttachments,
        .depthAttachment       = skyDepthAttachment,
        .useDepth              = true,
        .secondaryCmdBuffers   = m_renderer->getSecondayCmdBuffers(frameData.frameIndex)
    };

    renderGraph.setPassContext("skybox_pass", skyBoxCtx);
    renderGraph.addPass("skybox_pass", recordSkyBoxCmds);

    // create presentation pass
    auto presentReadAttachments = {
        GfxRenderingAttachment {
            .texture    = &m_textureDB->getTexture2D(m_rgResources.lightingRenderTextureId),
            .clearValue = DEFAULT_COLOR_CLEAR_VALUE,
            .loadOp     = GfxLoadOperation::Clear,
            .storeOp    = GfxStoreOperation::Store }
    };

    auto presentWriteAttachments = {
        GfxRenderingAttachment {
            .texture    = &swapImageTexture,
            .clearValue = DEFAULT_COLOR_CLEAR_VALUE,
            .loadOp     = GfxLoadOperation::Clear,
            .storeOp    = GfxStoreOperation::Store }
    };

    auto presentCtx = VkGfxRenderPassContext {
        .readAttachments       = presentReadAttachments,
        .writeColorAttachments = presentWriteAttachments,
        .useDepth              = false,
        .secondaryCmdBuffers   = m_renderer->getSecondayCmdBuffers(frameData.frameIndex)
    };

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
    DUSK_PROFILE_FUNCTION;

    auto&    ctx            = VkGfxDevice::getSharedVulkanContext();
    auto     extent         = m_renderer->getSwapChain().getCurrentExtent();
    uint32_t maxFramesCount = m_renderer->getMaxFramesCount();

    // g-buffer resources
    // Allocate g-buffer render targets
    m_rgResources.gbuffRenderTextureIds.push_back(m_textureDB->createColorTexture(
        "gbuffer_pass_albedo",
        extent.width,
        extent.height,
        VK_FORMAT_R32G32B32A32_SFLOAT));
    m_rgResources.gbuffRenderTextureIds.push_back(m_textureDB->createColorTexture(
        "gbuffer_pass_normal",
        extent.width,
        extent.height,
        VK_FORMAT_R16G16B16A16_SFLOAT));

    m_rgResources.gbuffRenderTextureIds.push_back(m_textureDB->createColorTexture(
        "gbuffer_pass_occlu_rough_metal",
        extent.width,
        extent.height,
        VK_FORMAT_R16G16B16A16_SFLOAT));

    m_rgResources.gbuffRenderTextureIds.push_back(m_textureDB->createColorTexture(
        "gbuffer_pass_emissive_color",
        extent.width,
        extent.height,
        VK_FORMAT_R16G16B16A16_SFLOAT));

    // Allocate g-buffer depth texture
    m_rgResources.gbuffDepthTextureId = m_textureDB->createDepthTexture(
        "gbuffer_pass_depth",
        extent.width,
        extent.height,
        VK_FORMAT_D32_SFLOAT_S8_UINT);

    m_rgResources.gbuffModelDescriptorPool = VkGfxDescriptorPool::Builder(ctx)
                                                 .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, maxFramesCount * maxRenderableMeshes)
                                                 .setDebugName("model_desc_pool")
                                                 .build(maxFramesCount, VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT);

    m_rgResources.gbuffModelDescriptorSetLayout = VkGfxDescriptorSetLayout::Builder(ctx)
                                                      .addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, maxRenderableMeshes, true)
                                                      .setDebugName("model_desc_set_layout")
                                                      .build();

    // create models buffer and descriptor set
    m_rgResources.gbuffModelsBuffer.resize(maxFramesCount);
    m_rgResources.gbuffModelDescriptorSet.resize(maxFramesCount);

    for (uint32_t frameIdx = 0u; frameIdx < maxFramesCount; ++frameIdx)
    {
        GfxBuffer::createHostWriteBuffer(
            GfxBufferUsageFlags::StorageBuffer,
            sizeof(ModelData),
            maxRenderableMeshes,
            "model_buffer_" + std::to_string(frameIdx),
            &m_rgResources.gbuffModelsBuffer[frameIdx]);

        m_rgResources.gbuffModelDescriptorSet[frameIdx] = m_rgResources.gbuffModelDescriptorPool->allocateDescriptorSet(
            *m_rgResources.gbuffModelDescriptorSetLayout, "model_desc_set");

        DynamicArray<VkDescriptorBufferInfo> meshBufferInfo;
        meshBufferInfo.reserve(maxRenderableMeshes);
        for (uint32_t meshIdx = 0u; meshIdx < maxRenderableMeshes; ++meshIdx)
        {
            meshBufferInfo.push_back(m_rgResources.gbuffModelsBuffer[frameIdx].getDescriptorInfoAtIndex(meshIdx));
        }

        m_rgResources.gbuffModelDescriptorSet[frameIdx]->configureBuffer(
            0,
            0,
            meshBufferInfo.size(),
            meshBufferInfo.data());

        m_rgResources.gbuffModelDescriptorSet[frameIdx]->applyConfiguration();
    }

    // create g-buff pipeline layout
    m_rgResources.gbuffPipelineLayout = VkGfxPipelineLayout::Builder(ctx)
                                            .addPushConstantRange(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(DrawData))
                                            .addDescriptorSetLayout(m_globalDescriptorSetLayout->layout)
                                            .addDescriptorSetLayout(m_materialDescriptorSetLayout->layout)
                                            .addDescriptorSetLayout(m_rgResources.gbuffModelDescriptorSetLayout->layout)
                                            .addDescriptorSetLayout(m_textureDB->getTexturesDescriptorSetLayout().layout)
                                            .build();

#ifdef VK_RENDERER_DEBUG
    vkdebug::setObjectName(
        ctx.device,
        VK_OBJECT_TYPE_PIPELINE_LAYOUT,
        (uint64_t)m_rgResources.gbuffPipelineLayout->get(),
        "gbuff_pipeline_layout");
#endif // VK_RENDERER_DEBUG

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
                                      .addColorAttachmentFormat(VK_FORMAT_R32G32B32A32_SFLOAT) // albedo
                                      .addColorAttachmentFormat(VK_FORMAT_R16G16B16A16_SFLOAT) // normal
                                      .addColorAttachmentFormat(VK_FORMAT_R16G16B16A16_SFLOAT) // ao-roughness-metallic
                                      .addColorAttachmentFormat(VK_FORMAT_R16G16B16A16_SFLOAT) // emissive color
                                      .build();

#ifdef VK_RENDERER_DEBUG
    vkdebug::setObjectName(
        ctx.device,
        VK_OBJECT_TYPE_PIPELINE,
        (uint64_t)m_rgResources.gbuffPipeline->get(),
        "gbuff_pipeline");
#endif // VK_RENDERER_DEBUG

    // presentation pass
    m_rgResources.presentPipelineLayout = VkGfxPipelineLayout::Builder(ctx)
                                              .addPushConstantRange(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PresentationPushConstant))
                                              .addDescriptorSetLayout(m_textureDB->getTexturesDescriptorSetLayout().layout)
                                              .build();

#ifdef VK_RENDERER_DEBUG
    vkdebug::setObjectName(
        ctx.device,
        VK_OBJECT_TYPE_PIPELINE_LAYOUT,
        (uint64_t)m_rgResources.presentPipelineLayout->get(),
        "presentation_pipeline_layout");
#endif // VK_RENDERER_DEBUG

    auto presentVertShaderCode    = FileSystem::readFileBinary(shaderPath / "present.vert.spv");

    auto presentFragShaderCode    = FileSystem::readFileBinary(shaderPath / "present.frag.spv");

    m_rgResources.presentPipeline = VkGfxRenderPipeline::Builder(ctx)
                                        .setVertexShaderCode(presentVertShaderCode)
                                        .setFragmentShaderCode(presentFragShaderCode)
                                        .setPipelineLayout(*m_rgResources.presentPipelineLayout)
                                        .addColorAttachmentFormat(VK_FORMAT_B8G8R8A8_SRGB)
                                        .removeVertexInputState()
                                        .build();

#ifdef VK_RENDERER_DEBUG
    vkdebug::setObjectName(
        ctx.device,
        VK_OBJECT_TYPE_PIPELINE,
        (uint64_t)m_rgResources.presentPipeline->get(),
        "presentation_pipeline");
#endif // VK_RENDERER_DEBUG

    // lighting pass
    m_rgResources.lightingRenderTextureId = m_textureDB->createColorTexture(
        "light_pass_color",
        extent.width,
        extent.height,
        VK_FORMAT_R8G8B8A8_SRGB);

    m_rgResources.lightingPipelineLayout = VkGfxPipelineLayout::Builder(ctx)
                                               .addPushConstantRange(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(LightingPushConstant))
                                               .addDescriptorSetLayout(m_globalDescriptorSetLayout->layout)
                                               .addDescriptorSetLayout(m_textureDB->getTexturesDescriptorSetLayout().layout)
                                               .addDescriptorSetLayout(m_lightsSystem->getLightsDescriptorSetLayout().layout)
                                               .build();

#ifdef VK_RENDERER_DEBUG
    vkdebug::setObjectName(
        ctx.device,
        VK_OBJECT_TYPE_PIPELINE_LAYOUT,
        (uint64_t)m_rgResources.lightingPipelineLayout->get(),
        "lighting_pipeline_layout");
#endif // VK_RENDERER_DEBUG

    auto lightingVertShaderCode    = FileSystem::readFileBinary(shaderPath / "lighting.vert.spv");

    auto lightingFragShaderCode    = FileSystem::readFileBinary(shaderPath / "lighting.frag.spv");

    m_rgResources.lightingPipeline = VkGfxRenderPipeline::Builder(ctx)
                                         .setVertexShaderCode(lightingVertShaderCode)
                                         .setFragmentShaderCode(lightingFragShaderCode)
                                         .setPipelineLayout(*m_rgResources.lightingPipelineLayout)
                                         .addColorAttachmentFormat(VK_FORMAT_R8G8B8A8_SRGB)
                                         .removeVertexInputState()
                                         .build();
#ifdef VK_RENDERER_DEBUG
    vkdebug::setObjectName(
        ctx.device,
        VK_OBJECT_TYPE_PIPELINE,
        (uint64_t)m_rgResources.lightingPipeline->get(),
        "lighting_pipeline");
#endif // VK_RENDERER_DEBUG

    // brdf lut pipeline
    m_rgResources.brdfLUTextureId = m_textureDB->createStorageTexture(
        "brdf_lut_tex",
        512,
        512,
        VK_FORMAT_R32G32_SFLOAT);

    VkSampler           brdfSampler;
    VkSamplerCreateInfo samplerInfo {};
    samplerInfo.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter               = VK_FILTER_LINEAR;
    samplerInfo.minFilter               = VK_FILTER_LINEAR;
    samplerInfo.addressModeU            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.anisotropyEnable        = VK_FALSE;
    samplerInfo.maxAnisotropy           = ctx.physicalDeviceProperties.limits.maxSamplerAnisotropy;
    samplerInfo.borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable           = VK_FALSE;
    samplerInfo.compareOp               = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR;

    vkCreateSampler(ctx.device, &samplerInfo, nullptr, &brdfSampler);
    m_textureDB->updateTextureSampler(m_rgResources.brdfLUTextureId, brdfSampler);

    m_rgResources.brdfLUTPipelineLayout = VkGfxPipelineLayout::Builder(ctx)
                                              .addPushConstantRange(VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(BRDFLUTPushConstant))
                                              .addDescriptorSetLayout(m_textureDB->getStorageTexturesDescriptorSetLayout().layout)
                                              .build();

#ifdef VK_RENDERER_DEBUG
    vkdebug::setObjectName(
        ctx.device,
        VK_OBJECT_TYPE_PIPELINE_LAYOUT,
        (uint64_t)m_rgResources.brdfLUTPipelineLayout->get(),
        "brdf_lut_pipeline_layout");
#endif // VK_RENDERER_DEBUG

    auto brdfLUTCompShader        = FileSystem::readFileBinary(shaderPath / "brdf_lut.comp.spv");

    m_rgResources.brdfLUTPipeline = VkGfxComputePipeline::Builder(ctx)
                                        .setComputeShaderCode(brdfLUTCompShader)
                                        .setPipelineLayout(*m_rgResources.brdfLUTPipelineLayout)
                                        .build();
#ifdef VK_RENDERER_DEBUG
    vkdebug::setObjectName(
        ctx.device,
        VK_OBJECT_TYPE_PIPELINE,
        (uint64_t)m_rgResources.brdfLUTPipeline->get(),
        "brdf_lut_pipeline");
#endif // VK_RENDERER_DEBUG
}

void Engine::releaseRenderGraphResources()
{
    m_rgResources.gbuffPipeline       = nullptr;
    m_rgResources.gbuffPipelineLayout = nullptr;

    m_rgResources.gbuffModelDescriptorPool->resetPool();
    m_rgResources.gbuffModelDescriptorSetLayout = nullptr;
    m_rgResources.gbuffModelDescriptorPool      = nullptr;

    for (auto& buffer : m_rgResources.gbuffModelsBuffer)
        buffer.free();

    // release present pass resources
    m_rgResources.presentPipeline       = nullptr;
    m_rgResources.presentPipelineLayout = nullptr;

    // release lighting pass resources
    m_rgResources.lightingPipeline       = nullptr;
    m_rgResources.lightingPipelineLayout = nullptr;

    // release brdf lut pipeline resources
    m_rgResources.brdfLUTPipeline       = nullptr;
    m_rgResources.brdfLUTPipelineLayout = nullptr;
}

void Engine::executeBRDFLUTcomputePipeline()
{
    VulkanContext               ctx = VkGfxDevice::getSharedVulkanContext();

    VkCommandBufferAllocateInfo allocInfo {};
    allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool        = ctx.commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(ctx.device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    auto& image = m_textureDB->getTexture2D(m_rgResources.brdfLUTextureId);
    image.recordTransitionLayout(commandBuffer, VK_IMAGE_LAYOUT_GENERAL);

    // Bind pipeline and descriptor sets
    m_rgResources.brdfLUTPipeline->bind(commandBuffer);
    vkCmdBindDescriptorSets(
        commandBuffer,
        VK_PIPELINE_BIND_POINT_COMPUTE,
        m_rgResources.brdfLUTPipelineLayout->get(),
        0,
        1,
        &m_textureDB->getStorageTexturesDescriptorSet().set,
        0,
        nullptr);

    BRDFLUTPushConstant push {};
    push.lutTextureIdx = m_rgResources.brdfLUTextureId;

    vkCmdPushConstants(
        commandBuffer,
        m_rgResources.brdfLUTPipelineLayout->get(),
        VK_SHADER_STAGE_COMPUTE_BIT,
        0,
        sizeof(BRDFLUTPushConstant),
        &push);

    // Dispatch compute work
    vkCmdDispatch(commandBuffer, 32, 32, 1);

    vkEndCommandBuffer(commandBuffer);

    // Submit command buffer
    VkSubmitInfo submitInfo {};
    submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers    = &commandBuffer;

    // Currently submitting to graphics queue
    // For exclusive compute queue usage we need compute queue specific
    // command pool, command buffer and queue ownership transfer barriers
    vkQueueSubmit(ctx.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(ctx.graphicsQueue);

    vkFreeCommandBuffers(ctx.device, ctx.commandPool, 1, &commandBuffer);
}

} // namespace dusk