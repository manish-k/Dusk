#pragma once

#include <chrono>

namespace dusk
{
/**
 * @brief Alias of std::chrono::high_resolution_clock
 */
using Time = std::chrono::high_resolution_clock;

/**
 * @brief TimePoint is a duration of time that has passed since the epoch of a
 * high resolution clock
 */
using TimePoint = std::chrono::time_point<std::chrono::high_resolution_clock>;

/**
 * @brief Duration in seconds
 */
using TimeStep = std::chrono::duration<float, std::chrono::seconds::period>;

/**
 * @brief Duration in nano seconds
 */
using TimeStepNs = std::chrono::duration<float, std::chrono::nanoseconds::period>;
} // namespace dusk
