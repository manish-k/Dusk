#pragma once

#include <cassert>

#include "core/application.h"
#include "core/base.h"
#include "core/log.h"

#ifdef NDEBUG
#else
    #define ENABLE_ASSERT
#endif  // DEBUG

// Engine Logger Macros
#define DUSK_INFO(...) dusk::Logger::GetEngineLogger()->info(__VA_ARGS__)
#define DUSK_WARN(...) dusk::Logger::GetEngineLogger()->warn(__VA_ARGS__)
#define DUSK_ERROR(...) dusk::Logger::GetEngineLogger()->error(__VA_ARGS__)
#define DUSK_CRITICAL(...) \
  dusk::Logger::GetEngineLogger()->critical(__VA_ARGS__)
#define DUSK_TRACE(...) dusk::Logger::GetEngineLogger()->trace(__VA_ARGS__)

// App Logger Macros
#define APP_INFO(...) dusk::Logger::GetAppLogger()->info(__VA_ARGS__)
#define APP_WARN(...) dusk::Logger::GetAppLogger()->warn(__VA_ARGS__)
#define APP_ERROR(...) dusk::Logger::GetAppLogger()->error(__VA_ARGS__)
#define APP_CRITICAL(...) dusk::Logger::GetAppLogger()->critical(__VA_ARGS__)
#define APP_TRACE(...) dusk::Logger::GetAppLogger()->trace(__VA_ARGS__)

// Assert Macro
#ifdef ENABLE_ASSERT
    #define DASSERT(check, ...)    \
      {                                 \
        if (!check) {                   \
          DUSK_ERROR(__VA_ARGS__); \
          __debugbreak();               \
        }                               \
      }
    #define ASSERT(check, ...)    \
      {                                \
        if (!check) {                  \
          APP_ERROR(__VA_ARGS__); \
          __debugbreak();              \
        }                              \
      }
#else
    #define DASSERT(...)
    #define ASSERT(...)
#endif  // ENABLE_ASSERT
