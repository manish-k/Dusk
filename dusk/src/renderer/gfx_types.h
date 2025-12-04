#pragma once

#include "dusk.h"
#include "texture.h"
#include "gfx_enums.h"
#include "gfx_buffer.h"
#include "image.h"

namespace dusk
{
struct GfxRenderingAttachment
{
    GfxTexture*       texture    = nullptr;
    VkClearValue      clearValue = {};
    GfxLoadOperation  loadOp     = GfxLoadOperation::DontCare;
    GfxStoreOperation storeOp    = GfxStoreOperation::Store;
};

struct GfxMeshInstanceData
{
    glm::mat4 modelMat   = { 1.f };
    glm::mat4 normalMat  = { 1.f };
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
} // namespace dusk