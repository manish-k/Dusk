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
#include "renderer/texture_db.h"
#include "renderer/environment.h"
#include "renderer/systems/basic_render_system.h"
#include "renderer/systems/lights_system.h"
#include "renderer/render_graph.h"
#include "renderer/passes/render_passes.h"
#include "renderer/geometry/frustum.h"

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
    // executeBRDFLUTcomputePipeline();

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

        m_app->onUpdate(m_deltaTime);

        onUpdate(m_deltaTime);
    }
}

void Engine::stop() { m_running = false; }

void Engine::shutdown()
{
    m_basicRenderSystem = nullptr;

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

    Registry::cleanup();
}

void Engine::onUpdate(TimeStep dt)
{
    DUSK_PROFILE_FUNCTION;

    uint32_t currentFrameIndex = m_renderer->getCurrentFrameIndex();

    if (VkCommandBuffer commandBuffer = m_renderer->beginFrame())
    {
        DUSK_PROFILE_SECTION("frame_update");

        auto      extent = m_renderer->getSwapChain().getCurrentExtent();

        FrameData frameData {
            currentFrameIndex,
            dt,
            commandBuffer,
            m_currentScene,
            extent.width,
            extent.height,
            &m_frameRenderables[currentFrameIndex],
            m_globalDescriptorSet->set,
            m_textureDB->getTexturesDescriptorSet().set,
            m_lightsSystem->getLightsDescriptorSet().set,
            m_materialsDescriptorSet->set,
            m_meshDataDescriptorSet->set,
            m_renderableDescriptorSets[currentFrameIndex]->set
        };

        if (m_currentScene)
        {
            DUSK_PROFILE_SECTION("scene_updates");

            m_currentScene->onUpdate(dt);

            m_currentScene->gatherRenderables(&m_frameRenderables[currentFrameIndex]);

            CameraComponent& camera = m_currentScene->getMainCamera();
            camera.setAspectRatio(m_renderer->getAspectRatio());

            Frustum cameraFrustum = extractFrustumFromMatrix(
                camera.projectionMatrix * camera.viewMatrix);

            GlobalUbo ubo {};
            ubo.view              = camera.viewMatrix;
            ubo.prjoection        = camera.projectionMatrix;
            ubo.inverseView       = glm::inverse(camera.viewMatrix);
            ubo.inverseProjection = camera.inverseProjectionMatrix;

            ubo.frustumPlanes[0]  = cameraFrustum.left.toVec4();
            ubo.frustumPlanes[1]  = cameraFrustum.right.toVec4();
            ubo.frustumPlanes[2]  = cameraFrustum.bottom.toVec4();
            ubo.frustumPlanes[3]  = cameraFrustum.top.toVec4();
            ubo.frustumPlanes[4]  = cameraFrustum.near.toVec4();
            ubo.frustumPlanes[5]  = cameraFrustum.far.toVec4();

            m_lightsSystem->updateLights(*m_currentScene, ubo);

            m_globalUbos.writeAndFlushAtIndex(currentFrameIndex, &ubo, sizeof(GlobalUbo));

            // write all renderables data
            m_modelMatrixBuffers[currentFrameIndex].writeAndFlush(
                0,
                m_frameRenderables[currentFrameIndex].modelMatrices.data(),
                m_frameRenderables[currentFrameIndex].modelMatrices.size() * sizeof(glm::mat4));
            m_normalMatrixBuffers[currentFrameIndex].writeAndFlush(
                0,
                m_frameRenderables[currentFrameIndex].normalMatrices.data(),
                m_frameRenderables[currentFrameIndex].normalMatrices.size() * sizeof(glm::mat4));
            m_boundingBoxBuffers[currentFrameIndex].writeAndFlush(
                0,
                m_frameRenderables[currentFrameIndex].boundingBoxes.data(),
                m_frameRenderables[currentFrameIndex].boundingBoxes.size() * sizeof(GfxBoundingBoxData));
            m_meshIdsBuffers[currentFrameIndex].writeAndFlush(
                0,
                m_frameRenderables[currentFrameIndex].meshIds.data(),
                m_frameRenderables[currentFrameIndex].meshIds.size() * sizeof(uint32_t));
            m_materialIdsBuffers[currentFrameIndex].writeAndFlush(
                0,
                m_frameRenderables[currentFrameIndex].materialIds.data(),
                m_frameRenderables[currentFrameIndex].materialIds.size() * sizeof(uint32_t));

            updateMaterialsBuffer(m_currentScene->getMaterials());
        }

        renderFrame(frameData);

        m_renderer->endFrame();

        m_frameRenderables[currentFrameIndex].modelMatrices.clear();
        m_frameRenderables[currentFrameIndex].normalMatrices.clear();
        m_frameRenderables[currentFrameIndex].boundingBoxes.clear();
        m_frameRenderables[currentFrameIndex].meshIds.clear();
        m_frameRenderables[currentFrameIndex].materialIds.clear();
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

    {
        DUSK_PROFILE_SECTION("mesh_data_buffer_update");
        // TODO: do something about public m_sceneMeshes access
        auto meshDataCount = static_cast<uint32_t>(scene->m_sceneMeshes.size());

        m_meshDataBuffer.writeAndFlush(0, scene->m_sceneMeshes.data(), meshDataCount * sizeof(GfxMeshData));

        // update descset
        DynamicArray<VkDescriptorBufferInfo> meshDataDescInfo;
        meshDataDescInfo.push_back(m_meshDataBuffer.getDescriptorInfo());

        m_meshDataDescriptorSet->configureBuffer(
            0,
            0,
            meshDataDescInfo.size(),
            meshDataDescInfo.data());
        m_meshDataDescriptorSet->applyConfiguration();
    }

    m_lightsSystem->registerAllLights(*scene);
}

void Engine::renderFrame(FrameData& frameData)
{
    DUSK_PROFILE_FUNCTION;

    RenderGraph renderGraph;

    GfxTexture  swapImageTexture = m_renderer->getSwapChain().getCurrentSwapImageTexture();

    // create rg resources
    RGImageResource swapImage = {
        .name    = "swap_image",
        .texture = &swapImageTexture
    };
    RGImageResource dirShadowMap = {
        .name    = "dir_shadow_map",
        .texture = &m_textureDB->getTexture2D(m_rgResources.dirShadowMapsTextureId)
    };
    RGImageResource gbuffDepth = {
        .name    = "gbuff_depth",
        .texture = &m_textureDB->getTexture2D(m_rgResources.gbuffDepthTextureId)
    };
    RGImageResource gbuffAlbedo = {
        .name    = "gbuff_albedo",
        .texture = &m_textureDB->getTexture2D(m_rgResources.gbuffRenderTextureIds[0])
    };
    RGImageResource gbuffNormal = {
        .name    = "gbuff_normal",
        .texture = &m_textureDB->getTexture2D(m_rgResources.gbuffRenderTextureIds[1])
    };
    RGImageResource gbuffAoMR = {
        .name    = "gbuff_ao_metallic_roughness",
        .texture = &m_textureDB->getTexture2D(m_rgResources.gbuffRenderTextureIds[2])
    };
    RGImageResource gbuffEmissive = {
        .name    = "gbuff_emissive",
        .texture = &m_textureDB->getTexture2D(m_rgResources.gbuffRenderTextureIds[3])
    };
    RGImageResource brdfLUT = {
        .name    = "brdf_lut",
        .texture = &m_textureDB->getTexture2D(m_rgResources.brdfLUTextureId)
    };
    RGImageResource irradianceCubemap = {
        .name    = "irradiance_environment_cubemap",
        .texture = &m_textureDB->getTexture2D(m_environment->getSkyIrradianceTextureId())
    };
    RGImageResource prefilteredEnvironmentMap = {
        .name    = "prefiltered_environment_map",
        .texture = &m_textureDB->getTexture2D(m_environment->getSkyPrefilteredTextureId())
    };
    RGImageResource lightingOutput = {
        .name    = "lighting_output",
        .texture = &m_textureDB->getTexture2D(m_rgResources.lightingRenderTextureId)
    };

    RGBufferResource indirectDrawCommandsBuffer = {
        .name   = "indirect_draw_commands_buffer",
        .buffer = &m_rgResources.frameIndirectDrawCommandsBuffers[frameData.frameIndex]
    };
    RGBufferResource indirectDrawCountBuffer = {
        .name   = "indirect_draw_count_buffer",
        .buffer = &m_rgResources.frameIndirectDrawCountBuffers[frameData.frameIndex]
    };

    auto cullPassId = renderGraph.addPass("cull_lod_pass", dispatchIndirectDrawCompute);
    renderGraph.addWriteResource(cullPassId, indirectDrawCommandsBuffer);
    renderGraph.addWriteResource(cullPassId, indirectDrawCountBuffer);
    renderGraph.addReadResource(cullPassId, indirectDrawCountBuffer);
    renderGraph.markAsCompute(cullPassId);

    // create shadow pass
    uint32_t dirLightsCount  = m_lightsSystem->getDirectionalLightsCount();
    uint32_t dirShadowMapVer = 0u;

    if (dirLightsCount > 0)
    {
        auto shadowPassId = renderGraph.addPass("dir_shadow_pass", recordShadow2DMapsCmds);

        renderGraph.addDepthResource(shadowPassId, dirShadowMap);

        dirShadowMapVer = renderGraph.addWriteResource(shadowPassId, dirShadowMap);
    }

    // create g-buffer pass
    auto gbuffPassId = renderGraph.addPass("gbuffer_pass", recordGBufferCmds);
    renderGraph.addDepthResource(gbuffPassId, gbuffDepth);

    renderGraph.addReadResource(gbuffPassId, indirectDrawCommandsBuffer);
    renderGraph.addReadResource(gbuffPassId, indirectDrawCountBuffer);

    uint32_t gbuffAlbedoVer   = renderGraph.addWriteResource(gbuffPassId, gbuffAlbedo);
    uint32_t gbuffNormalVer   = renderGraph.addWriteResource(gbuffPassId, gbuffNormal);
    uint32_t gbuffAoMRVer     = renderGraph.addWriteResource(gbuffPassId, gbuffAoMR);
    uint32_t gbuffEmissiveVer = renderGraph.addWriteResource(gbuffPassId, gbuffEmissive);
    uint32_t gbuffDepthVer    = renderGraph.addWriteResource(gbuffPassId, gbuffDepth);

    // create lighting pass
    auto lightPassId = renderGraph.addPass("lighting_pass", recordLightingCmds);
    renderGraph.addReadResource(lightPassId, gbuffAlbedo, gbuffAlbedoVer);
    renderGraph.addReadResource(lightPassId, gbuffNormal, gbuffNormalVer);
    renderGraph.addReadResource(lightPassId, gbuffAoMR, gbuffAoMRVer);
    renderGraph.addReadResource(lightPassId, gbuffEmissive, gbuffEmissiveVer);
    renderGraph.addReadResource(lightPassId, gbuffDepth, gbuffDepthVer);
    renderGraph.addReadResource(lightPassId, irradianceCubemap);
    renderGraph.addReadResource(lightPassId, prefilteredEnvironmentMap);
    renderGraph.addReadResource(lightPassId, brdfLUT);
    renderGraph.addReadResource(lightPassId, dirShadowMap, dirShadowMapVer);

    uint32_t lightOutputVer = renderGraph.addWriteResource(lightPassId, lightingOutput);

    // create skybox pass
    auto skyPassId = renderGraph.addPass("skybox_pass", recordSkyBoxCmds);
    renderGraph.addDepthResource(skyPassId, gbuffDepth);

    renderGraph.addReadResource(skyPassId, gbuffDepth, gbuffDepthVer);
    renderGraph.addReadResource(skyPassId, lightingOutput, lightOutputVer);

    uint32_t skyOutputVer = renderGraph.addWriteResource(skyPassId, lightingOutput);
    renderGraph.addWriteResource(skyPassId, gbuffDepth);

    // create presentation pass
    auto presentPassId = renderGraph.addPass("present_pass", recordPresentationCmds);
    renderGraph.addReadResource(presentPassId, lightingOutput, skyOutputVer);
    renderGraph.addWriteResource(presentPassId, swapImage);

    // execute render graph
    renderGraph.execute(frameData);
}

bool Engine::setupGlobals()
{
    uint32_t      maxFramesCount = m_renderer->getMaxFramesCount();
    VulkanContext ctx            = VkGfxDevice::getSharedVulkanContext();

    // setup 256mb vertex buffer * 128mb index buffer
    m_vertexBuffer.init(
        GfxBufferUsageFlags::VertexBuffer | GfxBufferUsageFlags::TransferTarget,
        256 * 1024 * 1024,
        GfxBufferMemoryTypeFlags::DedicatedDeviceMemory,
        "global_vertex_buffer");

    CHECK_AND_RETURN_FALSE(!m_vertexBuffer.isAllocated());

    m_indexBuffer.init(
        GfxBufferUsageFlags::IndexBuffer | GfxBufferUsageFlags::TransferTarget,
        128 * 1024 * 1024,
        GfxBufferMemoryTypeFlags::DedicatedDeviceMemory,
        "global_index_buffer");

    CHECK_AND_RETURN_FALSE(!m_indexBuffer.isAllocated())

    // create global descriptor pool
    m_globalDescriptorPool = VkGfxDescriptorPool::Builder(ctx)
                                 .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, maxFramesCount)
                                 .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 100) // TODO: make count configurable
                                 .setDebugName("global_desc_pool")
                                 .build(1, VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT);
    CHECK_AND_RETURN_FALSE(!m_globalDescriptorPool);

    m_globalDescriptorSetLayout = VkGfxDescriptorSetLayout::Builder(ctx)
                                      .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS | VK_SHADER_STAGE_COMPUTE_BIT, maxFramesCount, true)
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
                                   .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, MAX_MATERIALS_COUNT) // TODO: make count configurable
                                   .setDebugName("material_desc_pool")
                                   .build(1, VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT);
    CHECK_AND_RETURN_FALSE(!m_materialDescriptorPool);

    m_materialDescriptorSetLayout = VkGfxDescriptorSetLayout::Builder(ctx)
                                        .addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, MAX_MATERIALS_COUNT, true) // TODO: make count configurable
                                        .setDebugName("material_desc_set_layout")
                                        .build();
    CHECK_AND_RETURN_FALSE(!m_materialDescriptorSetLayout);

    // create storage buffer of storing materials;
    GfxBuffer::createHostWriteBuffer(
        GfxBufferUsageFlags::StorageBuffer,
        sizeof(Material),
        MAX_MATERIALS_COUNT,
        "material_buffer",
        &m_materialsBuffer);
    CHECK_AND_RETURN_FALSE(!m_materialsBuffer.isAllocated());

    m_materialsDescriptorSet = m_materialDescriptorPool->allocateDescriptorSet(*m_materialDescriptorSetLayout, "material_desc_set");
    CHECK_AND_RETURN_FALSE(!m_materialsDescriptorSet);

    // mesh info resources
    m_meshDataDescriptorPool = VkGfxDescriptorPool::Builder(ctx)
                                   .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, MAX_RENDERABLES_COUNT) // TODO: make count configurable
                                   .setDebugName("mesh_data_desc_pool")
                                   .build(1, VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT);
    CHECK_AND_RETURN_FALSE(!m_meshDataDescriptorPool);

    m_meshDataDescriptorSetLayout = VkGfxDescriptorSetLayout::Builder(ctx)
                                        .addBinding(
                                            0,
                                            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                            VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT,
                                            1,
                                            true)
                                        .setDebugName("mesh_data_desc_set_layout")
                                        .build();
    CHECK_AND_RETURN_FALSE(!m_meshDataDescriptorSetLayout);

    // TODO: make it device local buffer
    GfxBuffer::createHostWriteBuffer(
        GfxBufferUsageFlags::StorageBuffer,
        sizeof(GfxMeshData) * MAX_RENDERABLES_COUNT,
        1,
        "mesh_data_buffer",
        &m_meshDataBuffer);
    CHECK_AND_RETURN_FALSE(!m_meshDataBuffer.isAllocated());

    m_meshDataDescriptorSet = m_meshDataDescriptorPool->allocateDescriptorSet(*m_meshDataDescriptorSetLayout, "mesh_data_desc_set");
    CHECK_AND_RETURN_FALSE(!m_meshDataDescriptorSet);

    // renderables resources
    m_renderableDescriptorPool = VkGfxDescriptorPool::Builder(ctx)
                                     .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 5 * maxFramesCount)
                                     .setDebugName("renderables_desc_pool")
                                     .build(maxFramesCount, VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT);
    CHECK_AND_RETURN_FALSE(!m_renderableDescriptorPool);

    m_renderableDescriptorSetLayout = VkGfxDescriptorSetLayout::Builder(ctx)
                                          .addBinding(
                                              0, // model matrices binding
                                              VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                              VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT,
                                              1,
                                              true)
                                          .addBinding(
                                              1, // normal matrices binding
                                              VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                              VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT,
                                              1,
                                              true)
                                          .addBinding(
                                              2, // bounding boxes binding
                                              VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                              VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT,
                                              1,
                                              true)
                                          .addBinding(
                                              3, // mesh ids binding
                                              VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                              VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT,
                                              1,
                                              true)
                                          .addBinding(
                                              4, // material ids binding
                                              VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                              VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT,
                                              1,
                                              true)
                                          .setDebugName("renderables_desc_set_layout")
                                          .build();
    CHECK_AND_RETURN_FALSE(!m_renderableDescriptorSetLayout);

    m_frameRenderables.resize(maxFramesCount);
    m_modelMatrixBuffers.resize(maxFramesCount);
    m_normalMatrixBuffers.resize(maxFramesCount);
    m_boundingBoxBuffers.resize(maxFramesCount);
    m_meshIdsBuffers.resize(maxFramesCount);
    m_materialIdsBuffers.resize(maxFramesCount);
    m_renderableDescriptorSets.resize(maxFramesCount);
    for (uint32_t frameIdx = 0u; frameIdx < maxFramesCount; ++frameIdx)
    {
        m_frameRenderables[frameIdx].modelMatrices.reserve(MAX_RENDERABLES_COUNT);
        m_frameRenderables[frameIdx].normalMatrices.reserve(MAX_RENDERABLES_COUNT);
        m_frameRenderables[frameIdx].boundingBoxes.reserve(MAX_RENDERABLES_COUNT);
        m_frameRenderables[frameIdx].meshIds.reserve(MAX_RENDERABLES_COUNT);
        m_frameRenderables[frameIdx].materialIds.reserve(MAX_RENDERABLES_COUNT);

        GfxBuffer::createHostWriteBuffer(
            GfxBufferUsageFlags::StorageBuffer,
            sizeof(glm::mat4),
            MAX_RENDERABLES_COUNT,
            "model_matrix_buffer",
            &m_modelMatrixBuffers[frameIdx]);

        GfxBuffer::createHostWriteBuffer(
            GfxBufferUsageFlags::StorageBuffer,
            sizeof(glm::mat4),
            MAX_RENDERABLES_COUNT,
            "normal_matrix_buffer",
            &m_normalMatrixBuffers[frameIdx]);

        GfxBuffer::createHostWriteBuffer(
            GfxBufferUsageFlags::StorageBuffer,
            sizeof(GfxBoundingBoxData),
            MAX_RENDERABLES_COUNT,
            "bounding_box_buffer",
            &m_boundingBoxBuffers[frameIdx]);

        GfxBuffer::createHostWriteBuffer(
            GfxBufferUsageFlags::StorageBuffer,
            sizeof(uint32_t),
            MAX_RENDERABLES_COUNT,
            "mesh_id_buffer",
            &m_meshIdsBuffers[frameIdx]);

        GfxBuffer::createHostWriteBuffer(
            GfxBufferUsageFlags::StorageBuffer,
            sizeof(uint32_t),
            MAX_RENDERABLES_COUNT,
            "material_id_buffer",
            &m_materialIdsBuffers[frameIdx]);

        m_renderableDescriptorSets[frameIdx] = m_renderableDescriptorPool->allocateDescriptorSet(
            *m_renderableDescriptorSetLayout,
            "renderables_desc_set");

        DynamicArray<VkDescriptorBufferInfo> renderableBuffersInfo;
        renderableBuffersInfo.push_back(m_modelMatrixBuffers[frameIdx].getDescriptorInfo());
        renderableBuffersInfo.push_back(m_normalMatrixBuffers[frameIdx].getDescriptorInfo());
        renderableBuffersInfo.push_back(m_boundingBoxBuffers[frameIdx].getDescriptorInfo());
        renderableBuffersInfo.push_back(m_meshIdsBuffers[frameIdx].getDescriptorInfo());
        renderableBuffersInfo.push_back(m_materialIdsBuffers[frameIdx].getDescriptorInfo());

        m_renderableDescriptorSets[frameIdx]->configureBuffer(
            0,
            0,
            renderableBuffersInfo.size(),
            renderableBuffersInfo.data());

        m_renderableDescriptorSets[frameIdx]->applyConfiguration();
    }

    return true;
}

