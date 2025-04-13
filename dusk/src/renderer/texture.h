#pragma once

#include "dusk.h"

#include "backend/vulkan/vk_types.h"

namespace dusk
{
class Image;

struct Texture
{
    uint32_t      id;
    uint32_t      width       = 0;
    uint32_t      height      = 0;
    uint32_t      numChannels = 0;

    VulkanTexture vkTexture {};
    VulkanSampler vkSampler {};

    Texture(uint32_t id) :
        id(id) {};
    ~Texture() = default;

    Error init(Image& texImage);
    void  free();
};

} // namespace dusk