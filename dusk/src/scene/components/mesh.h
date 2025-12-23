#pragma once

#include "dusk.h"
#include "renderer/geometry/aabb.h"

namespace dusk
{
struct MeshComponent
{
    DynamicArray<uint32_t> meshes     = {};
    DynamicArray<uint32_t> materials  = {};
    AABB                   objectAABB = {};
    AABB                   worldAABB  = {};
};
} // namespace dusk