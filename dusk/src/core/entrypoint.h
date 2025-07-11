// This file creates the entrypoint main function
// wherever it gets included

#pragma once

#include "dusk.h"
#include "engine.h"
#include "core/application.h"
#include "debug/profiler.h"
#include "debug/renderdoc.h"

using namespace dusk;

extern Unique<dusk::Application> dusk::createApplication(int argc, char** argv);

// main func
int main(int argc, char** argv)
{
    DUSK_PROFILE_THREAD("main_thread");

    // start logger
    dusk::Logger::init();

     #ifdef ENABLE_RENDERDOC
         if (!renderdoc::init()) DUSK_ERROR("Unable to load renderdoc library");
     #endif

    auto engineConfig = Engine::Config::defaultConfig();
    auto engine       = createUnique<Engine>(engineConfig);

    // create application
    Shared<Application> app = std::move(dusk::createApplication(argc, argv));

    {
        DUSK_PROFILE_SECTION("engine start");
        if (!engine->start(app))
        {
            DUSK_ERROR("Failed to start engine");
            return -1;
        }
    }

    {
        DUSK_PROFILE_SECTION("app start");
        if (!app->start())
        {
            DUSK_ERROR("Failed to start app");
            return -1;
        }
    }

    engine->run();

    app->shutdown();
    engine->shutdown();
}