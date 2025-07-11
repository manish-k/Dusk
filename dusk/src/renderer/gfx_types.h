#pragma once

#include "dusk.h"
#include "texture.h"

namespace dusk
{
struct GfxRenderingAttachment
{
    Texture2D*        texture    = nullptr;
    VkClearValue      clearValue = {};
    GfxLoadOperation  loadOp     = GfxLoadOperation::DontCare;
    GfxStoreOperation storeOp    = GfxStoreOperation::Store;
};
} // namespace dusk