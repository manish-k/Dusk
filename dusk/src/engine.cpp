#include "engine.h"

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
        m_window = std::move(Window::createWindow());
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

            std::this_thread::sleep_for(16ms);
        }
    }

    void Engine::shutdown() {}

    void Engine::onUpdate(float dt) { m_window->onUpdate(dt); }

    void Engine::onEvent(Event& ev) {}
} // namespace dusk