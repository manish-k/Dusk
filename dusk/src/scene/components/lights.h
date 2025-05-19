#pragma once

#include "dusk.h"

namespace dusk
{
struct AmbientLightComponent
{
    glm::vec4 color = glm::vec4 { 1.0f }; // w component for intensity
};

struct alignas(16) DirectionalLightComponent
{
    int32_t   id = -1;
    int32_t   pad0;
    int32_t   pad1;
    int32_t   pad2;
    glm::vec4 color     = glm::vec4 { 1.0f }; // w component for intensity
    glm::vec3 direction = glm::vec3 { 0.f };
};

struct alignas(16) PointLightComponent
{
    int32_t id = -1;

    // attenuation values from https://learnopengl.com/Lighting/Light-casters
    float     constantAttenuationFactor  = 1.0f;
    float     linearAttenuationFactor    = 0.09f;
    float     quadraticAttenuationFactor = 0.032f;

    glm::vec4 color { 1.0f }; // w component for intensity
    glm::vec3 position { 0.f };
};

struct alignas(16) SpotLightComponent
{
    int32_t id = -1;

    // attenuation values from https://learnopengl.com/Lighting/Light-casters
    float     constantAttenuationFactor  = 1.0f;
    float     linearAttenuationFactor    = 0.09f;
    float     quadraticAttenuationFactor = 0.032f;

    glm::vec4 color { 1.0f };         // w component for intensity
    glm::vec3 position { 0.f };
    float     innerCutOffAngle = 0.f; // in degrees
    glm::vec3 direction { 0.f };      // direction where light is being pointed
    float     outerCutOffAngle = 0.f; // in degrees
};
} // namespace dusk