#include "application.h"
#include "platform/window.h"

#include <chrono>
#include <thread>

using namespace std::chrono_literals;

namespace dusk
{
    Application::Application() {}

    Application::~Application() {}

    void Application::run()
    {
        auto currentTime = std::chrono::high_resolution_clock::now();

        auto window = Window::createWindow();

        while (m_running)
        {
            auto newTime = std::chrono::high_resolution_clock::now();
            float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
            currentTime = newTime;

            window->onUpdate(frameTime);
            onUpdate(frameTime);

            std::this_thread::sleep_for(16ms);
        }
    }
} // namespace dusk