void Engine::cleanupGlobals()
{
    m_vertexBuffer.cleanup();
    m_indexBuffer.cleanup();

    m_globalDescriptorPool->resetPool();
    m_globalDescriptorSetLayout = nullptr;
    m_globalDescriptorPool      = nullptr;

    m_materialDescriptorPool->resetPool();
    m_materialDescriptorSetLayout = nullptr;
    m_materialDescriptorPool      = nullptr;

    m_meshDataDescriptorPool->resetPool();
    m_meshDataDescriptorSetLayout = nullptr;
    m_meshDataDescriptorPool      = nullptr;

    m_renderableDescriptorPool->resetPool();
    m_renderableDescriptorSetLayout = nullptr;
    m_renderableDescriptorPool      = nullptr;

    for (uint32_t frameIdx = 0u; frameIdx < m_renderer->getMaxFramesCount(); ++frameIdx)
    {
        m_modelMatrixBuffers[frameIdx].cleanup();
        m_normalMatrixBuffers[frameIdx].cleanup();
        m_boundingBoxBuffers[frameIdx].cleanup();
        m_meshIdsBuffers[frameIdx].cleanup();
        m_materialIdsBuffers[frameIdx].cleanup();
    }

    m_globalUbos.cleanup();
    m_materialsBuffer.cleanup();
    m_meshDataBuffer.cleanup();

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
    DUSK_PROFILE_FUNCTION;

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

    // Indirect draw resources
    m_rgResources.indirectDrawDescriptorPool = VkGfxDescriptorPool::Builder(ctx)
                                                   .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2 * maxFramesCount)
                                                   .setDebugName("indirect draw_desc_pool")
                                                   .build(maxFramesCount, VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT);

    m_rgResources.indirectDrawDescriptorSetLayout = VkGfxDescriptorSetLayout::Builder(ctx)
                                                        .addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, 1, true)
                                                        .addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, 1, true)
                                                        .setDebugName("indirect draw_desc_set_layout")
                                                        .build();

    m_rgResources.frameIndirectDrawCommandsBuffers.resize(maxFramesCount);
    m_rgResources.frameIndirectDrawCountBuffers.resize(maxFramesCount);
    m_rgResources.indirectDrawDescriptorSet.resize(maxFramesCount);

    for (uint32_t frameIdx = 0u; frameIdx < maxFramesCount; ++frameIdx)
    {
        m_rgResources.frameIndirectDrawCommandsBuffers[frameIdx].init(
            GfxBufferUsageFlags::StorageBuffer | GfxBufferUsageFlags::IndirectBuffer | GfxBufferUsageFlags::TransferTarget,
            sizeof(GfxIndexedIndirectDrawCommand) * MAX_RENDERABLES_COUNT * 2, // doubled because first half buffer for gbuffer pass, second half for shadow pass
            GfxBufferMemoryTypeFlags::DedicatedDeviceMemory,
            std::format("indirect_draw_buffer_{}", std::to_string(frameIdx)));

        // TODO: count buffer should scale with piplines used. For now we assume only one pipeline uses indirect draws.
        GfxBuffer::createDeviceLocalBuffer(
            GfxBufferUsageFlags::StorageBuffer | GfxBufferUsageFlags::IndirectBuffer | GfxBufferUsageFlags::TransferTarget,
            sizeof(GfxIndexedIndirectDrawCount),
            1,
            std::format("indirect_draw_count_buffer_{}", std::to_string(frameIdx)),
            &m_rgResources.frameIndirectDrawCountBuffers[frameIdx]);

        m_rgResources.indirectDrawDescriptorSet[frameIdx] = m_rgResources.indirectDrawDescriptorPool->allocateDescriptorSet(
            *m_rgResources.indirectDrawDescriptorSetLayout, "indirect_draw_desc_set");

        DynamicArray<VkDescriptorBufferInfo> drawCmdBufferInfo;
        drawCmdBufferInfo.push_back(m_rgResources.frameIndirectDrawCommandsBuffers[frameIdx].getDescriptorInfo());

        DynamicArray<VkDescriptorBufferInfo> drawCountBufferInfo;
        drawCountBufferInfo.push_back(m_rgResources.frameIndirectDrawCountBuffers[frameIdx].getDescriptorInfoAtIndex(0));

        m_rgResources.indirectDrawDescriptorSet[frameIdx]->configureBuffer(
            0,
            0,
            drawCmdBufferInfo.size(),
            drawCmdBufferInfo.data());

        m_rgResources.indirectDrawDescriptorSet[frameIdx]->configureBuffer(
            1,
            0,
            drawCountBufferInfo.size(),
            drawCountBufferInfo.data());

        m_rgResources.indirectDrawDescriptorSet[frameIdx]->applyConfiguration();
    }

    // g-buffer resources
    // Allocate g-buffer render targets
    m_rgResources.gbuffRenderTextureIds.push_back(m_textureDB->createColorTexture(
        "gbuffer_pass_albedo",
        extent.width,
        extent.height,
        VK_FORMAT_R8G8B8A8_UNORM));
    m_rgResources.gbuffRenderTextureIds.push_back(m_textureDB->createColorTexture(
        "gbuffer_pass_normal",
        extent.width,
        extent.height,
        VK_FORMAT_R16G16B16A16_UNORM));

    m_rgResources.gbuffRenderTextureIds.push_back(m_textureDB->createColorTexture(
        "gbuffer_pass_occlu_rough_metal",
        extent.width,
        extent.height,
        VK_FORMAT_R8G8B8A8_UNORM));

    m_rgResources.gbuffRenderTextureIds.push_back(m_textureDB->createColorTexture(
        "gbuffer_pass_emissive_color",
        extent.width,
        extent.height,
        VK_FORMAT_R8G8B8A8_UNORM));

    // Allocate g-buffer depth texture
    m_rgResources.gbuffDepthTextureId = m_textureDB->createDepthTexture(
        "gbuffer_pass_depth",
        extent.width,
        extent.height,
        VK_FORMAT_D32_SFLOAT_S8_UINT);

    // create g-buff pipeline layout
    m_rgResources.gbuffPipelineLayout = VkGfxPipelineLayout::Builder(ctx)
                                            .addPushConstantRange(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(DrawData))
                                            .addDescriptorSetLayout(m_globalDescriptorSetLayout->layout)
                                            .addDescriptorSetLayout(m_materialDescriptorSetLayout->layout)
                                            .addDescriptorSetLayout(m_renderableDescriptorSetLayout->layout)
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
                                      .addColorAttachmentFormat(VK_FORMAT_R8G8B8A8_UNORM)     // albedo
                                      .addColorAttachmentFormat(VK_FORMAT_R16G16B16A16_UNORM) // normal
                                      .addColorAttachmentFormat(VK_FORMAT_R8G8B8A8_UNORM)     // ao-roughness-metallic
                                      .addColorAttachmentFormat(VK_FORMAT_R8G8B8A8_UNORM)     // emissive color
                                      .setDebugName("gbuff_pipeline")
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
                                        .setDebugName("present_pipeline")
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
                                         .setDebugName("lighting_pipeline")
                                         .build();
