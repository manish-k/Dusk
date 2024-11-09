#pragma once

#include "dusk.h"

int main(int argc, char** argv)
{
	dusk::Logger::Init();

	APP_INFO("Starting App {}");
}