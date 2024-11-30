#pragma once

#include "events/event.h"

namespace dusk
{
    /**
     * @brief Base class for all rendering applications
     */
    class Application
    {
    public:
        Application();
        virtual ~Application();

        /**
         * @brief Starts the main loop of the rendering
         * application
         */
        void run();

        virtual bool start() = 0;
        virtual void shutdown() = 0;

        virtual void onUpdate(float dt) = 0;
        virtual void onEvent(Event& ev) = 0;

    private:
        bool m_running = true;
        bool m_paused = false;
    };

    /**
     * @brief Create the application
     * @param argc
     * @param argv
     * @return Pointer to the application instance
     */
    Application* createApplication(int argc, char** argv);
} // namespace dusk