#ifdef VK_RENDERER_DEBUG
    vkdebug::setObjectName(
        ctx.device,
        VK_OBJECT_TYPE_PIPELINE,
        (uint64_t)m_rgResources.lightingPipeline->get(),
        "lighting_pipeline");
#endif // VK_RENDERER_DEBUG

    // brdf lut pipeline
    /*m_rgResources.brdfLUTextureId = m_textureDB->createStorageTexture(
        "brdf_lut_tex",
        512,
        512,
        VK_FORMAT_R32G32_SFLOAT);*/
    std::filesystem::path brdfTexturePath = buildPath / "textures/brdf.ktx2";
    m_rgResources.brdfLUTextureId         = m_textureDB->createTextureAsync(brdfTexturePath.string(), TextureType::Texture2D, PixelFormat::R16G16_sfloat);

    VkSampler           brdfSampler;
    VkSamplerCreateInfo samplerInfo {};
    samplerInfo.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter               = VK_FILTER_LINEAR;
    samplerInfo.minFilter               = VK_FILTER_LINEAR;
    samplerInfo.addressModeU            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.anisotropyEnable        = VK_TRUE;
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
                                        .setDebugName("brdf_lut_pipeline")
                                        .build();
#ifdef VK_RENDERER_DEBUG
    vkdebug::setObjectName(
        ctx.device,
        VK_OBJECT_TYPE_PIPELINE,
        (uint64_t)m_rgResources.brdfLUTPipeline->get(),
        "brdf_lut_pipeline");
