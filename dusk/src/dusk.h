#pragma once

#include "core/base.h"
#include "core/log.h"

// Engine Logger Macros
#define DUSK_INFO(...) dusk::Logger::getEngineLogger()->info(__VA_ARGS__)
#define DUSK_WARN(...) dusk::Logger::getEngineLogger()->warn(__VA_ARGS__)
#define DUSK_ERROR(...) dusk::Logger::getEngineLogger()->error(__VA_ARGS__)
#define DUSK_CRITICAL(...) dusk::Logger::getEngineLogger()->critical(__VA_ARGS__)
#define DUSK_TRACE(...) dusk::Logger::getEngineLogger()->trace(__VA_ARGS__)

// App Logger Macros
#define APP_INFO(...) dusk::Logger::getAppLogger()->info(__VA_ARGS__)
#define APP_WARN(...) dusk::Logger::getAppLogger()->warn(__VA_ARGS__)
#define APP_ERROR(...) dusk::Logger::getAppLogger()->error(__VA_ARGS__)
#define APP_CRITICAL(...) dusk::Logger::getAppLogger()->critical(__VA_ARGS__)
#define APP_TRACE(...) dusk::Logger::getAppLogger()->trace(__VA_ARGS__)