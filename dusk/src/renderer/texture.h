#pragma once

#include "backend/vulkan/vk_types.h"

namespace dusk
{
class Image;
class VulkanTexture;
class VulkanSampler;

class Texture
{
public:
    Texture(uint32_t id) :
        m_id(id) {};
    ~Texture() = default;

    Error init(Image& texImage);
    void  free();

private:
    uint32_t      m_id;
    uint32_t      m_width       = 0;
    uint32_t      m_height      = 0;
    uint32_t      m_numChannels = 0;

    VulkanTexture m_vkTexture {};
    VulkanSampler m_vkSampler {};
};
} // namespace dusk