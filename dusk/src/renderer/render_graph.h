#pragma once

#include "dusk.h"

#include "renderer/gfx_enums.h"

#include <string>
#include <functional>

namespace dusk
{
struct FrameData;
struct GfxTexture;
struct GfxBuffer;

constexpr uint32_t MAX_RENDER_GRAPH_PASSES = 64;

using RecordCmdBuffFunction                = std::function<void(const FrameData&)>;

struct RGImageResource
{
    std::string name    = "";
    GfxTexture* texture = nullptr;
    uint64_t    writers = {}; // Note: bitset assumption is max 64 passes
    uint64_t    readers = {}; // Note: bitset assumption is max 64 passes
};

struct LoadStoreState
{
    GfxLoadOperation  loadOp  = GfxLoadOperation::DontCare;
    GfxStoreOperation storeOp = GfxStoreOperation::Store;
};

struct RGImageState
{
    int32_t                firstWriter = -1; // first pass index that writes to this resource
    DynamicArray<uint32_t> versions    = {}; // for tracking different versions of the same resource if written by multiple passes
};

struct RGBufferResource
{
    std::string name    = "";
    GfxBuffer*  buffer  = nullptr;
    uint64_t    writers = {}; // Note: bitset assumption is max 64 passes
    uint64_t    readers = {}; // Note: bitset assumption is max 64 passes
};

struct RGNodeResource
{
    void*    ptr            = nullptr; // can be RGImageResource* or RGBufferResource*
    uint64_t lastWriterMask = 0u;      // bitset of the last writer
};

struct RGNode
{
    std::string           name      = "";
    uint32_t              index     = 0u; // index of the pass
    RecordCmdBuffFunction recordFn  = nullptr;
    bool                  isCompute = false;

    // TODO:: need to reserve space, max 64 passes for now
    DynamicArray<RGNodeResource>      readTextureResources;
    DynamicArray<RGNodeResource>      writeTextureResources;
    DynamicArray<RGNodeResource>      readBufferResources;
    DynamicArray<RGNodeResource>      writeBufferResources;

    std::optional<RGImageResource*>   depthResource;

    HashMap<uint32_t, LoadStoreState> resourceLoadStoreStates;

    uint32_t                          viewMask   = 0u; // only for multiview
    uint32_t                          layerCount = 1u; // only for multiview
};

class RenderGraph
{
public:
    RenderGraph();
    uint32_t addPass(
        const std::string&           passName,
        const RecordCmdBuffFunction& recordFn);

    void execute(const FrameData& frameData);

    // helper functions for adding Resources
    void addReadResource(
        uint32_t         passId,
        RGImageResource& resource,
        uint32_t         version = 0u);

    void     addReadResource(uint32_t passId, RGBufferResource& resource);

    uint32_t addWriteResource(uint32_t passId, RGImageResource& resource);

    uint32_t addWriteResource(uint32_t passId, RGBufferResource& resource);

    void     addDepthResource(uint32_t passId, RGImageResource& resource);

    void     markAsCompute(uint32_t passId);

    void     setMulitView(uint32_t passId, uint32_t mask, uint32_t numLayers);

private:
    void buildDependencyGraph();
    void buildExecutionOrder();
    void generateLoadStoreOps();

    void beginPass(const FrameData& frameData, RGNode& pass);
    void endPass(const FrameData& frameData);

private:
    DynamicArray<RGNode>   m_passes;
    HashSet<std::string>   m_addedPasses;
    DynamicArray<uint32_t> m_passExecutionOrder;

    DynamicArray<uint64_t> m_inEdgesBitsets;
    DynamicArray<uint64_t> m_outEdgesBitsets;

    // TODO:: versioning for buffers if needed, currently only images are tracked
    HashMap<uint32_t, RGImageState> m_imageResourceStates;
};
} // namespace dusk