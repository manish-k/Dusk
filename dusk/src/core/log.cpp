#include "log.h"

#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"

namespace dusk
{
	SharedP<spdlog::logger> Logger::s_EngineLogger;
	SharedP<spdlog::logger> Logger::s_AppLogger;

	void Logger::init()
	{
		auto consoleSink = createSharedP<spdlog::sinks::stdout_color_sink_mt>();
		consoleSink->set_pattern("%^[%T] %n: %v%$");

		auto fileSink = createSharedP<spdlog::sinks::basic_file_sink_mt>("logs/dusk.log", true);
		fileSink->set_pattern("[%T] [%l] %n: %v");

		std::vector<spdlog::sink_ptr> logSinks{
			consoleSink,
			fileSink
		};

		s_AppLogger = createSharedP<spdlog::logger>(
			"App",
			logSinks.begin(),
			logSinks.end()
		);
		s_AppLogger->set_level(spdlog::level::trace);
		s_AppLogger->flush_on(spdlog::level::trace);
		spdlog::register_logger(s_AppLogger);

		s_EngineLogger = createSharedP<spdlog::logger>(
			"Dusk",
			logSinks.begin(),
			logSinks.end()
		);
		s_EngineLogger->set_level(spdlog::level::trace);
		s_EngineLogger->flush_on(spdlog::level::trace);
		spdlog::register_logger(s_EngineLogger);

		s_EngineLogger->info("Initialized logger.");
	}
}