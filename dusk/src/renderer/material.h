#pragma once

#include <glm/glm.hpp>

namespace dusk
{

struct Material
{
    uint32_t  id          = -1;

    glm::vec4 albedoColor = glm::vec4 { 1.f };

    uint32_t  albedoTexId = -1;
    uint32_t  normalTexId = -1;
};

} // namespace dusk