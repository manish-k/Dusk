#pragma once

#include <glm/glm.hpp>

namespace dusk
{

struct alignas(16) Material
{
    int32_t   id          = -1;
    int32_t   albedoTexId = -1;
    int32_t   normalTexId = -1;

    int32_t   padding;

    glm::vec4 albedoColor = glm::vec4 { 1.f };
};

} // namespace dusk