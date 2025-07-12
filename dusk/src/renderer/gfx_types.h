#pragma once

#include "dusk.h"
#include "texture.h"
#include "gfx_enums.h"
#include "gfx_buffer.h"

namespace dusk
{
struct GfxRenderingAttachment
{
    GfxTexture*        texture    = nullptr;
    VkClearValue      clearValue = {};
    GfxLoadOperation  loadOp     = GfxLoadOperation::DontCare;
    GfxStoreOperation storeOp    = GfxStoreOperation::Store;
};
} // namespace dusk