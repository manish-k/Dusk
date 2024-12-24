#include "engine.h"
#include "events/app_event.h"
#include "backend/vulkan/vk_renderer.h"

namespace dusk
{
Engine* Engine::s_instance = nullptr;

Engine::Engine(Engine::Config& config) :
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

    m_renderer = createUnique<VulkanRenderer>(std::dynamic_pointer_cast<GLFWVulkanWindow>(m_window));
    if (!m_renderer->init("Test", 1))
    {
        DUSK_ERROR("Renderer initialization failed");
        return false;
    }

    m_running = true;
    m_paused  = false;

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

void Engine::shutdown() { }

void Engine::onUpdate(TimeStep dt) { }

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
} // namespace dusk