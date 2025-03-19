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

    // get game objects and render system
    if (VkCommandBuffer commandBuffer = m_renderer->beginFrame())
    {
        uint32_t  currentFrameIndex = m_renderer->getCurrentFrameIndex();
        FrameData frameData {
            currentFrameIndex,
            dt,
            commandBuffer,
            *m_currentScene,
            m_globalDescriptorSets[currentFrameIndex].set
        };

        CameraComponent& camera = m_currentScene->getMainCamera();

        camera.setPerspectiveProjection(glm::radians(50.f), m_renderer->getAspectRatio(), 0.5f, 100.f);

        GlobalUbo ubo {};
        ubo.view              = camera.viewMatrix;
        ubo.prjoection        = camera.projectionMatrix;
        ubo.inverseView       = camera.inverseViewMatrix;
        ubo.ambientLightColor = glm::vec4(0.7f, 0.8f, 0.8f, 0.f);

        memcpy(m_globalUbos[currentFrameIndex].mappedMemory, &ubo, sizeof(GlobalUbo));

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

    dispatcher.dispatch<WindowResizeEvent>(
        [this](WindowResizeEvent ev)
        {
            // DUSK_INFO("WindowResizeEvent received");
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

    // pass unhandled event to application
    if (!ev.isHandled())
    {
        m_app->onEvent(ev);
    }
}

void Engine::loadScene(Scene* scene)
{
    m_currentScene = scene;
}

bool Engine::setupGlobals()
{
    uint32_t      maxFramesCount = m_renderer->getMaxFramesCount();
    VulkanContext ctx            = VkGfxDevice::getSharedVulkanContext();

    // create global descriptor pool
    m_globalDescriptorPool = createUnique<VkGfxDescriptorPool>(ctx);
    VulkanResult result    = m_globalDescriptorPool->addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1).create(maxFramesCount, 0);
    if (result.hasError())
    {
        DUSK_ERROR("Error in creation of global descriptor pool {}", result.toString());
        return false;
    }

    m_globalDescritptorSetLayout = createUnique<VkGfxDescriptorSetLayout>(ctx);
    result                       = m_globalDescritptorSetLayout->addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS, 1)
                 .create();
    if (result.hasError())
    {
        DUSK_ERROR("Error in creation of global descriptor set layout {}", result.toString());
        return false;
    }

    m_globalUbos = DynamicArray<VulkanGfxBuffer>(maxFramesCount);

    // uniform buffer params for creation
    GfxBufferParams uboParams {};
    uboParams.sizeInBytes = sizeof(GlobalUbo);
    uboParams.usage       = GfxBufferUsageFlags::UniformBuffer;
    uboParams.memoryType  = GfxBufferMemoryTypeFlags::HostSequentialWrite | GfxBufferMemoryTypeFlags::PersistentlyMapped;

    for (uint32_t uboIndex = 0u; uboIndex < maxFramesCount; ++uboIndex)
    {
        result = m_gfxDevice->createBuffer(uboParams, &m_globalUbos[uboIndex]);
        if (result.hasError())
        {
            DUSK_ERROR("Unable to create uniform buffer {}", result.toString());
            return false;
        }
        m_gfxDevice->mapBuffer(&m_globalUbos[uboIndex]);
    }

    for (uint32_t setIndex = 0u; setIndex < maxFramesCount; ++setIndex)
    {
        VkGfxDescriptorSet     set { ctx, *m_globalDescritptorSetLayout, *m_globalDescriptorPool };
        VulkanGfxBuffer&       ubo = m_globalUbos[setIndex];
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = ubo.buffer;
        bufferInfo.offset = 0;
        bufferInfo.range  = sizeof(GlobalUbo);

        result = set.create();
        if (result.hasError())
        {
            DUSK_ERROR("Unable to create global descriptor set {}", result.toString());
            return false;
        }

        set.configureBuffer(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &bufferInfo);
        set.applyConfiguration();

        m_globalDescriptorSets.push_back(set);
    }

    return true;
}

void Engine::cleanupGlobals()
{
    for (uint32_t uboIndex = 0u; uboIndex < m_globalUbos.size(); ++uboIndex)
    {
        m_gfxDevice->unmapBuffer(&m_globalUbos[uboIndex]);
        m_gfxDevice->freeBuffer(&m_globalUbos[uboIndex]);
    }

    m_globalDescriptorPool->resetPool();

    m_globalDescritptorSetLayout->destroy();
    m_globalDescriptorPool->destroy();
}

} // namespace dusk