#pragma once

#include "dusk.h"
#include "renderer/vertex.h"

namespace dusk
{

struct AABB
{
    glm::vec3 min;
    glm::vec3 max;
};

inline AABB computeAABB(const DynamicArray<Vertex>& vertices)
{
    AABB aabb;
    aabb.min                 = glm::vec3(std::numeric_limits<float>::max());
    aabb.max                 = glm::vec3(std::numeric_limits<float>::lowest());

    const size_t vertexCount = vertices.size();
    for (size_t i = 0; i < vertexCount; ++i)
    {
        const auto& pos = vertices[i].position;
        aabb.min.x      = glm::min(aabb.min.x, pos.x);
        aabb.min.y      = glm::min(aabb.min.y, pos.y);
        aabb.min.z      = glm::min(aabb.min.z, pos.z);
        aabb.max.x      = glm::max(aabb.max.x, pos.x);
        aabb.max.y      = glm::max(aabb.max.y, pos.y);
        aabb.max.z      = glm::max(aabb.max.z, pos.z);
    }
    return aabb;
}

inline AABB recomputeAABB(const AABB& aabb, const glm::mat4& transform)
{
    const glm::vec3 center       = (aabb.min + aabb.max) * 0.5f;
    const glm::vec3 extents      = (aabb.max - aabb.min) * 0.5f;

    const glm::vec3 worldCenter  = glm::vec3(transform * glm::vec4(center, 1.0f));

    const glm::mat3 absTransform = glm::mat3(
        glm::abs(glm::vec3(transform[0])),
        glm::abs(glm::vec3(transform[1])),
        glm::abs(glm::vec3(transform[2])));

    const glm::vec3 worldExtents = absTransform * extents;

    return {
        worldCenter - worldExtents,
        worldCenter + worldExtents
    };
}

} // namespace dusk