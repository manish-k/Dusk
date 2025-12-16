#pragma once

#include "dusk.h"

#undef far
#undef near

namespace dusk
{
struct Plane
{
    glm::vec3 normal   = { 0.f, 1.f, 0.f };
    float     distance = 0.f; // distance from origin along normal

    glm::vec4 toVec4() const
    {
        return glm::vec4(normal, distance);
    }
};

inline Plane normalizePlane(const Plane& plane)
{
    Plane normalizedPlane = plane;
    float length          = glm::length(plane.normal);
    if (length > 0.f)
    {
        normalizedPlane.normal   = plane.normal / length;
        normalizedPlane.distance = plane.distance / length;
    }
    return normalizedPlane;
}
} // namespace dusk