#endif // VK_RENDERER_DEBUG

    // shadow pass for directional and spot lights
    /*m_rgResources.dirShadowMapsTextureId = m_textureDB->createDepthTextureArray(
        "dir_shadow_maps",
        extent.width,
        extent.height,
        4,
        VK_FORMAT_D32_SFLOAT_S8_UINT);*/
    m_rgResources.dirShadowMapsTextureId = m_textureDB->createDepthTexture(
        "dir_shadow_maps",
        extent.width,
        extent.height,
        VK_FORMAT_D32_SFLOAT_S8_UINT);

    VkSampler shadowSampler;
    samplerInfo                         = {};
    samplerInfo.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter               = VK_FILTER_LINEAR;
    samplerInfo.minFilter               = VK_FILTER_LINEAR;
    samplerInfo.addressModeU            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.anisotropyEnable        = VK_TRUE;
    samplerInfo.maxAnisotropy           = ctx.physicalDeviceProperties.limits.maxSamplerAnisotropy;
    samplerInfo.borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable           = VK_TRUE;               // changed for shadow maps
    samplerInfo.compareOp               = VK_COMPARE_OP_GREATER; // changed for shadow maps
    samplerInfo.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR;

    vkCreateSampler(ctx.device, &samplerInfo, nullptr, &shadowSampler);
    m_textureDB->updateTextureSampler(m_rgResources.dirShadowMapsTextureId, shadowSampler);

    m_rgResources.shadow2DMapPipelineLayout = VkGfxPipelineLayout::Builder(ctx)
                                                  .addDescriptorSetLayout(m_renderableDescriptorSetLayout->layout)
                                                  .addDescriptorSetLayout(m_lightsSystem->getLightsDescriptorSetLayout().layout)
                                                  .build();

