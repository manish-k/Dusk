#pragma once

#include "dusk.h"
#include "vk.h"
#include "frame_data.h"

#include "backend/vulkan/vk_pass.h"

#include <string>
#include <functional>

namespace dusk
{
using RecordCmdBuffFunction = std::function<void(
    FrameData&, 
    VkGfxRenderPassContext&)>;

struct RenderGraphNode
{
    std::string           name;
    RecordCmdBuffFunction recordFn;
};

class RenderGraph
{
public:
    void addPass(
        const std::string&           passName,
        const RecordCmdBuffFunction& recordFn);
    void setPassContext(const std::string& passName, const VkGfxRenderPassContext& ctx);

    void execute(FrameData& frameData);

private:
    DynamicArray<RenderGraphNode>                m_passes;
    HashMap<std::string, VkGfxRenderPassContext> m_passContexts;
    HashSet<std::string>                         m_addedPasses;
};
} // namespace dusk