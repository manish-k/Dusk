#pragma once

#include "dusk.h"
#include "gfx_enums.h"

#include <glm/glm.hpp>

namespace dusk
{
struct VertexAttributeDescription
{
    uint32_t              location;
    uint32_t              offset;
    VertexAttributeFormat format;
};

struct Vertex
{
    glm::vec3 position {};
    glm::vec3 color {};
    glm::vec3 normal {};
    glm::vec2 uv {};

    /**
     * @brief Vertex comparison operator overload
     * @param other vertex
     * @return true if all properties are same, false otherwise
     */
    bool operator==(const Vertex& other) const
    {
        return position == other.position && color == other.color && normal == other.normal && uv == other.uv;
    }

    /**
     * @brief Get all vertex attributes location and offset
     * @return Array containing vertex attributes info
     */
    static DynamicArray<VertexAttributeDescription> getVertexAttributesDescription()
    {
        DynamicArray<VertexAttributeDescription> attributesInfo;

        // position
        attributesInfo.push_back({ 0, offsetof(Vertex, position), VertexAttributeFormat ::X32Y32Z32_FLOAT });

        // color
        attributesInfo.push_back({ 1, offsetof(Vertex, color), VertexAttributeFormat ::X32Y32Z32_FLOAT });

        // normal
        attributesInfo.push_back({ 2, offsetof(Vertex, normal), VertexAttributeFormat ::X32Y32Z32_FLOAT });

        // uv
        attributesInfo.push_back({ 3, offsetof(Vertex, uv), VertexAttributeFormat ::X32Y32_FLOAT });

        return attributesInfo;
    }
};
} // namespace dusk