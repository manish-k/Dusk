#pragma once

#include "dusk.h"

#include <string>

namespace dusk
{
class VkGfxTexture;

class Texture
{
public:
    Texture() = default;
    ~Texture() = default;

    static Unique<Texture> loadFromFile(const std::string& filepath);

private:
    uint32_t     m_id;
    uint32_t     m_width;
    uint32_t     m_height;
    uint32_t     m_numChannels;

    //VkGfxTexture m_gfxTexture;
};
} // namespace dusk