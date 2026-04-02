#include "log.h"

#include <spdlog/async.h>
#include <spdlog/async_logger.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace dusk
{
Shared<spdlog::logger> Logger::s_engineLogger;
Shared<spdlog::logger> Logger::s_appLogger;
Shared<spdlog::logger> Logger::s_vulkanLogger;

// initialize loggers
void Logger::init()
{
    spdlog::init_thread_pool(8192, 1);

    auto consoleSink = createShared<spdlog::sinks::stdout_color_sink_mt>();
    consoleSink->set_pattern("%^[%T] [%l] %n: %v%$");

    auto fileSink = createShared<spdlog::sinks::basic_file_sink_mt>("logs/dusk.log", true);
    fileSink->set_pattern("[%T] [%l] %n: %v");

    DynamicArray<spdlog::sink_ptr> logSinks {
        consoleSink,
        fileSink
    };

    s_appLogger = createShared<spdlog::async_logger>(
        "App",
        logSinks.begin(),
        logSinks.end(),
        spdlog::thread_pool(),
        spdlog::async_overflow_policy::block);
    s_appLogger->set_level(spdlog::level::trace);
    s_appLogger->flush_on(spdlog::level::trace);
    spdlog::register_logger(s_appLogger);

    s_engineLogger = createShared<spdlog::async_logger>(
        "Dusk",
        logSinks.begin(),
        logSinks.end(),
        spdlog::thread_pool(),
        spdlog::async_overflow_policy::block);
    s_engineLogger->set_level(spdlog::level::trace);
    s_engineLogger->flush_on(spdlog::level::trace);
    spdlog::register_logger(s_engineLogger);

    s_vulkanLogger = createShared<spdlog::async_logger>(
        "Vulkan",
        logSinks.begin(),
        logSinks.end(),
        spdlog::thread_pool(),
        spdlog::async_overflow_policy::block);
    s_vulkanLogger->set_level(spdlog::level::trace);
    s_vulkanLogger->flush_on(spdlog::level::trace);
    spdlog::register_logger(s_vulkanLogger);

    s_engineLogger->info("Initialized logger.");
}

void Logger::shutdown()
{
    spdlog::shutdown();
}

void Logger::appAssertLog(const std::string& msg)
{
    getAppLogger()->critical("Assertion failed: {}", msg);
}

void Logger::appAssertLog()
{
    getAppLogger()->critical("Assertion failed");
}

void Logger::engineAssertLog(const std::string& msg)
{
    getEngineLogger()->critical("Assertion failed: {}", msg);
}

void Logger::engineAssertLog()
{
    getEngineLogger()->critical("Assertion failed");
}

} // namespace dusk