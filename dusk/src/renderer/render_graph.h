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

struct RGImageState
{
    GfxLoadOperation  loadOp      = GfxLoadOperation::DontCare;
    GfxStoreOperation storeOp     = GfxStoreOperation::Store;
    int32_t           firstWriter = -1; // first pass index that writes to this resource
};

struct RGBufferResource
{
    std::string name    = "";
    GfxBuffer*  buffer  = nullptr;
    uint64_t    writers = {}; // Note: bitset assumption is max 64 passes
    uint64_t    readers = {}; // Note: bitset assumption is max 64 passes
};

struct RGNode
{
    std::string           name      = "";
    RecordCmdBuffFunction recordFn  = nullptr;
    bool                  isCompute = false;

    // TODO:: need to reserve space, max 64 passes for now
    DynamicArray<RGImageResource*>  readTextureResources;
    DynamicArray<RGImageResource*>  writeTextureResources;
    DynamicArray<RGBufferResource*> readBufferResources;
    DynamicArray<RGBufferResource*> writeBufferResources;

    std::optional<RGImageResource*> depthResource;

    uint32_t                        viewMask   = 0u; // only for multiview
    uint32_t                        layerCount = 1u; // only for multiview

    HashMap<uint32_t, RGImageState> imageResourceStates;

    // helper functions for adding Resources
    void addReadResource(RGImageResource& resource)
    {
        readTextureResources.push_back(&resource);
    }

    void addReadResource(RGBufferResource& resource)
    {
        readBufferResources.push_back(&resource);
    }

    void addWriteResource(RGImageResource& resource)
    {
        writeTextureResources.push_back(&resource);
    }

    void addWriteResource(RGBufferResource& resource)
    {
        writeBufferResources.push_back(&resource);
    }

    void addDepthResource(RGImageResource& resource)
    {
        depthResource = &resource;
        writeTextureResources.push_back(&resource);
        readTextureResources.push_back(&resource);
    }

    void markAsCompute()
    {
        isCompute = true;
    }

    void setMulitView(uint32_t viewMask_, uint32_t layerCount_)
    {
        viewMask   = viewMask_;
        layerCount = layerCount_;
    }
};

class RenderGraph
{
public:
    RenderGraph();
    RGNode& addPass(
        const std::string&           passName,
        const RecordCmdBuffFunction& recordFn);

    void execute(const FrameData& frameData);

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
};
} // namespace dusk