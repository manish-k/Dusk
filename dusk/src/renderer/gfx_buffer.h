#pragma once

#include "backend/vulkan/vk_allocator.h"

namespace dusk
{
enum GfxBufferUsageFlags : uint32_t
{
    TransferSource = 0x00000001,
    TransferTarget = 0x00000002,
    UniformBuffer  = 0x00000004,
    VertexBuffer   = 0x00000008,
    IndexBuffer    = 0x00000010
};

enum GfxBufferMemoryTypeFlags : uint32_t
{
    PersistentlyMapped    = 0x00000001,
    HostSequentialWrite   = 0x00000002,
    HostRandomAccess      = 0x00000004,
    DedicatedDeviceMemory = 0x00000008
};

struct GfxBufferParams
{
    size_t   sizeInBytes = 0u;
    uint32_t usage;
    uint32_t memoryType;
    size_t   alignment;
};
} // namespace dusk