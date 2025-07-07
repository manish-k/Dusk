#pragma once

#include "texture.h"
#include "vk_types.h"

namespace dusk
{
struct RenderTarget
{
    Texture2D    texture;
    VkFormat     format;
    VkClearValue clearValue;
};
} // namespace dusk