#ifdef VK_RENDERER_DEBUG
    vkdebug::setObjectName(
        ctx.device,
        VK_OBJECT_TYPE_PIPELINE_LAYOUT,
        (uint64_t)m_rgResources.shadow2DMapPipelineLayout->get(),
        "shadow_2d_map_pipeline_layout");
#endif // VK_RENDERER_DEBUG

    auto shadowMapVertShaderCode      = FileSystem::readFileBinary(shaderPath / "dir_shadows.vert.spv");

    auto shadowMapFragShaderCode      = FileSystem::readFileBinary(shaderPath / "dir_shadows.frag.spv");

    m_rgResources.shadow2DMapPipeline = VkGfxRenderPipeline::Builder(ctx)
                                            .setVertexShaderCode(shadowMapVertShaderCode)
                                            .setFragmentShaderCode(shadowMapFragShaderCode)
                                            .setPipelineLayout(*m_rgResources.shadow2DMapPipelineLayout)
                                            .setViewMask(0)
                                            .setDebugName("shadow_2d_map_pipeline")
                                            .build();

    // cull & lod compute pipeline
    m_rgResources.cullLodPipelineLayout = VkGfxPipelineLayout::Builder(ctx)
                                              .addPushConstantRange(VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(CullLodPushConstant))
                                              .addDescriptorSetLayout(m_globalDescriptorSetLayout->layout)
                                              .addDescriptorSetLayout(m_meshDataDescriptorSetLayout->layout)
                                              .addDescriptorSetLayout(m_renderableDescriptorSetLayout->layout)
                                              .addDescriptorSetLayout(m_rgResources.indirectDrawDescriptorSetLayout->layout)
                                              .build();

