#include "log.h"

#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"

namespace dusk
{
	Shared<spdlog::logger> Logger::s_engineLogger;
	Shared<spdlog::logger> Logger::s_appLogger;

	void Logger::init()
	{
		auto consoleSink = createShared<spdlog::sinks::stdout_color_sink_mt>();
		consoleSink->set_pattern("%^[%T] %n: %v%$");

		auto fileSink = createShared<spdlog::sinks::basic_file_sink_mt>("logs/dusk.log", true);
		fileSink->set_pattern("[%T] [%l] %n: %v");

		std::vector<spdlog::sink_ptr> logSinks{
			consoleSink,
			fileSink
		};

		s_appLogger = createShared<spdlog::logger>(
			"App",
			logSinks.begin(),
			logSinks.end()
		);
		s_appLogger->set_level(spdlog::level::trace);
		s_appLogger->flush_on(spdlog::level::trace);
		spdlog::register_logger(s_appLogger);

		s_engineLogger = createShared<spdlog::logger>(
			"Dusk",
			logSinks.begin(),
			logSinks.end()
		);
		s_engineLogger->set_level(spdlog::level::trace);
		s_engineLogger->flush_on(spdlog::level::trace);
		spdlog::register_logger(s_engineLogger);

		s_engineLogger->info("Initialized logger.");
	}
}