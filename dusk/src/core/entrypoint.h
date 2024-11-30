// This file creates the entrypoint main function
// wherever it gets included

#pragma once

#include "dusk.h"
#include "engine.h"
#include "core/application.h"

using namespace dusk;

extern Unique<dusk::Application> dusk::createApplication(int argc, char** argv);

int main(int argc, char** argv)
{
    // start logger
    dusk::Logger::init();

    auto engineConfig = Engine::Config::defaultConfig();
    auto engine = createUnique<Engine>(engineConfig);

    Shared<Application> app = std::move(dusk::createApplication(argc, argv));

    if (!engine->start(app))
    {
        DUSK_ERROR("Failed to start engine");
        return -1;
    }

    if (!app->start())
    {
        DUSK_ERROR("Failed to start app");
        return -1;
    }

    engine->run();

    app->shutdown();
    engine->shutdown();
}