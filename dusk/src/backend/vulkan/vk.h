#pragma once

#include "core/error.h"
#include "vk_base.h"

#include <volk.h>
#include <string>
#include <iostream>
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

    bool        isOk() const { return vkResult == VK_SUCCESS; }
    bool        hasError() const { return vkResult != VK_SUCCESS; }

    Error       getErrorId() const;
    std::string toString() const;
};

inline std::ostream& operator<<(std::ostream& os, const VulkanResult& result) { return os << result.toString(); }
} // namespace dusk