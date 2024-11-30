// This file creates the entrypoint main function
// wherever it gets included

#pragma once

#include "dusk.h"
#include "core/application.h"

using namespace dusk;

extern Unique<dusk::Application> dusk::createApplication(int argc, char** argv);

int main(int argc, char** argv)
{
    dusk::Logger::init();

    APP_INFO("Starting App {}");

    auto app = dusk::createApplication(argc, argv);
    app->start();
    app->run();
}