#pragma once

#include "dusk.h"
#include "texture.h"
#include "gfx_enums.h"
#include "gfx_buffer.h"
#include "image.h"

namespace dusk
{
#define DEFAULT_COLOR_CLEAR_VALUE   { 0.f, 0.f, 0.f, 1.f }
#define DEFAULT_DEPTH_STENCIL_VALUE { 1.0f, 0 }

struct GfxBoundingBoxData
{
    glm::vec3 center  = {};
    glm::vec3 extents = {};
};

struct GfxMeshData
{
    uint32_t indexCount   = 0u;
    uint32_t firstIndex   = 0u;
    int32_t  vertexOffset = 0u;
};

struct GfxRenderables
{
    DynamicArray<glm::mat4>          modelMatrices  = {};
    DynamicArray<glm::mat4>          normalMatrices = {};
    DynamicArray<GfxBoundingBoxData> boundingBoxes  = {};
    DynamicArray<uint32_t>           meshIds        = {};
    DynamicArray<uint32_t>           materialIds    = {};
};

// TODO:: make it std430 aligned
struct alignas(16) GfxMeshInstanceData
{
    glm::mat4 modelMat   = { 1.f };
    glm::mat4 normalMat  = { 1.f };
    glm::vec3 center     = {};
    uint32_t  meshId     = 0u;
    glm::vec3 extents    = {};
    uint32_t  materialId = 0u;
};

struct GfxIndirectDrawCommand
{
    uint32_t vertexCount   = 0u;
    uint32_t instanceCount = 0u;
    uint32_t firstVertex   = 0u;
    uint32_t firstInstance = 0u;
};

struct GfxIndexedIndirectDrawCommand
{
    uint32_t indexCount    = 0u;
    uint32_t instanceCount = 0u;
    uint32_t firstIndex    = 0u;
    int32_t  vertexOffset  = 0u;
    uint32_t firstInstance = 0u;
};

struct GfxIndexedIndirectDrawCount
{
    uint32_t count = 0u;
};
} // namespace dusk