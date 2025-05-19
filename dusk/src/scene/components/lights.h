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
    int32_t   id        = -1;
    int32_t   pad0;
    int32_t   pad1;
    int32_t   pad2;
    glm::vec4 color     = glm::vec4 { 1.0f }; // w component for intensity
    glm::vec3 direction = glm::vec3 { 0.f };
};

struct PointLightComponent
{
    int32_t   id = -1;
    glm::vec3 position { 0.f };
    glm::vec4 color { 1.0f }; // w component for intensity

    // attenuation values from https://learnopengl.com/Lighting/Light-casters
    float constantAttenuationFactor  = 1.0f;
    float linearAttenuationFactor    = 0.09f;
    float quadraticAttenuationFactor = 0.032f;
};

struct SpotLightComponent
{
    int32_t   id = -1;
    glm::vec3 position { 0.f };
    glm::vec3 direction { 0.f }; // direction where light is being pointed
    glm::vec4 color { 1.0f };    // w component for intensity

    // attenuation values from https://learnopengl.com/Lighting/Light-casters
    float constantAttenuationFactor  = 1.0f;
    float linearAttenuationFactor    = 0.09f;
    float quadraticAttenuationFactor = 0.032f;

    float innerCutOffAngle           = 0.f; // in degrees
    float outerCutOffAngle           = 0.f; // in degrees
};
} // namespace dusk