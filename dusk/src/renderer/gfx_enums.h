#pragma once

namespace dusk
{
enum class VertexAttributeFormat
{
    X32Y32_FLOAT,
    X32Y32Z32_FLOAT,
    X32Y32Z32W32_FLOAT
};

enum class GfxLoadOperation : uint8_t
{
    DontCare,
    Load,
    Clear,
};

enum class GfxStoreOperation : uint8_t
{
    DontCare,
    Store,
    None,
};

} // namespace dusk