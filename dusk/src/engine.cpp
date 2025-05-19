#include "engine.h"

#include "core/application.h"
#include "platform/window.h"
#include "utils/utils.h"
#include "ui/ui.h"

#include "scene/scene.h"
#include "scene/components/camera.h"

#include "events/event.h"
#include "events/app_event.h"

#include "backend/vulkan/vk_device.h"
#include "backend/vulkan/vk_renderer.h"
#include "backend/vulkan/vk_descriptors.h"

#include "renderer/frame_data.h"
#include "renderer/material.h"
#include "renderer/texture.h"
#include "renderer/systems/basic_render_system.h"
#include "renderer/systems/grid_render_system.h"
#include "renderer/systems/skybox_render_system.h"
#include "renderer/systems/lights_system.h"

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
    TimePoint m_lastFrameTime = Time::now();

    while (m_running)
    {
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
    uint32_t currentFrameIndex = m_renderer->getCurrentFrameIndex();

    if (VkCommandBuffer commandBuffer = m_renderer->beginFrame())
    {
        FrameData frameData {
            currentFrameIndex,
            dt,
            commandBuffer,
            m_currentScene,
            m_globalDescriptorSet->set,
            m_lightsSystem->getLightsDescriptorSet().set,
            m_materialsDescriptorSet->set
        };

        m_renderer->beginRendering(commandBuffer);

        m_ui->beginRendering();

        if (m_currentScene)
        {
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
            m_ui->renderSceneWidgets(*m_currentScene);
        }

        m_basicRenderSystem->renderGameObjects(frameData);

        // m_gridRenderSystem->renderGrid(frameData);
        m_skyboxRenderSystem->renderSkybox(frameData);

        m_ui->endRendering(commandBuffer);

        m_renderer->endRendering(commandBuffer);

        m_renderer->endFrame();
    }

    m_renderer->deviceWaitIdle();
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

bool Engine::setupGlobals()
{
    uint32_t      maxFramesCount = m_renderer->getMaxFramesCount();
    VulkanContext ctx            = VkGfxDevice::getSharedVulkanContext();

    // create global descriptor pool
    m_globalDescriptorPool = VkGfxDescriptorPool::Builder(ctx)
                                 .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, maxFramesCount)
                                 .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 100) // TODO: make count configurable
                                 .build(1, VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT);
    CHECK_AND_RETURN_FALSE(!m_globalDescriptorPool);

    m_globalDescriptorSetLayout = VkGfxDescriptorSetLayout::Builder(ctx)
                                      .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS, maxFramesCount, true)
                                      .addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1000, true) // TODO: make count configurable
                                      .build();
    CHECK_AND_RETURN_FALSE(!m_globalDescriptorSetLayout);

    GfxBuffer::createHostWriteBuffer(
        GfxBufferUsageFlags::UniformBuffer,
        sizeof(GlobalUbo),
        maxFramesCount,
        "global_uniform_buffer",
        &m_globalUbos);
    CHECK_AND_RETURN_FALSE(!m_globalUbos.isAllocated());

    m_globalDescriptorSet = m_globalDescriptorPool->allocateDescriptorSet(*m_globalDescriptorSetLayout);
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
                                   .build(1, VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT);
    CHECK_AND_RETURN_FALSE(!m_materialDescriptorPool);

    m_materialDescriptorSetLayout = VkGfxDescriptorSetLayout::Builder(ctx)
                                        .addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, maxMaterialCount, true) // TODO: make count configurable
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

    m_materialsDescriptorSet = m_materialDescriptorPool->allocateDescriptorSet(*m_materialDescriptorSetLayout);
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

void Engine::setSkyboxVisibility(bool state)
{
    if (m_skyboxRenderSystem) m_skyboxRenderSystem->setVisble(state);
}

} // namespace dusk