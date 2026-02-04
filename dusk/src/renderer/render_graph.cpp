#include "render_graph.h"

#include "renderer/frame_data.h"
#include "renderer/gfx_types.h"

#include "backend/vulkan/vk.h"
#include "backend/vulkan/vk_types.h"

#include "debug/profiler.h"

#include <bit>

namespace dusk
{

void RenderGraph::addReadResource(
    uint32_t         passId,
    RGImageResource& resource,
    uint32_t         version)
{
    auto& pass = m_passes[passId];
    if (m_imageResourceStates[resource.texture->id].versions.size() > 0)
    {
        uint32_t writer = m_imageResourceStates[resource.texture->id].versions[version];
        pass.readTextureResources.push_back({ (void*)&resource, (1ULL << writer) });
        return;
    }
    pass.readTextureResources.push_back({ (void*)&resource, 0ULL });
}

void RenderGraph::addReadResource(uint32_t passId, RGBufferResource& resource)
{
    auto& pass = m_passes[passId];
    pass.readBufferResources.push_back({ (void*)&resource, 0u });
}

uint32_t RenderGraph::addWriteResource(uint32_t passId, RGImageResource& resource)
{
    auto&    pass       = m_passes[passId];
    auto&    versions   = m_imageResourceStates[resource.texture->id].versions;
    uint32_t newVersion = static_cast<uint32_t>(versions.size());
    versions.push_back(passId);
    pass.writeTextureResources.push_back({ (void*)&resource, (1ULL << passId) });
    return newVersion;
}

uint32_t RenderGraph::addWriteResource(uint32_t passId, RGBufferResource& resource)
{
    auto& pass = m_passes[passId];
    pass.writeBufferResources.push_back({ (void*)&resource, 0u });
    return 0u;
}

void RenderGraph::addDepthResource(uint32_t passId, RGImageResource& resource)
{
    auto& pass         = m_passes[passId];
    pass.depthResource = &resource;
}

void RenderGraph::markAsCompute(uint32_t passId)
{
    auto& pass     = m_passes[passId];
    pass.isCompute = true;
}

void RenderGraph::setMulitView(uint32_t passId, uint32_t mask, uint32_t numLayers)
{
    auto& pass      = m_passes[passId];
    pass.viewMask   = mask;
    pass.layerCount = numLayers;
}

RenderGraph::RenderGraph()
{
    m_passes.reserve(MAX_RENDER_GRAPH_PASSES);
    m_passExecutionOrder.reserve(MAX_RENDER_GRAPH_PASSES);
    m_inEdgesBitsets.reserve(MAX_RENDER_GRAPH_PASSES);
    m_outEdgesBitsets.reserve(MAX_RENDER_GRAPH_PASSES);
}

uint32_t RenderGraph::addPass(
    const std::string&           passName,
    const RecordCmdBuffFunction& recordFn)
{
    DASSERT(!m_addedPasses.has(passName));

    // add render node
    auto passId = static_cast<uint32_t>(m_passes.size());
    m_passes.push_back({ passName, passId, recordFn });

    return passId;
}

void RenderGraph::execute(const FrameData& frameData)
{
    DASSERT(m_passes.size() <= MAX_RENDER_GRAPH_PASSES, "Currently maximum supported passes in a graph is 64");

    buildDependencyGraph();
    buildExecutionOrder();
    generateLoadStoreOps();

    uint32_t passCount = static_cast<uint32_t>(m_passExecutionOrder.size());
    for (uint32_t passIdx = 0u; passIdx < passCount; ++passIdx)
    {
        auto& pass = m_passes[m_passExecutionOrder[passIdx]];
        //DUSK_DEBUG("executing pass: {}", pass.name);

        if (pass.isCompute)
        {
            vkdebug::cmdBeginLabel(frameData.commandBuffer, pass.name.c_str(), glm::vec4(0.7f, 0.7f, 0.f, 0.f));
            pass.recordFn(frameData);
            vkdebug::cmdEndLabel(frameData.commandBuffer);
        }
        else
        {
            vkdebug::cmdBeginLabel(frameData.commandBuffer, pass.name.c_str(), glm::vec4(0.7f, 0.7f, 0.f, 0.f));
            beginPass(frameData, pass);
            pass.recordFn(frameData);
            endPass(frameData);
            vkdebug::cmdEndLabel(frameData.commandBuffer);
        }
    }
}

void RenderGraph::buildDependencyGraph()
{
    // TODO: Add following validations and error reporting:
    //  cycle detection and reporting
    //  depth resources intent should be explicitly defined

    uint32_t nodeCount = static_cast<uint32_t>(m_passes.size());

    m_inEdgesBitsets.resize(nodeCount, 0ULL);
    m_outEdgesBitsets.resize(nodeCount, 0ULL);

    // update readers/writers of all resources
    for (uint32_t nodeIdx = 0; nodeIdx < nodeCount; ++nodeIdx)
    {
        // TODO:: RGNode struct is not cache-line friendly
        const auto& pass = m_passes[nodeIdx];

        for (auto& readTexRes : pass.readTextureResources)
        {
            ((RGImageResource*)readTexRes.ptr)->readers |= (1ULL << nodeIdx);
        }

        for (auto& writeTexRes : pass.writeTextureResources)
        {
            ((RGImageResource*)writeTexRes.ptr)->writers |= (1ULL << nodeIdx);
        }

        for (auto& readBufRes : pass.readBufferResources)
        {
            ((RGBufferResource*)readBufRes.ptr)->readers |= (1ULL << nodeIdx);
        }

        for (auto& writeBufRes : pass.writeBufferResources)
        {
            ((RGBufferResource*)writeBufRes.ptr)->writers |= (1ULL << nodeIdx);
        }
    }

    // add incoming edges based on readers/writers
    // writer -> reader edges (RAW hazards)
    // writer -> reader -> writer edges
    for (uint32_t nodeIdx = 0; nodeIdx < nodeCount; ++nodeIdx)
    {
        // TODO:: RGNode struct is not cache-line friendly
        const auto& pass = m_passes[nodeIdx];
        // DUSK_DEBUG("Creating edges for pass: {}({})", pass.name, pass.index);

        for (const auto& readTexRes : pass.readTextureResources)
        {
            RGImageResource* resource = (RGImageResource*)readTexRes.ptr;
            // DUSK_DEBUG("{} - pass:{}, last writer mask: {}", resource->name, std::countr_zero(readTexRes.lastWriterMask), readTexRes.lastWriterMask);
            m_inEdgesBitsets[nodeIdx] |= readTexRes.lastWriterMask;
            // DUSK_DEBUG("in edges - {}", m_inEdgesBitsets[nodeIdx]);
        }

        // for (const auto& writeTexRes : pass.writeTextureResources)
        //{
        //     m_inEdgesBitsets[nodeIdx] |= writeTexRes->writers;
        // }

        // TODO:: version for buffers is not implemented yet
        for (const auto& readBufRes : pass.readBufferResources)
        {
            m_inEdgesBitsets[nodeIdx] |= ((RGBufferResource*)readBufRes.ptr)->writers;
        }

        for (const auto& writeBufRes : pass.writeBufferResources)
        {
            m_inEdgesBitsets[nodeIdx] |= ((RGBufferResource*)writeBufRes.ptr)->writers;
        }

        // remove self-loop if any
        m_inEdgesBitsets[nodeIdx] &= ~(1ULL << nodeIdx);
    }

    // update outgoing edges based on incoming edges
    for (uint32_t nodeIdx = 0; nodeIdx < nodeCount; ++nodeIdx)
    {
        uint64_t inEdges = m_inEdgesBitsets[nodeIdx];
        while (inEdges)
        {
            // we will consider node based on least significant set bit
            uint32_t inNode = std::countr_zero(inEdges);

            // remove from incoming edge list
            inEdges &= inEdges - 1;

            // add outgoing edge from inNode to current node
            m_outEdgesBitsets[inNode] |= (1ULL << nodeIdx);
        }
    }
}

void RenderGraph::buildExecutionOrder()
{
    uint32_t nodeCount = static_cast<uint32_t>(m_passes.size());

    // represent all nodes as bitset eg: for 5 nodes 0b11111
    uint64_t allNodesBitset = (1ULL << nodeCount) - 1ULL;

    // track initial zero in-degree nodes
    uint64_t zeroInDegreeNodesBitset = 0ULL;
    for (uint32_t nodeIdx = 0; nodeIdx < nodeCount; ++nodeIdx)
    {
        zeroInDegreeNodesBitset |= (uint64_t)(!m_inEdgesBitsets[nodeIdx]) << nodeIdx;
    }

    // Kahn's algorithm for topological sorting
    // zeroInDegreeNodesBitset will act as a queue
    // TODO:: even faster version exists using SoA 64-Node Tile Graph where edges are stored in a transposed manner
    while (zeroInDegreeNodesBitset)
    {
        // TODO:: should we consider built-in functions for trailing zero counting?
        uint32_t nodeIdx = std::countr_zero(zeroInDegreeNodesBitset);
        uint32_t nodeBit = (1U << nodeIdx);

        // pop the node from zero in-degree bitset
        zeroInDegreeNodesBitset &= zeroInDegreeNodesBitset - 1ULL;

        // remove node from all nodes bitset for concluding its process
        allNodesBitset &= ~nodeBit;

        // add node to execution order
        m_passExecutionOrder.push_back(nodeIdx);

        uint64_t outEdges = m_outEdgesBitsets[nodeIdx];

        // decrease in-degree of all other nodes via outgoing edges from current node
        while (outEdges)
        {
            uint32_t otherNodeIdx = std::countr_zero(outEdges);

            // remove outgoing edge
            outEdges &= outEdges - 1;

            uint64_t beforeState = m_inEdgesBitsets[otherNodeIdx];

            // remove incoming edge
            m_inEdgesBitsets[otherNodeIdx] &= ~nodeBit;

            uint64_t newState = m_inEdgesBitsets[otherNodeIdx];

            // update zero in-degree nodes bitset if other node has become zero
            // in-degree after removing the edge
            zeroInDegreeNodesBitset |= (uint64_t)(beforeState != 0ULL && newState == 0ULL) << otherNodeIdx;
        }
    }
}

void RenderGraph::generateLoadStoreOps()
{
    uint32_t nodeCount = static_cast<uint32_t>(m_passExecutionOrder.size());
    for (uint32_t execIdx = 0u; execIdx < nodeCount; ++execIdx)
    {
        uint32_t passIdx = m_passExecutionOrder[execIdx];
        auto&    pass    = m_passes[passIdx];

        for (uint32_t resIdx = 0u; resIdx < pass.writeTextureResources.size(); ++resIdx)
        {
            const RGImageResource* resource = (RGImageResource*)pass.writeTextureResources[resIdx].ptr;
            uint32_t               resId    = resource->texture->id;

            if (!m_imageResourceStates.has(resId))
            {
                m_imageResourceStates[resId] = {};
            }

            auto& state = m_imageResourceStates[resId];

            if (state.firstWriter == -1)
            {
                state.firstWriter                          = passIdx;
                pass.resourceLoadStoreStates[resId].loadOp = GfxLoadOperation::Clear; // TODO:: make configurable
            }
            else
            {
                // already written before, so load existing content
                pass.resourceLoadStoreStates[resId].loadOp = GfxLoadOperation::Load;
            }
        }

        // for read-only resources
        for (uint32_t resIdx = 0u; resIdx < pass.readTextureResources.size(); ++resIdx)
        {
            const RGImageResource* resource = (RGImageResource*)pass.readTextureResources[resIdx].ptr;
            uint32_t               resId    = resource->texture->id;

            if (!m_imageResourceStates.has(resId))
            {
                m_imageResourceStates[resId] = {};
            }

            if (resource->writers == 0)
            {
                // resource is read-only throughout the graph, so we can assume its content is valid
                pass.resourceLoadStoreStates[resId].loadOp = GfxLoadOperation::Load;
            }
        }
    }
}

void RenderGraph::beginPass(const FrameData& frameData, RGNode& pass)
{
    DUSK_PROFILE_SECTION("begin_pass");
    VkCommandBuffer cmdBuffer = frameData.commandBuffer;

    for (const auto& attachment : pass.readTextureResources)
    {
        // transition to shader read layout
        const RGImageResource* resource = (RGImageResource*)attachment.ptr;
        resource->texture->recordTransitionLayout(cmdBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    VkRenderingInfo                         renderingInfo        = {};
    DynamicArray<VkFormat>                  colorFormats         = {};
    DynamicArray<VkRenderingAttachmentInfo> colorAttachmentInfos = {};
    VkRenderingAttachmentInfo               depthAttachmentInfo  = {};

    // depth attachment
    bool     useDepth       = pass.depthResource.has_value();
    uint32_t depthTextureId = 0u;

    if (useDepth)
    {
        auto* depthTexture              = pass.depthResource.value()->texture;
        depthTextureId                  = depthTexture->id;
        const auto& lsState             = pass.resourceLoadStoreStates[depthTextureId];

        depthAttachmentInfo             = { VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
        depthAttachmentInfo.imageView   = depthTexture->imageView;
        depthAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
        depthAttachmentInfo.loadOp      = vulkan::getLoadOp(lsState.loadOp);
        depthAttachmentInfo.storeOp     = vulkan::getStoreOp(lsState.storeOp);
        depthAttachmentInfo.clearValue  = DEFAULT_DEPTH_STENCIL_VALUE;

        depthTexture->recordTransitionLayout(cmdBuffer, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    }

    for (const auto& attachment : pass.writeTextureResources)
    {
        const RGImageResource* resource = (RGImageResource*)attachment.ptr;

        if (useDepth && resource->texture->id == depthTextureId)
        {
            // already handled as depth attachment
            continue;
        }

        DASSERT(resource != nullptr, "valid texture is required");

        auto*       texture = resource->texture;
        const auto& lsState = pass.resourceLoadStoreStates[texture->id];

        // rendering info
        VkRenderingAttachmentInfo colorAttachment { VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
        colorAttachment.imageView   = texture->imageView;
        colorAttachment.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
        colorAttachment.loadOp      = vulkan::getLoadOp(lsState.loadOp);
        colorAttachment.storeOp     = vulkan::getStoreOp(lsState.storeOp);
        colorAttachment.clearValue  = DEFAULT_COLOR_CLEAR_VALUE;
        colorAttachmentInfos.push_back(colorAttachment);

        colorFormats.push_back(texture->format);

        // transition to color out layout
        texture->recordTransitionLayout(cmdBuffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    }

    renderingInfo                      = { VK_STRUCTURE_TYPE_RENDERING_INFO };
    renderingInfo.layerCount           = 1;
    renderingInfo.renderArea           = { { 0, 0 }, { frameData.width, frameData.height } };
    renderingInfo.colorAttachmentCount = static_cast<uint32_t>(colorAttachmentInfos.size());
    renderingInfo.pColorAttachments    = colorAttachmentInfos.data();
    renderingInfo.pDepthAttachment     = useDepth ? &depthAttachmentInfo : nullptr;
    renderingInfo.flags                = VK_RENDERING_CONTENTS_SECONDARY_COMMAND_BUFFERS_BIT;

    if (pass.viewMask > 0)
    {
        renderingInfo.viewMask   = pass.viewMask;
        renderingInfo.layerCount = pass.layerCount;
    }

    vkCmdBeginRendering(cmdBuffer, &renderingInfo);

    VkViewport viewport {};
    viewport.x        = 0.0f;
    viewport.y        = 0.0f;
    viewport.width    = static_cast<float>(frameData.width);
    viewport.height   = static_cast<float>(frameData.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);

    VkRect2D scissor {};
    scissor.offset = { 0, 0 };
    scissor.extent = { frameData.width, frameData.height };
    vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);
}

void RenderGraph::endPass(const FrameData& frameData)
{
    DUSK_PROFILE_SECTION("end_pass");

    vkCmdEndRendering(frameData.commandBuffer);
}

} // namespace dusk