#ifdef VK_RENDERER_DEBUG
    vkdebug::setObjectName(
        ctx.device,
        VK_OBJECT_TYPE_PIPELINE_LAYOUT,
        (uint64_t)m_rgResources.cullLodPipelineLayout->get(),
        "cull_lod_pipeline_layout");
#endif // VK_RENDERER_DEBUG

    auto cullLodShader            = FileSystem::readFileBinary(shaderPath / "cull_lod.comp.spv");

    m_rgResources.cullLodPipeline = VkGfxComputePipeline::Builder(ctx)
                                        .setComputeShaderCode(cullLodShader)
                                        .setPipelineLayout(*m_rgResources.cullLodPipelineLayout)
                                        .setDebugName("cull_lod_pipeline")
                                        .build();
#ifdef VK_RENDERER_DEBUG
    vkdebug::setObjectName(
        ctx.device,
        VK_OBJECT_TYPE_PIPELINE,
        (uint64_t)m_rgResources.cullLodPipeline->get(),
        "cull_lod_pipeline");
#endif // VK_RENDERER_DEBUG
}

void Engine::releaseRenderGraphResources()
{
    for (auto& buffer : m_rgResources.frameIndirectDrawCommandsBuffers)
        buffer.cleanup();

    for (auto& buffer : m_rgResources.frameIndirectDrawCountBuffers)
        buffer.cleanup();

    m_rgResources.indirectDrawDescriptorPool->resetPool();
    m_rgResources.indirectDrawDescriptorSetLayout = nullptr;
    m_rgResources.indirectDrawDescriptorPool      = nullptr;

    m_rgResources.gbuffPipeline                   = nullptr;
    m_rgResources.gbuffPipelineLayout             = nullptr;

    m_rgResources.presentPipeline                 = nullptr;
    m_rgResources.presentPipelineLayout           = nullptr;

    m_rgResources.lightingPipeline                = nullptr;
    m_rgResources.lightingPipelineLayout          = nullptr;

    m_rgResources.brdfLUTPipeline                 = nullptr;
    m_rgResources.brdfLUTPipelineLayout           = nullptr;

    m_rgResources.shadow2DMapPipeline             = nullptr;
    m_rgResources.shadow2DMapPipelineLayout       = nullptr;

    m_rgResources.cullLodPipeline                 = nullptr;
    m_rgResources.cullLodPipelineLayout           = nullptr;
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

#ifdef VK_RENDERER_DEBUG
    vkdebug::setObjectName(
        ctx.device,
        VK_OBJECT_TYPE_COMMAND_BUFFER,
        (uint64_t)commandBuffer,
        "brdf_lut_cmd_buff");
#endif // VK_RENDERER_DEBUG

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

size_t Engine::copyToVertexBuffer(const GfxBuffer& srcVertexBuffer, size_t size)
{
    size_t startOffset = m_availableVertexOffset;

    m_gfxDevice->copyBuffer(
        srcVertexBuffer.vkBuffer,
        0,
        m_vertexBuffer.vkBuffer,
        m_availableVertexOffset * sizeof(Vertex),
        size);

    m_availableVertexOffset += size / sizeof(Vertex);

    return startOffset;
}

size_t Engine::copyToIndexBuffer(const GfxBuffer& srcIndexBuffer, size_t size)
{
    size_t startOffset = m_availableIndexOffset;

    m_gfxDevice->copyBuffer(
        srcIndexBuffer.vkBuffer,
        0,
        m_indexBuffer.vkBuffer,
        m_availableIndexOffset * sizeof(uint32_t),
        size);

    m_availableIndexOffset += size / sizeof(uint32_t);

    return startOffset;
}

// TODO:: implement with chunks if needed
void Engine::uploadVertexAndIndexBuffers(
    DynamicArray<Vertex>&   vertices,
    DynamicArray<uint32_t>& indices,
    int*                    outVertexOffset,
    uint32_t*               outfirstIndex)
{
    *outVertexOffset        = static_cast<int>(m_availableVertexOffset);
    *outfirstIndex          = static_cast<uint32_t>(m_availableIndexOffset);

    auto      totalVertices = vertices.size();
    auto      totalIndices  = indices.size();

    GfxBuffer stagingBuffer;
    size_t    stagingBufferSize = vertices.size() * sizeof(Vertex) + indices.size() * sizeof(uint32_t);
    GfxBuffer::createHostWriteBuffer(
        GfxBufferUsageFlags::TransferSource,
        stagingBufferSize,
        1,
        "staging_vertex_index_buffer",
        &stagingBuffer);

    // upload vertex buffer
    stagingBuffer.writeAndFlush(0, (void*)vertices.data(), totalVertices * sizeof(Vertex));

    m_gfxDevice->copyBuffer(
        stagingBuffer.vkBuffer,
        0,
        m_vertexBuffer.vkBuffer,
        m_availableVertexOffset * sizeof(Vertex),
        totalVertices * sizeof(Vertex));

    m_availableVertexOffset += totalVertices;

    // upload index buffer
    stagingBuffer.writeAndFlush(
        totalVertices * sizeof(Vertex),
        (void*)indices.data(),
        totalIndices * sizeof(uint32_t));

    m_gfxDevice->copyBuffer(
        stagingBuffer.vkBuffer,
        totalVertices * sizeof(Vertex),
        m_indexBuffer.vkBuffer,
        m_availableIndexOffset * sizeof(uint32_t),
        totalIndices * sizeof(uint32_t));

    m_availableIndexOffset += totalIndices;

    stagingBuffer.cleanup();
}

} // namespace dusk