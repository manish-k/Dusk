#pragma once

#include "base.h"

#include "spdlog/spdlog.h"

namespace dusk
{
	/**
	 * @brief Logging class for the Application and Engine
	 */
	class Logger final
	{
	public:
		/**
		 * @brief Initialize the logger with format
		 * and sink settings
		 */
		static void Init();

		/**
		 * @brief Get static instance of Engine logger
		 * @return Shared pointer to static instance
		 */
		static SharedP<spdlog::logger>& GetEngineLogger() { return s_EngineLogger; }

		/**
		 * @brief Get static instance of Application logger
		 * @return Shared pointer to static instance
		 */
		static SharedP<spdlog::logger>& GetAppLogger() { return s_AppLogger; }

	private:
		static SharedP<spdlog::logger> s_EngineLogger;
		static SharedP<spdlog::logger> s_AppLogger;
	};
}