#pragma once

#include "base.h"

#include <spdlog/spdlog.h>
#include <string>

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
    static void init();

    /**
     * @brief Get static instance of Engine logger
     * @return Shared pointer to static instance
     */
    static Shared<spdlog::logger>& getEngineLogger() { return s_engineLogger; }

    /**
     * @brief Get static instance of Application logger
     * @return Shared pointer to static instance
     */
    static Shared<spdlog::logger>& getAppLogger() { return s_appLogger; }

    /**
     * @brief Get static instance of Vulkan debug logger
     * @return Shared pointer to static instance
     */
    static Shared<spdlog::logger>& getVulkanLogger() { return s_vulkanLogger; }

    /**
     * @brief app assertion log with msg
     * @param msg
     */
    static void appAssertLog(const std::string& msg);

    /**
     * @brief app assertion log without any msg
     */
    static void appAssertLog();

    /**
     * @brief engine assertion log with msg
     * @param msg
     */
    static void engineAssertLog(const std::string& msg);

    /**
     * @brief engine assertion log without any msg
     */
    static void engineAssertLog();

private:
    static Shared<spdlog::logger> s_engineLogger;
    static Shared<spdlog::logger> s_appLogger;
    static Shared<spdlog::logger> s_vulkanLogger;
};
} // namespace dusk