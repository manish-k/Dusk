#include "engine.h"
#include "events/app_event.h"

#include <chrono>
#include <thread>

using namespace std::chrono_literals;

namespace dusk
{
    Engine* Engine::s_instance = nullptr;

    Engine::Engine(Engine::Config& config) : m_config(config)
    {
        DASSERT(!s_instance, "Engine instance already exists");
        s_instance = this;
    }

    Engine::~Engine() {}

    bool Engine::start(Shared<Application> app)
    {
        DASSERT(!m_app, "Application is null")

        m_app = app;

        // create window
        auto windowProps = Window::Properties::defaultWindowProperties();
        m_window = std::move(Window::createWindow(windowProps));
        if (!m_window)
        {
            DUSK_ERROR("Window creation failed");
            return false;
        }
        m_window->setEventCallback([this](Event& ev) { this->onEvent(ev); });

        m_running = true;
        m_paused = false;

        return true;
    }

    void Engine::run()
    {
        auto currentTime = std::chrono::high_resolution_clock::now();

        while (m_running)
        {
            auto newTime = std::chrono::high_resolution_clock::now();
            float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
            currentTime = newTime;

            onUpdate(frameTime);

            m_app->onUpdate(frameTime);

            m_window->onUpdate(frameTime);
            //std::this_thread::sleep_for(16ms);
        }
    }

    void Engine::stop() { m_running = false; }

    void Engine::shutdown() {}

    void Engine::onUpdate(float dt) { m_window->onUpdate(dt); }

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
                DUSK_INFO("WindowResizeEvent received");
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