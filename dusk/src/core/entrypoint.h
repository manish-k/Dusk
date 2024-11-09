#pragma once

#include "dusk.h"

int main(int argc, char** argv)
{
	dusk::Logger::init();

	APP_INFO("Starting App {}");
}