#include "engine.h"
#include "events/app_event.h"
#include "backend/vulkan/vk_renderer.h"
#include "renderer/frame_data.h"

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

    m_basicRenderSystem = createUnique<BasicRenderSystem>(*m_gfxDevice, *m_globalDescritptorSetLayout);

    return true;
}

void Engine::run()
{
    TimePoint m_lastFrameTime = Time::now();

    while (m_running)
    {
        TimePoint newTime     = Time::now();
        TimeStep  m_deltaTime = newTime - m_lastFrameTime;
        m_lastFrameTime       = newTime;

        onUpdate(m_deltaTime);

        m_app->onUpdate(m_deltaTime);

        m_window->onUpdate(m_deltaTime);
    }
}

void Engine::stop() { m_running = false; }

void Engine::shutdown()
{
    m_basicRenderSystem = nullptr;

    cleanupGlobals();

    m_renderer->cleanup();
    m_gfxDevice->cleanupGfxDevice();
}

void Engine::onUpdate(TimeStep dt)
{
    if (!m_currentScene) return;

    m_currentScene->onUpdate(dt);

    // get game objects and render system
    if (VkCommandBuffer commandBuffer = m_renderer->beginFrame())
    {
        uint32_t  currentFrameIndex = m_renderer->getCurrentFrameIndex();
        FrameData frameData {
            currentFrameIndex,
            dt,
            commandBuffer,
            *m_currentScene,
            m_globalDescriptorSet->set
        };

        CameraComponent& camera = m_currentScene->getMainCamera();

        camera.setPerspectiveProjection(glm::radians(50.f), m_renderer->getAspectRatio(), 0.5f, 100.f);

        GlobalUbo ubo {};
        ubo.view              = camera.viewMatrix;
        ubo.prjoection        = camera.projectionMatrix;
        ubo.inverseView       = camera.inverseViewMatrix;

        ubo.lightDirection    = glm::vec4(normalize(glm::vec3(1.0, -3.0, -1.0)), 0.f);
        ubo.ambientLightColor = glm::vec4(0.7f, 0.8f, 0.8f, 0.8f);

        void* dst             = (char*)m_globalUbos.mappedMemory + sizeof(GlobalUbo) * currentFrameIndex;
        memcpy(dst, &ubo, sizeof(GlobalUbo));

        m_renderer->beginRendering(commandBuffer);

        m_basicRenderSystem->renderGameObjects(frameData);

        m_renderer->endRendering(commandBuffer);

        m_renderer->endFrame();

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
}

bool Engine::setupGlobals()
{
    uint32_t      maxFramesCount = m_renderer->getMaxFramesCount();
    VulkanContext ctx            = VkGfxDevice::getSharedVulkanContext();

    // create global descriptor pool
    m_globalDescriptorPool = createUnique<VkGfxDescriptorPool>(ctx);
    VulkanResult result    = m_globalDescriptorPool
                              ->addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, maxFramesCount)
                              .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 100)
                              .create(1, VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT);

    if (result.hasError())
    {
        DUSK_ERROR("Error in creation of global descriptor pool {}", result.toString());
        return false;
    }

    m_globalDescritptorSetLayout = createUnique<VkGfxDescriptorSetLayout>(ctx);
    result                       = m_globalDescritptorSetLayout
                 ->addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS, maxFramesCount)
                 .addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 100, true)
                 .create();
    if (result.hasError())
    {
        DUSK_ERROR("Error in creation of global descriptor set layout {}", result.toString());
        return false;
    }

    // uniform buffer params for creation
    GfxBufferParams uboParams {};
    uboParams.sizeInBytes = maxFramesCount * sizeof(GlobalUbo);
    uboParams.usage       = GfxBufferUsageFlags::UniformBuffer;
    uboParams.memoryType  = GfxBufferMemoryTypeFlags::HostSequentialWrite | GfxBufferMemoryTypeFlags::PersistentlyMapped;

    result                = m_gfxDevice->createBuffer(uboParams, &m_globalUbos);
    if (result.hasError())
    {
        DUSK_ERROR("Unable to create uniform buffer {}", result.toString());
        return false;
    }
    m_gfxDevice->mapBuffer(&m_globalUbos);

    m_globalDescriptorSet = createUnique<VkGfxDescriptorSet>(ctx, *m_globalDescritptorSetLayout, *m_globalDescriptorPool);

    result                = m_globalDescriptorSet->create();
    if (result.hasError())
    {
        DUSK_ERROR("Unable to create global descriptor set {}", result.toString());
        return false;
    }

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

    return true;
}

void Engine::cleanupGlobals()
{
    m_gfxDevice->unmapBuffer(&m_globalUbos);
    m_gfxDevice->freeBuffer(&m_globalUbos);

    m_globalDescriptorPool->resetPool();

    m_globalDescritptorSetLayout->destroy();
    m_globalDescriptorPool->destroy();
}

void Engine::registerTextures(DynamicArray<Texture>& textures)
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
}

} // namespace dusk