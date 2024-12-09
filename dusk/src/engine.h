#pragma once

#include "dusk.h"
#include "core/dtime.h"
#include "core/application.h"
#include "events/event.h"
#include "platform/window.h"
#include "renderer/render_api.h"
#include "renderer/renderer.h"

namespace dusk
{
class Engine final
{
public:
    struct Config
    {
        RenderAPI::API renderAPI;

        static Config defaultConfig()
        {
            auto config      = Config {};
            config.renderAPI = RenderAPI::API::VULKAN;
            return config;
        }
    };

public:
    Engine(Config& conf);
    ~Engine();

    bool start(Shared<Application> app);
    void run();
    void stop();
    void shutdown();
    void onUpdate(TimeStep dt);
    void onEvent(Event& ev);

private:
    Config m_config;

    Unique<Renderer> m_renderer = nullptr;

    Shared<Window>      m_window = nullptr;
    Shared<Application> m_app    = nullptr;

    bool m_running = false;
    bool m_paused  = false;

    TimePoint m_lastFrameTime;
    TimeStep  m_deltaTime;

private:
    static Engine* s_instance;
};
} // namespace dusk