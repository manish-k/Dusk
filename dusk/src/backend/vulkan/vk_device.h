#pragma once

#include <volk.h>

#include <string_view>

namespace dusk
{
class VkGfxDevice
{
public:
    VkGfxDevice();
    ~VkGfxDevice();

    bool createInstance(std::string_view appName, uint32_t version);
    void destroyInstance();
};
} // namespace dusk