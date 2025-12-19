#pragma once

#include "plane.h"

namespace dusk
{
struct Frustum
{
    Plane top    = {};
    Plane bottom = {};

    Plane left   = {};
    Plane right  = {};

    Plane near   = {};
    Plane far    = {};
};

inline Frustum extractFrustumFromMatrix(const glm::mat4& vpMatrix)
{
    Frustum frustum = {};

    // column major order
    // Left plane
    frustum.left.normal.x = vpMatrix[0][3] + vpMatrix[0][0];
    frustum.left.normal.y = vpMatrix[1][3] + vpMatrix[1][0];
    frustum.left.normal.z = vpMatrix[2][3] + vpMatrix[2][0];
    frustum.left.distance = vpMatrix[3][3] + vpMatrix[3][0];
    frustum.left          = normalizePlane(frustum.left);
    
    // Right plane
    frustum.right.normal.x = vpMatrix[0][3] - vpMatrix[0][0];
    frustum.right.normal.y = vpMatrix[1][3] - vpMatrix[1][0];
    frustum.right.normal.z = vpMatrix[2][3] - vpMatrix[2][0];
    frustum.right.distance = vpMatrix[3][3] - vpMatrix[3][0];
    frustum.right          = normalizePlane(frustum.right);
    
    // Bottom plane
    frustum.bottom.normal.x = vpMatrix[0][3] + vpMatrix[0][1];
    frustum.bottom.normal.y = vpMatrix[1][3] + vpMatrix[1][1];
    frustum.bottom.normal.z = vpMatrix[2][3] + vpMatrix[2][1];
    frustum.bottom.distance = vpMatrix[3][3] + vpMatrix[3][1];
    frustum.bottom          = normalizePlane(frustum.bottom);
    
    // Top plane
    frustum.top.normal.x = vpMatrix[0][3] - vpMatrix[0][1];
    frustum.top.normal.y = vpMatrix[1][3] - vpMatrix[1][1];
    frustum.top.normal.z = vpMatrix[2][3] - vpMatrix[2][1];
    frustum.top.distance = vpMatrix[3][3] - vpMatrix[3][1];
    frustum.top          = normalizePlane(frustum.top);
    
    // Near plane, specific to vulkan's clip space
    frustum.near.normal.x = vpMatrix[0][2];
    frustum.near.normal.y = vpMatrix[1][2];
    frustum.near.normal.z = vpMatrix[2][2];
    frustum.near.distance = vpMatrix[3][2];
    frustum.near          = normalizePlane(frustum.near);

    // Far plane
    frustum.far.normal.x = vpMatrix[0][3] - vpMatrix[0][2];
    frustum.far.normal.y = vpMatrix[1][3] - vpMatrix[1][2];
    frustum.far.normal.z = vpMatrix[2][3] - vpMatrix[2][2];
    frustum.far.distance = vpMatrix[3][3] - vpMatrix[3][2];
    frustum.far          = normalizePlane(frustum.far);
    
    return frustum;
}
} // namespace dusk
