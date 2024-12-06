#pragma once

#include "vk_device.h"

namespace dusk
{
VkGfxDevice::VkGfxDevice()
{
}

VkGfxDevice::~VkGfxDevice()
{
}

bool VkGfxDevice::createInstance(std::string_view appName, uint32_t version)
{
    return VK_TRUE;
}

void VkGfxDevice::destroyInstance()
{
}
} // namespace dusk