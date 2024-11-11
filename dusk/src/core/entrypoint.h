#pragma once

#include "dusk.h"

extern dusk::Application* dusk::CreateApplication(int argc, char** argv);

int main(int argc, char** argv)
{
	dusk::Logger::Init();

	APP_INFO("Starting App {}");

	auto app = dusk::CreateApplication(argc, argv);
	app->Run();

	delete app;
}