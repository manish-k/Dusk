#include "render_graph.h"

namespace dusk
{
void RenderGraph::addPass(
    const std::string&     passName,
    RecordCmdBuffFunction& recordFn)
{
    DASSERT(!m_addedPasses.has(passName));

    // add render node
    m_passes.push_back({ passName, recordFn });
}

void RenderGraph::setPassContext(const std::string& passName, const VkGfxRenderPassContext& ctx)
{
    m_passContexts[passName] = ctx;
}

void RenderGraph::execute(FrameData& frameData)
{
    for (auto& pass : m_passes)
    {
        VkGfxRenderPassContext& renderCtx = m_passContexts[pass.name];

        renderCtx.cmdBuffer               = frameData.commandBuffer;
        renderCtx.extent                  = { frameData.width, frameData.height };

        renderCtx.begin();
        pass.recordFn(frameData, renderCtx);
        renderCtx.end();
    }
}
} // namespace dusk