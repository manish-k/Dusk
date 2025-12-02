#pragma once

#include "dusk.h"

namespace dusk
{
struct Plane
{
    glm::vec3 normal   = { 0.f, 1.f, 0.f };
    float     distance = 0.f; // distance from origin along normal
};
} // namespace dusk