#pragma once

namespace dusk
{
class RenderAPI
{
public:
    enum class API
    {
        VULKAN,
        DX12,
        OPENGL
    };
};
} // namespace dusk