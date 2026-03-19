#include "render_graph.h"

#include "engine.h"
#include "stats_recorder.h"

#include "renderer/frame_data.h"
#include "renderer/gfx_types.h"

#include "backend/vulkan/vk.h"
#include "backend/vulkan/vk_types.h"

#include "debug/profiler.h"

#include <bit>
#include <iostream>
#include <fstream>
#include <algorithm>

namespace dusk
{

void RenderGraph::addReadResource(
    uint32_t         passId,
    RGImageResource& resource,
    uint32_t         version)
{
    auto&       pass              = m_passes[passId];
    auto        textureId         = resource.texture->id;
    const auto& availableVersions = m_imageVersions[textureId];

    if (availableVersions.size() > 0)
    {
        DASSERT(version < availableVersions.size(), "provided version doesn't exists");

        uint32_t writer = availableVersions[version];
        pass.readTextureResources.push_back({ (void*)&resource, (1ULL << writer) });
        return;
    }

    // add current layout to exec state
    m_imageExecStates[resource.texture->id].layout = resource.texture->currentLayout;

    pass.readTextureResources.push_back({ (void*)&resource, 0ULL });
}

void RenderGraph::addReadResource(
    uint32_t          passId,
    RGBufferResource& resource,
    uint32_t          version)
{
    auto&       pass              = m_passes[passId];
    auto        bufferId          = resource.buffer->getId();
    const auto& availableVersions = m_bufferVersions[bufferId];

    if (availableVersions.size() > 0)
    {
        DASSERT(version < availableVersions.size(), "provided version doesn't exists");

        uint32_t writer = availableVersions[version];
        pass.readBufferResources.push_back({ (void*)&resource, (1ULL << writer) });
        return;
    }
    pass.readBufferResources.push_back({ (void*)&resource, 0ULL });
}

uint32_t RenderGraph::addWriteResource(uint32_t passId, RGImageResource& resource)
{
    auto&    pass              = m_passes[passId];
    auto&    availableVersions = m_imageVersions[resource.texture->id];
    uint32_t newVersion        = static_cast<uint32_t>(availableVersions.size());
    availableVersions.push_back(passId);
    pass.writeTextureResources.push_back({ (void*)&resource, (1ULL << passId) });
    return newVersion;
}

uint32_t RenderGraph::addWriteResource(uint32_t passId, RGBufferResource& resource)
{
    auto&    pass              = m_passes[passId];
    auto&    availableVersions = m_bufferVersions[resource.buffer->getId()];
    uint32_t newVersion        = static_cast<uint32_t>(availableVersions.size());
    availableVersions.push_back(passId);
    pass.writeBufferResources.push_back({ (void*)&resource, (1ULL << passId) });
    return newVersion;
}

uint32_t RenderGraph::addDepthResource(
    uint32_t         passId,
    RGImageResource& depthResource,
    uint32_t         version)
{
    auto& pass         = m_passes[passId];
    pass.depthResource = &depthResource;

    auto& versions     = m_imageVersions[depthResource.texture->id];

    if (!versions.empty() && versions.size() > version)
    {
        uint32_t writer = versions[version];
        pass.readTextureResources.push_back({ (void*)&depthResource, (1ULL << writer) });
    }
    else
    {
        pass.readTextureResources.push_back({ (void*)&depthResource, 0ULL });
    }

    uint32_t newVersion = static_cast<uint32_t>(versions.size());
    versions.push_back(passId);
    pass.writeTextureResources.push_back({ (void*)&depthResource, (1ULL << passId) });

    return newVersion;
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

RenderGraph::RenderGraph(VulkanContext vkCtx) :
    m_vkContext(vkCtx)
{
    m_passes.reserve(MAX_RENDER_GRAPH_PASSES);
    m_passExecutionOrder.reserve(MAX_RENDER_GRAPH_PASSES);
    m_inEdgesBitsets.reserve(MAX_RENDER_GRAPH_PASSES);
    m_outEdgesBitsets.reserve(MAX_RENDER_GRAPH_PASSES);
}

uint32_t RenderGraph::addPass(
    const std::string&           passName,
    RGQueueFamilyType            targetQueue,
    const RecordCmdBuffFunction& recordFn)
{
    // add render node
    auto passId = static_cast<uint32_t>(m_passes.size());
    m_passes.emplace_back(passName, passId, targetQueue, recordFn);

    return passId;
}

void RenderGraph::execute(const FrameData& frameData)
{
    DASSERT(m_passes.size() <= MAX_RENDER_GRAPH_PASSES, "Currently maximum supported passes in a graph is 64");

    buildDependencyGraph();
    buildExecutionOrder();
    buildResourcesStates();
    buildWaitSignalValues();
    buildSubmissionBatches();

    auto*    statsRecorder = StatsRecorder::get();

    uint32_t passCount     = static_cast<uint32_t>(m_passExecutionOrder.size());
    for (uint32_t passExecutionIdx = 0u; passExecutionIdx < passCount; ++passExecutionIdx)
    {
        auto& pass = m_passes[m_passExecutionOrder[passExecutionIdx]];

        statsRecorder->beginPass(frameData.commandBuffer, pass.name, passExecutionIdx);

        insertPrePassBarriers(frameData, pass);

        // DUSK_DEBUG("executing pass: {}", pass.name);

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

        insertPostPassBarriers(frameData, pass);

        statsRecorder->endPass(frameData.commandBuffer, passExecutionIdx);
    }
}

void RenderGraph::buildDependencyGraph()
{
    // TODO: Add following validations and error reporting:
    //  cycle detection and reporting
    //  depth resources intent should be explicitly defined
    // TODO:: Hazard types assertions and reporting (RAW, WAW, WAR)
    // TODO: graph prunning

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
    for (uint32_t nodeIdx = 0; nodeIdx < nodeCount; ++nodeIdx)
    {
        // TODO:: RGNode struct is not cache-line friendly
        const auto& pass = m_passes[nodeIdx];
        // DUSK_DEBUG("Creating edges for pass: {}({})", pass.name, pass.index);

        for (const auto& readTexRes : pass.readTextureResources)
        {
            m_inEdgesBitsets[nodeIdx] |= readTexRes.lastWriterMask;
        }

        for (const auto& readBuffRes : pass.readBufferResources)
        {
            m_inEdgesBitsets[nodeIdx] |= readBuffRes.lastWriterMask;
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

    // copying, avoiding modifications on original array
    DynamicArray<uint64_t> inEdgeBitsets = m_inEdgesBitsets;

    // track initial zero in-degree nodes
    uint64_t zeroInDegreeNodesBitset = 0ULL;
    for (uint32_t nodeIdx = 0; nodeIdx < nodeCount; ++nodeIdx)
    {
        zeroInDegreeNodesBitset |= (uint64_t)(!inEdgeBitsets[nodeIdx]) << nodeIdx;
    }

    // Kahn's algorithm for topological sorting
    // zeroInDegreeNodesBitset will act as a queue
    // TODO:: even faster version exists using SoA 64-Node Tile Graph where edges are stored in a transposed manner
    while (zeroInDegreeNodesBitset)
    {
        // TODO:: should we consider built-in functions for trailing zero counting?
        uint32_t nodeIdx = std::countr_zero(zeroInDegreeNodesBitset);
        uint64_t nodeBit = (1ULL << nodeIdx);

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

            uint64_t beforeState = inEdgeBitsets[otherNodeIdx];

            // remove incoming edge
            inEdgeBitsets[otherNodeIdx] &= ~nodeBit;

            uint64_t newState = inEdgeBitsets[otherNodeIdx];

            // update zero in-degree nodes bitset if other node has become zero
            // in-degree after removing the edge
            zeroInDegreeNodesBitset |= (uint64_t)(beforeState != 0ULL && newState == 0ULL) << otherNodeIdx;
        }
    }
}

void RenderGraph::buildResourcesStates()
{
    uint32_t nodeCount = static_cast<uint32_t>(m_passExecutionOrder.size());
    for (uint32_t execIdx = 0u; execIdx < nodeCount; ++execIdx)
    {
        uint32_t passIdx  = m_passExecutionOrder[execIdx];
        auto&    pass     = m_passes[passIdx];

        bool     useDepth = pass.depthResource.has_value();

        buildWriteImageResourcesState(pass, useDepth);
        buildReadImageResourcesState(pass, useDepth);
        buildWriteBufferResourcesState(pass);
        buildReadBufferResourcesState(pass);
    }
}

void RenderGraph::buildWaitSignalValues()
{
    uint32_t nodeCount = static_cast<uint32_t>(m_passExecutionOrder.size());
    for (uint32_t execIdx = 0u; execIdx < nodeCount; ++execIdx)
    {
        uint32_t passIdx = m_passExecutionOrder[execIdx];
        auto&    pass    = m_passes[passIdx];

        // calculate wait value as OR of all signal values of its dependencies
        uint32_t waitValue = 0u;
        uint64_t deps      = m_inEdgesBitsets[passIdx];

        while (deps)
        {
            uint32_t depIdx = std::countr_zero(deps);

            deps &= deps - 1;
            waitValue = std::max(waitValue, m_passes[depIdx].signalValue);
        }

        pass.waitValue   = waitValue;
        pass.signalValue = execIdx + 1;
    }
}

void RenderGraph::buildSubmissionBatches()
{
    // start with an empty batch
    m_submissionOrder.graphicBatches.push_back({});
    m_submissionOrder.computeBatches.push_back({});

    uint32_t nodeCount           = static_cast<uint32_t>(m_passExecutionOrder.size());
    bool     closeCurrentBatches = false;

    for (uint32_t execIdx = 0u; execIdx < nodeCount; ++execIdx)
    {
        uint32_t         passIdx      = m_passExecutionOrder[execIdx];
        const auto&      pass         = m_passes[passIdx];

        SubmissionBatch& currentBatch = pass.targetQueueFamily == RGQueueFamilyType::Graphics ? m_submissionOrder.graphicBatches.back() : m_submissionOrder.computeBatches.back();
        SubmissionBatch& otherBatch   = pass.targetQueueFamily == RGQueueFamilyType::Graphics ? m_submissionOrder.computeBatches.back() : m_submissionOrder.graphicBatches.back();

        // no cross-queue dependencies, add to current batch
        if (pass.crossQueueDeps == 0)
        {
            currentBatch.passesMask |= (1ULL << passIdx);
            currentBatch.signalValue = std::max(currentBatch.signalValue, pass.signalValue);
            currentBatch.waitValue   = std::max(currentBatch.waitValue, pass.waitValue);
        }
        else
        {
            // and also add all its dependencies to the batch to ensure they are submitted together
            uint64_t deps = pass.crossQueueDeps;
            while (deps)
            {
                uint32_t depIdx = std::countr_zero(deps);
                deps &= deps - 1;

                // check if depeendecy already submitted in previous batch
                if (1ULL << depIdx & m_submissionOrder.batchMask)
                {
                    continue;
                }
                // otherwise dependency must exist in current batch because of topological order
                else
                {
                    DASSERT(otherBatch.passesMask & (1ULL << depIdx), "Critical error in render graph's execution order, dependency doesn't exist in existing batch.");

                    closeCurrentBatches = true;
                }
            }
        }

        // We will submit the batch containing cross queue depenedent passes and also submit the ongoing batch
        // for current queue because that batch should not wait for the passes with cross-queue dependencies
        if (closeCurrentBatches)
        {
            // commit passes in the submitted batch mask
            m_submissionOrder.batchMask |= m_submissionOrder.graphicBatches.back().passesMask;
            m_submissionOrder.batchMask |= m_submissionOrder.computeBatches.back().passesMask;

            // close current batches and start new batch with current pass
            m_submissionOrder.graphicBatches.push_back({});
            m_submissionOrder.computeBatches.push_back({});

            if (pass.targetQueueFamily == RGQueueFamilyType::Graphics)
            {
                m_submissionOrder.graphicBatches.back().passesMask |= (1ULL << passIdx);
            }
            else
            {
                m_submissionOrder.computeBatches.back().passesMask |= (1ULL << passIdx);
            }

            closeCurrentBatches = false;
        }
    }
}

void RenderGraph::buildWriteImageResourcesState(RGNode& pass, bool useDepth)
{
    for (uint32_t resIdx = 0u; resIdx < pass.writeTextureResources.size(); ++resIdx)
    {
        const auto* resource = (RGImageResource*)pass.writeTextureResources[resIdx].ptr;
        auto        resId    = resource->texture->id;
        auto&       state    = m_imageExecStates[resId];

        // deduce layout, access and stage flags
        VkImageLayout         newLayout;
        VkPipelineStageFlags2 newStage;
        VkAccessFlags2        newAccess;

        // deduce load store states
        pass.resourceLoadStoreStates[resId].loadOp = GfxLoadOperation::Load;
        if (state.firstWriter == -1)
        {
            state.firstWriter                          = pass.index;
            pass.resourceLoadStoreStates[resId].loadOp = GfxLoadOperation::Clear; // TODO:: make configurable, Expose per-pass intent
        }

        VkImageAspectFlags imageAspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;

        if (useDepth && resource->texture->usage & DepthStencilTexture)
        {
            // if writing depth buffer
            newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            newStage  = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
            newAccess = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

            // if reading depth buffer in the same pass
            if (resource->readers & (1ULL << pass.index))
            {
                newAccess |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
            }
        }
        else
        {
            newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            newStage  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            newAccess = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

            if (pass.targetQueueFamily == RGQueueFamilyType::Compute)
            {
                newLayout = VK_IMAGE_LAYOUT_GENERAL;
                newStage  = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
                newAccess = VK_ACCESS_2_SHADER_WRITE_BIT;
            }
        }

        if (resource->texture->usage & DepthStencilTexture)
        {
            // if reading depth buffer
            imageAspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
        }

        // TODO:: Need proper reasoning about WAW ownership cases. Currently there is no use case for it.
        // handle ownership transfer for Write -> Write cases across different queue families
        bool needOwnershipTransfer = state.lastWriter != -1
            && state.lastWriter != pass.index
            && state.currentQueueFamily != pass.targetQueueFamily;

        bool addedReleaseBarrier = false;
        bool addedAcquireBarrier = false;

        if (needOwnershipTransfer)
        {
            RGNode& oldPass = m_passes[state.lastWriter];

            // release on old queue family
            VkImageMemoryBarrier2 release { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };

            // wait for the old stage to finish all its operation before release
            release.srcStageMask  = state.stage;
            release.srcAccessMask = state.access;

            // dst doesnt' matter for release
            release.dstStageMask                    = VK_PIPELINE_STAGE_2_NONE;
            release.dstAccessMask                   = 0;

            release.oldLayout                       = state.layout;
            release.newLayout                       = state.layout;

            release.srcQueueFamilyIndex             = getQueueFamilyIndex(oldPass.targetQueueFamily);
            release.dstQueueFamilyIndex             = getQueueFamilyIndex(pass.targetQueueFamily);

            release.image                           = resource->texture->image.vkImage;

            release.subresourceRange.aspectMask     = imageAspectFlags;
            release.subresourceRange.baseMipLevel   = 0;
            release.subresourceRange.levelCount     = resource->texture->numMipLevels;
            release.subresourceRange.baseArrayLayer = 0;
            release.subresourceRange.layerCount     = resource->texture->numLayers;

            // oldPass.postImageBarriers.push_back(release);

            pass.crossQueueDeps = 1ULL << state.lastWriter; // ensure submit happens between release and acquire

            addedReleaseBarrier = true;
        }

        // emit barrier if layout transition is needed
        if (state.layout != newLayout)
        {
            VkImageMemoryBarrier2 barrier { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };
            barrier.srcStageMask                    = state.stage;
            barrier.dstStageMask                    = newStage;

            barrier.srcAccessMask                   = state.access;
            barrier.dstAccessMask                   = newAccess;

            barrier.oldLayout                       = state.layout;
            barrier.newLayout                       = newLayout;

            barrier.image                           = resource->texture->image.vkImage;

            barrier.subresourceRange.aspectMask     = imageAspectFlags;
            barrier.subresourceRange.baseMipLevel   = 0;
            barrier.subresourceRange.levelCount     = resource->texture->numMipLevels;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount     = resource->texture->numLayers;

            // merging ownership transfer barrier if needed with layout transition
            if (needOwnershipTransfer)
            {
                const RGNode& oldPass = m_passes[state.lastWriter];

                // barrier.srcQueueFamilyIndex = getQueueFamilyIndex(oldPass.targetQueueFamily);
                // barrier.dstQueueFamilyIndex = getQueueFamilyIndex(pass.targetQueueFamily);

                addedAcquireBarrier = true;
            }

            pass.preImageBarriers.push_back(barrier);
        }

        DASSERT(addedReleaseBarrier == addedAcquireBarrier, "missing either release or acquire barriers");

        state.layout             = newLayout;
        state.stage              = newStage;
        state.access             = newAccess;
        state.currentQueueFamily = pass.targetQueueFamily;
        state.lastWriter         = pass.index;
    }
}

void RenderGraph::buildReadImageResourcesState(RGNode& pass, bool useDepth)
{
    for (uint32_t resIdx = 0u; resIdx < pass.readTextureResources.size(); ++resIdx)
    {
        const RGImageResource* resource = (RGImageResource*)pass.readTextureResources[resIdx].ptr;
        uint32_t               resId    = resource->texture->id;
        auto&                  state    = m_imageExecStates[resId];

        // deduce layout, access and stage flags
        VkImageLayout         newLayout;
        VkPipelineStageFlags2 newStage;
        VkAccessFlags2        newAccess;

        // deduce load store states
        if (resource->writers == 0)
        {
            // resource is read-only throughout the graph, so we can assume its content is valid
            pass.resourceLoadStoreStates[resId].loadOp = GfxLoadOperation::Load;
        }

        VkImageAspectFlags imageAspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
        if (resource->texture->usage & DepthStencilTexture)
        {
            // if reading depth buffer
            imageAspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
        }

        // handle ownership transfer for Write -> Read cases across different queue families
        bool needOwnershipTransfer = state.lastWriter != -1
            && state.lastWriter != pass.index
            && state.currentQueueFamily != pass.targetQueueFamily;

        bool addedReleaseBarrier = false;
        bool addedAcquireBarrier = false;

        if (needOwnershipTransfer)
        {
            RGNode& oldPass = m_passes[state.lastWriter];

            // release on old queue family
            VkImageMemoryBarrier2 release { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };

            // wait for the old stage to finish all its operation before release
            release.srcStageMask  = state.stage;
            release.srcAccessMask = state.access;

            // dst doesnt' matter for release
            release.dstStageMask                    = VK_PIPELINE_STAGE_2_NONE;
            release.dstAccessMask                   = 0;

            release.oldLayout                       = state.layout;
            release.newLayout                       = state.layout;

            release.srcQueueFamilyIndex             = getQueueFamilyIndex(oldPass.targetQueueFamily);
            release.dstQueueFamilyIndex             = getQueueFamilyIndex(pass.targetQueueFamily);

            release.image                           = resource->texture->image.vkImage;

            release.subresourceRange.aspectMask     = imageAspectFlags;
            release.subresourceRange.baseMipLevel   = 0;
            release.subresourceRange.levelCount     = resource->texture->numMipLevels;
            release.subresourceRange.baseArrayLayer = 0;
            release.subresourceRange.layerCount     = resource->texture->numLayers;

            // oldPass.postImageBarriers.push_back(release);

            pass.crossQueueDeps = 1ULL << state.lastWriter; // ensure submit happens between release and acquire

            addedReleaseBarrier = true;
        }

        // default to graphic reading stage
        newStage  = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        newAccess = VK_ACCESS_SHADER_READ_BIT;
        newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        if (useDepth && resource->texture->usage & DepthStencilTexture)
        {
            // handled in write
            continue;
        }
        else
        {
            if (pass.targetQueueFamily == RGQueueFamilyType::Compute)
            {
                newStage  = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
                newAccess = VK_ACCESS_SHADER_READ_BIT;
                newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            }
        }

        // emit barrier if layout transition is needed
        if (state.layout != newLayout)
        {
            VkImageMemoryBarrier2 barrier { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };
            barrier.srcStageMask                    = state.stage;
            barrier.srcAccessMask                   = state.access;

            barrier.dstStageMask                    = newStage;
            barrier.dstAccessMask                   = newAccess;

            barrier.oldLayout                       = state.layout;
            barrier.newLayout                       = newLayout;

            barrier.image                           = resource->texture->image.vkImage;

            barrier.subresourceRange.aspectMask     = imageAspectFlags;
            barrier.subresourceRange.baseMipLevel   = 0;
            barrier.subresourceRange.levelCount     = resource->texture->numMipLevels;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount     = resource->texture->numLayers;

            // merging ownership transfer barrier if needed with layout transition
            if (needOwnershipTransfer)
            {
                const RGNode& oldPass = m_passes[state.lastWriter];

                // barrier.srcQueueFamilyIndex = getQueueFamilyIndex(oldPass.targetQueueFamily);
                // barrier.dstQueueFamilyIndex = getQueueFamilyIndex(pass.targetQueueFamily);

                addedAcquireBarrier = true;
            }

            pass.preImageBarriers.push_back(barrier);
        }

        DASSERT(addedReleaseBarrier == addedAcquireBarrier, "missing either release or acquire barriers");

        state.layout             = newLayout;
        state.stage              = newStage;
        state.access             = newAccess;
        state.currentQueueFamily = pass.targetQueueFamily;
    }
}

void RenderGraph::buildWriteBufferResourcesState(RGNode& pass)
{
    for (uint32_t resIdx = 0u; resIdx < pass.writeBufferResources.size(); ++resIdx)
    {
        const auto* resource = (RGBufferResource*)pass.writeBufferResources[resIdx].ptr;
        auto        resId    = resource->buffer->getId();
        auto&       state    = m_bufferExecStates[resId];

        // deduce stage and access flags
        VkPipelineStageFlags2 newStage  = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
        VkAccessFlags2        newAccess = VK_ACCESS_2_SHADER_WRITE_BIT;

        if (state.firstWriter == -1)
        {
            state.firstWriter = pass.index;
        }

        if (pass.isCompute)
        {
            newStage  = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
            newAccess = VK_ACCESS_2_SHADER_WRITE_BIT;

            // if also being read by the pass
            if (resource->readers & (1ULL << pass.index))
            {
                newAccess |= VK_ACCESS_2_SHADER_READ_BIT;
            }
        }

        bool needOwnershipTransfer = state.lastWriter != -1
            && state.lastWriter != pass.index
            && state.currentQueueFamily != pass.targetQueueFamily;

        bool addedReleaseBarrier = false;
        bool addedAcquireBarrier = false;

        if (needOwnershipTransfer)
        {
            RGNode& oldPass = m_passes[state.lastWriter];

            // release on old queue family
            VkBufferMemoryBarrier2 release { VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2 };

            // wait for the old stage to finish all its operation before release
            release.srcStageMask  = state.stage;
            release.srcAccessMask = state.access;

            // dst doesnt' matter for release
            release.dstStageMask        = VK_PIPELINE_STAGE_2_NONE;
            release.dstAccessMask       = 0;

            release.srcQueueFamilyIndex = getQueueFamilyIndex(oldPass.targetQueueFamily);
            release.dstQueueFamilyIndex = getQueueFamilyIndex(pass.targetQueueFamily);

            release.buffer              = resource->buffer->vkBuffer.buffer;
            release.offset              = 0;
            release.size                = resource->buffer->vkBuffer.sizeInBytes;

            // oldPass.postBufferBarriers.push_back(release);

            pass.crossQueueDeps = 1ULL << state.lastWriter; // ensure submit happens between release and acquire

            addedReleaseBarrier = true;
        }

        // emit barrier if access or stage flags have changed
        if (state.stage != newStage || state.access != newAccess)
        {
            VkBufferMemoryBarrier2 barrier { VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2 };
            barrier.srcStageMask  = state.stage;
            barrier.dstStageMask  = newStage;

            barrier.srcAccessMask = state.access;
            barrier.dstAccessMask = newAccess;

            barrier.buffer        = resource->buffer->vkBuffer.buffer;
            barrier.offset        = 0;
            barrier.size          = resource->buffer->vkBuffer.sizeInBytes;

            if (needOwnershipTransfer)
            {
                const RGNode& oldPass = m_passes[state.lastWriter];

                // barrier.srcQueueFamilyIndex = getQueueFamilyIndex(oldPass.targetQueueFamily);
                // barrier.dstQueueFamilyIndex = getQueueFamilyIndex(pass.targetQueueFamily);

                addedAcquireBarrier = true;
            }

            pass.preBufferBarriers.push_back(barrier);
        }

        DASSERT(addedReleaseBarrier == addedAcquireBarrier, "missing either release or acquire barriers");

        state.stage              = newStage;
        state.access             = newAccess;
        state.lastWriter         = pass.index;
        state.currentQueueFamily = pass.targetQueueFamily;
    }
}

void RenderGraph::buildReadBufferResourcesState(RGNode& pass)
{
    for (uint32_t resIdx = 0u; resIdx < pass.readBufferResources.size(); ++resIdx)
    {
        const auto* resource = (RGBufferResource*)pass.readBufferResources[resIdx].ptr;
        auto        resId    = resource->buffer->getId();
        auto&       state    = m_bufferExecStates[resId];

        if (resource->writers & (1ULL << pass.index)) continue; // already handled in write resource loop

        // deduce stage and access flags
        VkPipelineStageFlags2 newStage  = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
        VkAccessFlags2        newAccess = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;

        if (resource->buffer->usage & GfxBufferUsageFlags::IndirectBuffer)
        {
            newStage  = VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT;
            newAccess = VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT;
        }

        // handle ownership transfer for Write -> Read cases across different queue families
        bool needOwnershipTransfer = state.lastWriter != -1
            && state.lastWriter != pass.index
            && state.currentQueueFamily != pass.targetQueueFamily;

        bool addedReleaseBarrier = false;
        bool addedAcquireBarrier = false;

        if (needOwnershipTransfer)
        {
            RGNode& oldPass = m_passes[state.lastWriter];

            // release on old queue family
            VkBufferMemoryBarrier2 release { VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2 };

            // wait for the old stage to finish all its operation before release
            release.srcStageMask  = state.stage;
            release.srcAccessMask = state.access;

            // dst doesnt' matter for release
            release.dstStageMask        = VK_PIPELINE_STAGE_2_NONE;
            release.dstAccessMask       = 0;

            release.srcQueueFamilyIndex = getQueueFamilyIndex(oldPass.targetQueueFamily);
            release.dstQueueFamilyIndex = getQueueFamilyIndex(pass.targetQueueFamily);

            release.buffer              = resource->buffer->vkBuffer.buffer; // TODO: WTF is this?
            release.offset              = 0;
            release.size                = resource->buffer->vkBuffer.sizeInBytes;

            // oldPass.postBufferBarriers.push_back(release);

            pass.crossQueueDeps = 1ULL << state.lastWriter; // ensure submit happens between release and acquire

            addedReleaseBarrier = true;
        }

        // emit barrier if access or stage flags have changed
        if (state.stage != newStage || state.access != newAccess)
        {
            VkBufferMemoryBarrier2 barrier { VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2 };
            barrier.srcStageMask  = state.stage;
            barrier.dstStageMask  = newStage;

            barrier.srcAccessMask = state.access;
            barrier.dstAccessMask = newAccess;

            barrier.buffer        = resource->buffer->vkBuffer.buffer; // TODO: WTF is this?
            barrier.offset        = 0;
            barrier.size          = resource->buffer->vkBuffer.sizeInBytes;

            if (needOwnershipTransfer)
            {
                const RGNode& oldPass = m_passes[state.lastWriter];

                // barrier.srcQueueFamilyIndex = getQueueFamilyIndex(oldPass.targetQueueFamily);
                // barrier.dstQueueFamilyIndex = getQueueFamilyIndex(pass.targetQueueFamily);

                addedAcquireBarrier = true;
            }

            pass.preBufferBarriers.push_back(barrier);
        }

        DASSERT(addedReleaseBarrier == addedAcquireBarrier, "missing either release or acquire barriers");

        state.stage              = newStage;
        state.access             = newAccess;
        state.currentQueueFamily = pass.targetQueueFamily;
    }
}

void RenderGraph::insertPrePassBarriers(const FrameData& frameData, const RGNode& pass) const
{
    VkCommandBuffer  cmdBuffer = frameData.commandBuffer;

    VkDependencyInfo dependencyInfo { VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
    dependencyInfo.imageMemoryBarrierCount  = static_cast<uint32_t>(pass.preImageBarriers.size());
    dependencyInfo.pImageMemoryBarriers     = pass.preImageBarriers.data();
    dependencyInfo.bufferMemoryBarrierCount = static_cast<uint32_t>(pass.preBufferBarriers.size());
    dependencyInfo.pBufferMemoryBarriers    = pass.preBufferBarriers.data();
    vkCmdPipelineBarrier2(cmdBuffer, &dependencyInfo);
}

void RenderGraph::insertPostPassBarriers(const FrameData& frameData, const RGNode& pass) const
{
    VkCommandBuffer  cmdBuffer = frameData.commandBuffer;

    VkDependencyInfo dependencyInfo { VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
    dependencyInfo.imageMemoryBarrierCount  = static_cast<uint32_t>(pass.postImageBarriers.size());
    dependencyInfo.pImageMemoryBarriers     = pass.postImageBarriers.data();
    dependencyInfo.bufferMemoryBarrierCount = static_cast<uint32_t>(pass.postBufferBarriers.size());
    dependencyInfo.pBufferMemoryBarriers    = pass.postBufferBarriers.data();
    vkCmdPipelineBarrier2(cmdBuffer, &dependencyInfo);
}

void RenderGraph::beginPass(const FrameData& frameData, RGNode& pass) const
{
    DUSK_PROFILE_SECTION("begin_pass");
    VkCommandBuffer cmdBuffer = frameData.commandBuffer;

    // TODO:: VkRenderingAttachmentInfo can be prepared earlier in buildResourceStates
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
    }

    renderingInfo                      = { VK_STRUCTURE_TYPE_RENDERING_INFO };
    renderingInfo.layerCount           = 1;
    renderingInfo.renderArea           = { { 0, 0 }, { frameData.width, frameData.height } };
    renderingInfo.colorAttachmentCount = static_cast<uint32_t>(colorAttachmentInfos.size());
    renderingInfo.pColorAttachments    = colorAttachmentInfos.data();
    renderingInfo.pDepthAttachment     = useDepth ? &depthAttachmentInfo : nullptr;

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

void RenderGraph::endPass(const FrameData& frameData) const
{
    DUSK_PROFILE_SECTION("end_pass");

    vkCmdEndRendering(frameData.commandBuffer);
}

void RenderGraph::dumpDebugGraph(const std::string& path) const
{
    DASSERT(m_passExecutionOrder.size() > 0);

    HashMap<uint32_t, uint32_t> passIdxtoOrderMap = {};
    DebugGraph                  graph             = {};

    for (uint32_t orderIdx = 0u; orderIdx < m_passExecutionOrder.size(); ++orderIdx)
    {
        passIdxtoOrderMap[m_passExecutionOrder[orderIdx]] = orderIdx;
    }

    for (uint32_t orderIdx = 0u; orderIdx < m_passExecutionOrder.size(); ++orderIdx)
    {
        const auto&      pass = m_passes[m_passExecutionOrder[orderIdx]];

        DebugGraph::Node node { pass.name };
        node.isCompute = pass.isCompute;

        for (uint32_t readIdx = 0u; readIdx < pass.readTextureResources.size(); ++readIdx)
        {
            const RGImageResource* resource = (RGImageResource*)pass.readTextureResources[readIdx].ptr;
            node.reads.push_back(resource->name);
        }

        for (uint32_t readIdx = 0u; readIdx < pass.readBufferResources.size(); ++readIdx)
        {
            const RGBufferResource* resource = (RGBufferResource*)pass.readBufferResources[readIdx].ptr;
            node.reads.push_back(resource->name);
        }

        for (uint32_t readIdx = 0u; readIdx < pass.writeTextureResources.size(); ++readIdx)
        {
            const RGImageResource* resource = (RGImageResource*)pass.writeTextureResources[readIdx].ptr;
            node.writes.push_back(resource->name);
        }

        for (uint32_t readIdx = 0u; readIdx < pass.writeBufferResources.size(); ++readIdx)
        {
            const RGBufferResource* resource = (RGBufferResource*)pass.writeBufferResources[readIdx].ptr;
            node.writes.push_back(resource->name);
        }

        graph.nodes.emplace_back(node);

        uint64_t inEdges      = m_inEdgesBitsets[pass.index];
        uint32_t passOrderIdx = passIdxtoOrderMap[pass.index];
        while (inEdges)
        {
            uint32_t inNodeIdx = std::countr_zero(inEdges);

            inEdges &= inEdges - 1;

            DebugGraph::Edge e = {};
            e.dst              = passOrderIdx;
            e.src              = passIdxtoOrderMap[inNodeIdx];

            graph.edges.emplace_back(e);
        }
    }

    for (uint32_t batchIdx = 0u; batchIdx < m_submissionOrder.graphicBatches.size(); ++batchIdx)
    {
        DebugGraph::Batch batch  = {};
        uint64_t          passes = m_submissionOrder.graphicBatches[batchIdx].passesMask;

        batch.signalValue        = m_submissionOrder.graphicBatches[batchIdx].signalValue;
        batch.waitValue          = m_submissionOrder.graphicBatches[batchIdx].waitValue;

        if (passes == 0) continue;

        while (passes)
        {
            uint32_t passIdx = std::countr_zero(passes);
            passes &= passes - 1;

            batch.nodes.push_back(passIdxtoOrderMap[passIdx]);
            graph.graphicNodesCount++;
        }

        graph.graphicBatches.push_back(batch);
    }

    for (uint32_t batchIdx = 0u; batchIdx < m_submissionOrder.computeBatches.size(); ++batchIdx)
    {
        DebugGraph::Batch batch  = {};
        uint64_t          passes = m_submissionOrder.computeBatches[batchIdx].passesMask;

        batch.signalValue        = m_submissionOrder.computeBatches[batchIdx].signalValue;
        batch.waitValue          = m_submissionOrder.computeBatches[batchIdx].waitValue;

        if (passes == 0) continue;

        while (passes)
        {
            uint32_t passIdx = std::countr_zero(passes);
            passes &= passes - 1;

            batch.nodes.push_back(passIdxtoOrderMap[passIdx]);
            graph.computeNodesCount++;
        }

        graph.computeBatches.push_back(batch);
    }

    auto& executor = Engine::get().getTfExecutor();
    executor.silent_async(
        [path, graph]()
        {
            // graph.exportDot(path.c_str());
            graph.exportSubmissionDot(path.c_str());
        });
}

uint32_t RenderGraph::getQueueFamilyIndex(RGQueueFamilyType queueFamily) const
{
    switch (queueFamily)
    {
        case RGQueueFamilyType::Graphics: return m_vkContext.graphicsQueueFamilyIndex;
        case RGQueueFamilyType::Compute:  return m_vkContext.computeQueueFamilyIndex;
        case RGQueueFamilyType::Transfer: return m_vkContext.transferQueueFamilyIndex;
    }
}

void DebugGraph::exportDot(const char* path) const
{
    std::ofstream dotFile(path);

    dotFile << "digraph RenderGraph {\n";
    dotFile << "rankdir=TD;\n";
    dotFile << "node [shape=box, style=filled];\n\n";

    dotFile << "subgraph cluster_graphics {\n";
    dotFile << "label=\"Graphic Queue\";\n";
    dotFile << "style = dotted; \n";

    for (uint32_t nodeIdx = 0u; nodeIdx < nodes.size(); ++nodeIdx)
    {
        const auto&       node  = nodes[nodeIdx];
        const char*       color = node.isCompute ? "lightblue" : "palegreen";

        std::stringstream label;
        label << node.name << "\\n";

        if (!node.reads.empty())
        {
            label << "R: ";
            for (auto& r : node.reads) label << r << " ";
            label << "\\n";
        }

        if (!node.writes.empty())
        {
            label << "W: ";
            for (auto& w : node.writes) label << w << " ";
        }

        dotFile << "node_" << nodeIdx
                << " [label=\"" << label.str() << "\""
                << ", fillcolor=\"" << color << "\"]; \n";
    }

    dotFile << "}\n";
    dotFile << "\n";

    for (uint32_t edgeIdx = 0u; edgeIdx < edges.size(); ++edgeIdx)
    {
        const auto& edge = edges[edgeIdx];
        dotFile << "node_" << edge.src << " -> node_" << edge.dst << ";\n";
    }

    dotFile << "}\n";
}

void DebugGraph::exportSubmissionDot(const char* path) const
{
    std::ofstream dotFile(path);

    dotFile << "digraph RenderGraph {\n";
    dotFile << "rankdir=TD;\n";
    dotFile << "newrank=true;\n";
    dotFile << "node [shape=box, style=filled];\n\n";

    dotFile << "subgraph cluster_graphics {\n";
    dotFile << "label=\"Graphic Queue\";\n";
    dotFile << "style=dotted; \n";

    std::string firstGraphicNodeId;
    std::string lastGraphicNodeId;
    std::string firstComputeNodeId;
    std::string lastComputeNodeId;

    for (uint32_t batchIdx = 0u; batchIdx < graphicBatches.size(); ++batchIdx)
    {
        const auto& batch = graphicBatches[batchIdx];

        dotFile << "subgraph cluster_graphics_batch_" << batchIdx << " {\n";
        dotFile << "label=\"Graphic Batch #" << batchIdx << "\n wait=" << batch.waitValue << " signal=" << batch.signalValue << "\";\n";
        dotFile << "style=dashed; \n";

        std::stringstream label;
        for (uint32_t i = 0u; i < batch.nodes.size(); ++i)
        {
            uint32_t          nodeIdx = batch.nodes[i];
            const auto&       node    = nodes[nodeIdx];
            const char*       color   = node.isCompute ? "lightblue" : "palegreen";

            std::stringstream label;
            label << node.name << "\\n";

            if (!node.reads.empty())
            {
                label << "R: ";
                for (auto& r : node.reads) label << r << " ";
                label << "\\n";
            }

            if (!node.writes.empty())
            {
                label << "W: ";
                for (auto& w : node.writes) label << w << " ";
            }

            dotFile << "node_" << nodeIdx
                    << " [label=\"" << label.str() << "\""
                    << ", fillcolor=\"" << color << "\"]; \n";

            std::string nodeId = "node_" + std::to_string(nodeIdx);
            if (firstGraphicNodeId.empty())
            {
                firstGraphicNodeId = nodeId;
            }
            lastGraphicNodeId = nodeId;
        }

        dotFile << "}\n";
    }

    if (graphicNodesCount < computeNodesCount)
    {
        // pad with invisible nodes and edges for proper cluster alignments in graphviz rendering
        uint32_t padNodesCount = computeNodesCount - graphicNodesCount;

        dotFile << "subgraph cluster_graphic_batch_invis {\n";
        dotFile << "label=\"\";\n";
        dotFile << "margin=18;\n";
        dotFile << "style=invis; \n";

        for (uint32_t i = 0u; i < padNodesCount; ++i)
        {
            std::string padNodeId = "pad_graphic_" + std::to_string(i);
            dotFile << padNodeId << " [label=\"\", style=invis];\n";
            if (!lastGraphicNodeId.empty())
            {
                dotFile << lastGraphicNodeId << " -> " << padNodeId << " [style=invis];\n";
            }
            lastGraphicNodeId = padNodeId;
        }

        dotFile << "}\n";
    }

    dotFile << "}\n";
    dotFile << "\n";

    dotFile << "subgraph cluster_computes {\n";
    dotFile << "label=\"Compute Queue\";\n";
    dotFile << "style=dotted; \n";

    for (uint32_t batchIdx = 0u; batchIdx < computeBatches.size(); ++batchIdx)
    {
        const auto& batch = computeBatches[batchIdx];

        dotFile << "subgraph cluster_compute_batch_" << batchIdx << " {\n";
        dotFile << "label=\"Compute Batch #" << batchIdx << "\n wait=" << batch.waitValue << " signal=" << batch.signalValue << "\";\n";
        dotFile << "style=dashed; \n";

        std::stringstream label;
        for (uint32_t i = 0u; i < batch.nodes.size(); ++i)
        {
            uint32_t          nodeIdx = batch.nodes[i];
            const auto&       node    = nodes[nodeIdx];
            const char*       color   = node.isCompute ? "lightblue" : "palegreen";

            std::stringstream label;
            label << node.name << "\\n";

            if (!node.reads.empty())
            {
                label << "R: ";
                for (auto& r : node.reads) label << r << " ";
                label << "\\n";
            }

            if (!node.writes.empty())
            {
                label << "W: ";
                for (auto& w : node.writes) label << w << " ";
            }

            dotFile << "node_" << nodeIdx
                    << " [label=\"" << label.str() << "\""
                    << ", fillcolor=\"" << color << "\"]; \n";

            std::string nodeId = "node_" + std::to_string(nodeIdx);
            if (firstComputeNodeId.empty())
            {
                firstComputeNodeId = nodeId;
            }
            lastComputeNodeId = nodeId;
        }

        dotFile << "}\n";
    }

    if (computeNodesCount < graphicNodesCount)
    {
        // pad with invisible nodes and edges for proper cluster alignments in graphviz rendering
        uint32_t padNodesCount = graphicNodesCount - computeNodesCount;

        dotFile << "subgraph cluster_compute_batch_invis {\n";
        dotFile << "label=\"\";\n";
        dotFile << "margin=18;\n";
        dotFile << "style=invis; \n";

        for (uint32_t i = 0u; i < padNodesCount; ++i)
        {
            std::string padNodeId = "pad_compute_" + std::to_string(i);
            dotFile << padNodeId << " [label=\"\", style=invis];\n";
            if (!lastGraphicNodeId.empty())
            {
                dotFile << lastComputeNodeId << " -> " << padNodeId << " [style=invis];\n";
            }
            lastComputeNodeId = padNodeId;
        }

        dotFile << "}\n";
    }

    dotFile << "}\n";
    dotFile << "\n";

    dotFile << "{rank=same; " << firstGraphicNodeId << "; " << firstComputeNodeId << ";}\n";

    for (uint32_t edgeIdx = 0u; edgeIdx < edges.size(); ++edgeIdx)
    {
        const auto& edge = edges[edgeIdx];
        dotFile << "node_" << edge.src << " -> node_" << edge.dst << ";\n";
    }

    dotFile << "}\n";

    dotFile.close();

    DUSK_DEBUG("Render graph save at {}", path);
}

} // namespace dusk