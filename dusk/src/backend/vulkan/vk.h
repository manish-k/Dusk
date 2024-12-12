#pragma once

#include "core/error.h"

#include <volk.h>
#include <string>
#include <sstream>

namespace dusk
{
/**
 * @brief Encapsulation of VkResult with useful functions
 */
struct VulkanResult
{
    VkResult vkResult = VK_SUCCESS;

    VulkanResult() { }
    VulkanResult(VkResult result) :
        vkResult(result) { }

    bool        isOk() { return vkResult == VK_SUCCESS; }
    bool        hasError() { return vkResult != VK_SUCCESS; }

    Error       getErrorId();
    std::string toString();
};
} // namespace dusk