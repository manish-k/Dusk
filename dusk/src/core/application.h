#pragma once

#include "core/base.h"
#include "core/dtime.h"
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

    virtual bool start()    = 0;
    virtual void shutdown() = 0;

    virtual void onUpdate(TimeStep dt) = 0;
    virtual void onEvent(Event& ev) = 0;

private:
    bool m_running = true;
    bool m_paused  = false;
};

/**
     * @brief Create the application
     * @param argc
     * @param argv
     * @return Pointer to the application instance
     */
Unique<Application> createApplication(int argc, char** argv);
} // namespace dusk