#pragma once

#include <glm/glm.hpp>

namespace dusk
{

struct alignas(16) Material
{
    int32_t   id                     = -1;
    int32_t   albedoTexId            = -1;
    int32_t   normalTexId            = -1;
    int32_t   metallicRoughnessTexId = -1;

    int32_t   aoTexId                = -1;
    int32_t   emissiveTexId          = -1;
    float     aoStrength             = 1.0f; // Modulate AO texture
    float     emissiveIntensity      = 1.0f; // Modulate emissive output

    float     normalScale            = 1.0f; // Scale normal map effect
    float     metal                  = 1.0f; // Uniform fallback
    float     rough                  = 1.0f; // Uniform fallback
    float     padding0               = 0.0f; // Padding to align vec4

    glm::vec4 albedoColor            = glm::vec4 { 1.f };
    glm::vec4 emissiveColor          = glm::vec4 { 1.f };
};

} // namespace dusk