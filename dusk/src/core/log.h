#pragma once

#include "base.h"

#include "spdlog/spdlog.h"

namespace dusk
{
	class Logger final
	{
	public:

		static void init();

		static SharedP<spdlog::logger>& getEngineLogger() { return s_EngineLogger; }
		static SharedP<spdlog::logger>& getAppLogger() { return s_AppLogger; }

	private:
		static SharedP<spdlog::logger> s_EngineLogger;
		static SharedP<spdlog::logger> s_AppLogger;
	};
}