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
        const glm::vec3& pos = vertices[i].position;
        aabb.min.x           = glm::min(aabb.min.x, pos.x);
        aabb.min.y           = glm::min(aabb.min.y, pos.y);
        aabb.min.z           = glm::min(aabb.min.z, pos.z);
        aabb.max.x           = glm::max(aabb.max.x, pos.x);
        aabb.max.y           = glm::max(aabb.max.y, pos.y);
        aabb.max.z           = glm::max(aabb.max.z, pos.z);
    }
    return aabb;
}

// Recomputes an AABB by transforming its corners with the given transformation matrix.
// This is a fast approach but may produce larger AABBs than necessary. 
inline AABB recomputeAABB(const AABB& aabb, const glm::mat4& transform)
{
    glm::vec3 corners[8] = {
        glm::vec3(aabb.min.x, aabb.min.y, aabb.min.z),
        glm::vec3(aabb.max.x, aabb.min.y, aabb.min.z),
        glm::vec3(aabb.min.x, aabb.max.y, aabb.min.z),
        glm::vec3(aabb.max.x, aabb.max.y, aabb.min.z),
        glm::vec3(aabb.min.x, aabb.min.y, aabb.max.z),
        glm::vec3(aabb.max.x, aabb.min.y, aabb.max.z),
        glm::vec3(aabb.min.x, aabb.max.y, aabb.max.z),
        glm::vec3(aabb.max.x, aabb.max.y, aabb.max.z),
    };

    AABB transformedAABB;
    transformedAABB.min = glm::vec3(std::numeric_limits<float>::max());
    transformedAABB.max = glm::vec3(std::numeric_limits<float>::lowest());

    for (uint8_t i = 0u; i < 8; ++i)
    {
        glm::vec4 transformedCorner = transform * glm::vec4(corners[i], 1.0f);
        transformedAABB.min.x       = glm::min(transformedAABB.min.x, transformedCorner.x);
        transformedAABB.min.y       = glm::min(transformedAABB.min.y, transformedCorner.y);
        transformedAABB.min.z       = glm::min(transformedAABB.min.z, transformedCorner.z);
        transformedAABB.max.x       = glm::max(transformedAABB.max.x, transformedCorner.x);
        transformedAABB.max.y       = glm::max(transformedAABB.max.y, transformedCorner.y);
        transformedAABB.max.z       = glm::max(transformedAABB.max.z, transformedCorner.z);
    }
    return transformedAABB;
}

} // namespace dusk