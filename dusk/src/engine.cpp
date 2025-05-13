#include "engine.h"
#include "events/app_event.h"
#include "backend/vulkan/vk_renderer.h"
#include "renderer/frame_data.h"
#include "utils/utils.h"

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

    m_basicRenderSystem = createUnique<BasicRenderSystem>(
        *m_gfxDevice,
        *m_globalDescriptorSetLayout,
        *m_materialDescriptorSetLayout);

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
            ubo.view              = camera.viewMatrix;
            ubo.prjoection        = camera.projectionMatrix;
            ubo.inverseView       = camera.inverseViewMatrix;

            ubo.lightDirection    = glm::vec4(normalize(glm::vec3(1.0, -3.0, -1.0)), 0.f);
            ubo.ambientLightColor = glm::vec4(0.7f, 0.8f, 0.8f, 0.8f);

            void* dst             = (char*)m_globalUbos.mappedMemory + sizeof(GlobalUbo) * currentFrameIndex;
            memcpy(dst, &ubo, sizeof(GlobalUbo));

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
                                      .addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 100, true) // TODO: make count configurable
                                      .build();
    CHECK_AND_RETURN_FALSE(!m_globalDescriptorSetLayout);

    // uniform buffer params for creation
    GfxBufferParams uboParams {};
    uboParams.sizeInBytes = maxFramesCount * sizeof(GlobalUbo);
    uboParams.usage       = GfxBufferUsageFlags::UniformBuffer;
    uboParams.memoryType  = GfxBufferMemoryTypeFlags::HostSequentialWrite | GfxBufferMemoryTypeFlags::PersistentlyMapped;

    VulkanResult result   = m_gfxDevice->createBuffer(uboParams, &m_globalUbos);
    if (result.hasError())
    {
        DUSK_ERROR("Unable to create uniform buffer {}", result.toString());
        return false;
    }
    m_gfxDevice->mapBuffer(&m_globalUbos);

    m_globalDescriptorSet = m_globalDescriptorPool->allocateDescriptorSet(*m_globalDescriptorSetLayout);
    CHECK_AND_RETURN_FALSE(!m_globalDescriptorSet);

    DynamicArray<VkDescriptorBufferInfo> buffersInfo(maxFramesCount);

    for (uint32_t frameIndex = 0u; frameIndex < maxFramesCount; ++frameIndex)
    {
        VkDescriptorBufferInfo& bufferInfo = buffersInfo[frameIndex];
        bufferInfo.buffer                  = m_globalUbos.buffer;
        bufferInfo.offset                  = sizeof(GlobalUbo) * frameIndex;
        bufferInfo.range                   = sizeof(GlobalUbo);

        m_globalDescriptorSet->configureBuffer(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, frameIndex, 1, &bufferInfo);
    }

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
    size_t          alignmentSize = getAlignment(sizeof(Material), ctx.physicalDeviceProperties.limits.minStorageBufferOffsetAlignment);
    GfxBufferParams matBufferParams {};
    matBufferParams.sizeInBytes   = maxMaterialCount * alignmentSize;
    matBufferParams.usage         = GfxBufferUsageFlags::StorageBuffer;
    matBufferParams.memoryType    = GfxBufferMemoryTypeFlags::HostSequentialWrite | GfxBufferMemoryTypeFlags::PersistentlyMapped;
    matBufferParams.alignmentSize = alignmentSize;

    result                        = m_gfxDevice->createBuffer(matBufferParams, &m_materialsBuffer);
    if (result.hasError())
    {
        DUSK_ERROR("Unable to create storage buffer for materials {}", result.toString());
        return false;
    }
    m_gfxDevice->mapBuffer(&m_materialsBuffer);

    m_materialsDescriptorSet = m_materialDescriptorPool->allocateDescriptorSet(*m_materialDescriptorSetLayout);
    CHECK_AND_RETURN_FALSE(!m_materialsDescriptorSet);

    // setup lighting globasl
    m_lightsDescriptorPool = VkGfxDescriptorPool::Builder(ctx)
                                 .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, maxSupportedLights)
                                 .build(1, VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT);
    CHECK_AND_RETURN_FALSE(!m_lightsDescriptorPool);

    m_lightsDescriptorSetLayout = VkGfxDescriptorSetLayout::Builder(ctx)
                                      .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 1)                        // Ambient light
                                      .addBinding(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, maxSupportedLights, true) // Directional Light
                                      .addBinding(2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, maxSupportedLights, true) // Point Light
                                      .addBinding(3, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, maxSupportedLights, true) // Sport Light
                                      .build();
    CHECK_AND_RETURN_FALSE(!m_lightsDescriptorSetLayout);

    m_lightsDescriptorSet = m_lightsDescriptorPool->allocateDescriptorSet(*m_lightsDescriptorSetLayout);
    CHECK_AND_RETURN_FALSE(!m_lightsDescriptorSet);

    // create ambient light buffer
    alignmentSize           = getAlignment(sizeof(AmbientLightComponent), ctx.physicalDeviceProperties.limits.minUniformBufferOffsetAlignment);

    uint32_t bufferSize     = static_cast<uint32_t>(alignmentSize);

    uboParams               = {};
    uboParams.sizeInBytes   = bufferSize;
    uboParams.usage         = GfxBufferUsageFlags::UniformBuffer;
    uboParams.memoryType    = GfxBufferMemoryTypeFlags::HostSequentialWrite | GfxBufferMemoryTypeFlags::PersistentlyMapped;
    uboParams.alignmentSize = alignmentSize;

    result                  = m_gfxDevice->createBuffer(uboParams, &m_ambientLightBuffer);
    if (result.hasError())
    {
        DUSK_ERROR("Unbale to create uniform buffer for ambient light {}", result.toString());
        return false;
    }
    m_gfxDevice->mapBuffer(&m_ambientLightBuffer);

    // create directional light buffer
    alignmentSize           = getAlignment(sizeof(DirectionalLightComponent), ctx.physicalDeviceProperties.limits.minUniformBufferOffsetAlignment);

    bufferSize              = maxSupportedLights * alignmentSize;

    uboParams               = {};
    uboParams.sizeInBytes   = bufferSize;
    uboParams.usage         = GfxBufferUsageFlags::UniformBuffer;
    uboParams.memoryType    = GfxBufferMemoryTypeFlags::HostSequentialWrite | GfxBufferMemoryTypeFlags::PersistentlyMapped;
    uboParams.alignmentSize = alignmentSize;

    result                  = m_gfxDevice->createBuffer(uboParams, &m_directionalLightsBuffer);
    if (result.hasError())
    {
        DUSK_ERROR("Unbale to create uniform buffer for directional lights {}", result.toString());
        return false;
    }
    m_gfxDevice->mapBuffer(&m_directionalLightsBuffer);

    // create point light buffer
    alignmentSize           = getAlignment(sizeof(PointLightComponent), ctx.physicalDeviceProperties.limits.minUniformBufferOffsetAlignment);

    bufferSize              = maxSupportedLights * alignmentSize;

    uboParams               = {};
    uboParams.sizeInBytes   = bufferSize;
    uboParams.usage         = GfxBufferUsageFlags::UniformBuffer;
    uboParams.memoryType    = GfxBufferMemoryTypeFlags::HostSequentialWrite | GfxBufferMemoryTypeFlags::PersistentlyMapped;
    uboParams.alignmentSize = alignmentSize;

    result                  = m_gfxDevice->createBuffer(uboParams, &m_pointLightsBuffer);
    if (result.hasError())
    {
        DUSK_ERROR("Unbale to create uniform buffer for point lights {}", result.toString());
        return false;
    }
    m_gfxDevice->mapBuffer(&m_pointLightsBuffer);

    // create spot light buffer
    alignmentSize           = getAlignment(sizeof(SpotLightComponent), ctx.physicalDeviceProperties.limits.minUniformBufferOffsetAlignment);

    bufferSize              = maxRenderableMeshes * alignmentSize;

    uboParams               = {};
    uboParams.sizeInBytes   = bufferSize;
    uboParams.usage         = GfxBufferUsageFlags::UniformBuffer;
    uboParams.memoryType    = GfxBufferMemoryTypeFlags::HostSequentialWrite | GfxBufferMemoryTypeFlags::PersistentlyMapped;
    uboParams.alignmentSize = alignmentSize;

    result                  = m_gfxDevice->createBuffer(uboParams, &m_spotLightsBuffer);
    if (result.hasError())
    {
        DUSK_ERROR("Unbale to create uniform buffer for spot lights {}", result.toString());
        return false;
    }
    m_gfxDevice->mapBuffer(&m_spotLightsBuffer);

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

    m_lightsDescriptorPool->resetPool();
    m_lightsDescriptorSetLayout = nullptr;
    m_lightsDescriptorPool      = nullptr;

    m_gfxDevice->unmapBuffer(&m_globalUbos);
    m_gfxDevice->freeBuffer(&m_globalUbos);

    m_gfxDevice->unmapBuffer(&m_materialsBuffer);
    m_gfxDevice->freeBuffer(&m_materialsBuffer);

    m_gfxDevice->unmapBuffer(&m_ambientLightBuffer);
    m_gfxDevice->freeBuffer(&m_ambientLightBuffer);
    m_gfxDevice->unmapBuffer(&m_directionalLightsBuffer);
    m_gfxDevice->freeBuffer(&m_directionalLightsBuffer);
    m_gfxDevice->unmapBuffer(&m_pointLightsBuffer);
    m_gfxDevice->freeBuffer(&m_pointLightsBuffer);
    m_gfxDevice->unmapBuffer(&m_spotLightsBuffer);
    m_gfxDevice->freeBuffer(&m_spotLightsBuffer);
}

