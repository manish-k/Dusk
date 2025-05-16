#pragma once

#include <glm/glm.hpp>

namespace dusk
{

struct Material
{
    int32_t  id          = -1;

    glm::vec4 albedoColor = glm::vec4 { 1.f };

    int32_t   albedoTexId = -1;
    int32_t   normalTexId = -1;
};

} // namespace dusk