#pragma once

#include "base.h"

#include "spdlog/spdlog.h"

namespace dusk
{
	class Logger final
	{
	public:

		static void Init();

		static SharedP<spdlog::logger>& GetEngineLogger() { return s_EngineLogger; }
		static SharedP<spdlog::logger>& GetAppLogger() { return s_AppLogger; }

	private:
		static SharedP<spdlog::logger> s_EngineLogger;
		static SharedP<spdlog::logger> s_AppLogger;
	};
}