void Engine::registerTextures(DynamicArray<Texture2D>& textures)
{
    DynamicArray<VkDescriptorImageInfo> imagesInfo(textures.size());
    for (uint32_t texIndex = 0u; texIndex < textures.size(); ++texIndex)
    {
        imagesInfo[texIndex].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imagesInfo[texIndex].imageView   = textures[texIndex].vkTexture.imageView;
        imagesInfo[texIndex].sampler     = textures[texIndex].vkSampler.sampler;
    }

    m_globalDescriptorSet->configureImage(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, textures.size(), imagesInfo.data());

    m_globalDescriptorSet->applyConfiguration();
}

void Engine::registerMaterials(DynamicArray<Material>& materials)
{
    DynamicArray<VkDescriptorBufferInfo> matInfo(materials.size());
    for (uint32_t matIndex = 0u; matIndex < matInfo.size(); ++matIndex)
    {
        VkDescriptorBufferInfo& bufferInfo = matInfo[matIndex];
        bufferInfo.buffer                  = m_materialsBuffer.buffer;
        bufferInfo.offset                  = m_materialsBuffer.alignmentSize * matIndex;
        bufferInfo.range                   = m_materialsBuffer.alignmentSize;
    }

    m_materialsDescriptorSet->configureBuffer(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 0, materials.size(), matInfo.data());

    m_materialsDescriptorSet->applyConfiguration();
}

} // namespace dusk