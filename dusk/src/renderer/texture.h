#pragma once

#include "dusk.h"
#include "backend/vulkan/vk_types.h"

namespace dusk
{
class Image;

struct Texture2D
{
    uint32_t      id;
    uint32_t      width       = 0;
    uint32_t      height      = 0;
    uint32_t      numChannels = 0;

    VulkanTexture vkTexture {};
    VulkanSampler vkSampler {}; // TODO: sampler can be a common one instead of per texture

    Texture2D(uint32_t id) :
        id(id) {};
    ~Texture2D() = default;

    Error init(Image& texImage, const char* debugName = nullptr);
    void  free();
};

struct Texture3D
{
    uint32_t      id;
    uint32_t      width       = 0;
    uint32_t      height      = 0;
    uint32_t      numChannels = 0;
    uint32_t      numImages   = 0;

    VulkanTexture vkTexture {};
    VulkanSampler vkSampler {}; // TODO: sampler can be a common one instead of per texture

    Texture3D(uint32_t id) :
        id(id) {};
    ~Texture3D() = default;

    Error init(DynamicArray<Image>& texImages); // TODO: input params not ideal, maybe support with shared ptr?
    void  free();
};

} // namespace dusk