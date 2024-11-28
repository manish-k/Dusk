#pragma once

#include <cassert>

#include "core/application.h"
#include "core/base.h"
#include "core/log.h"

#ifdef NDEBUG
#else
#define ENABLE_ASSERT
#endif // DEBUG

// Engine Logger Macros
#define DUSK_DEBUG(...) dusk::Logger::getEngineLogger()->debug(__VA_ARGS__)
#define DUSK_INFO(...) dusk::Logger::getEngineLogger()->info(__VA_ARGS__)
#define DUSK_WARN(...) dusk::Logger::getEngineLogger()->warn(__VA_ARGS__)
#define DUSK_ERROR(...) dusk::Logger::getEngineLogger()->error(__VA_ARGS__)
#define DUSK_CRITICAL(...) dusk::Logger::getEngineLogger()->critical(__VA_ARGS__)
#define DUSK_TRACE(...) dusk::Logger::getEngineLogger()->trace(__VA_ARGS__)

// App Logger Macros
#define APP_DEBUG(...) dusk::Logger::getAppLogger()->debug(__VA_ARGS__)
#define APP_INFO(...) dusk::Logger::getAppLogger()->info(__VA_ARGS__)
#define APP_WARN(...) dusk::Logger::getAppLogger()->warn(__VA_ARGS__)
#define APP_ERROR(...) dusk::Logger::getAppLogger()->error(__VA_ARGS__)
#define APP_CRITICAL(...) dusk::Logger::getAppLogger()->critical(__VA_ARGS__)
#define APP_TRACE(...) dusk::Logger::getAppLogger()->trace(__VA_ARGS__)

// Assert Macro
#ifdef ENABLE_ASSERT
#define DASSERT(check, ...)          \
    {                                \
        if (!check)                  \
        {                            \
            DUSK_ERROR(__VA_ARGS__); \
            __debugbreak();          \
        }                            \
    }
#define ASSERT(check, ...)          \
    {                               \
        if (!check)                 \
        {                           \
            APP_ERROR(__VA_ARGS__); \
            __debugbreak();         \
        }                           \
    }
#else
#define DASSERT(...)
#define ASSERT(...)
#endif // ENABLE_ASSERT

// Bit left shift Macro
#define BIT(x) (1 << x)
