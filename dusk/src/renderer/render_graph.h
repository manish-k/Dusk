#pragma once

#include "dusk.h"

#include "renderer/gfx_enums.h"

#include "backend/vulkan/vk.h"

#include <string>
#include <functional>

namespace dusk
{
struct FrameData;
struct GfxTexture;
struct GfxBuffer;

constexpr uint32_t MAX_RENDER_GRAPH_PASSES = 64;

using RecordCmdBuffFunction                = std::function<void(const FrameData&)>;

struct LoadStoreState
{
    GfxLoadOperation  loadOp  = GfxLoadOperation::DontCare;
    GfxStoreOperation storeOp = GfxStoreOperation::Store;
};

struct RGImageLifeTimeState
{
    DynamicArray<uint32_t> versions = {}; // for tracking different versions of the same resource if written by multiple passes
};

struct RGImageExecState
{
    int32_t               firstWriter = -1;
    VkImageLayout         layout      = VK_IMAGE_LAYOUT_UNDEFINED;
    VkPipelineStageFlags2 stage      = VK_PIPELINE_STAGE_2_NONE;
    VkAccessFlags2        access      = VK_ACCESS_2_NONE;
};

struct RGImageResource
{
    std::string name    = "";
    GfxTexture* texture = nullptr;
    uint64_t    writers = {}; // Note: bitset assumption is max 64 passes
    uint64_t    readers = {}; // Note: bitset assumption is max 64 passes
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

    /**
     * @brief Adds a pass with the given name and associated command-recording function.
     * @param passName The name for the pass.
     * @param recordFn A callable invoked to record commands for the pass.
     * @return Handle for the newly added pass.
     */
    uint32_t addPass(
        const std::string&           passName,
        const RecordCmdBuffFunction& recordFn);

    /**
     * @brief Executes the render graph for the given frame data.
     * @param frameData
     */
    void execute(const FrameData& frameData);

    /**
     * @brief Registers an image resource as a read dependency for the specified pass.
     * @param passId Handle of the pass to which the resource will be added.
     * @param resource Reference to the RGImageResource to add as a read dependency.
     * @param version Optional resource version to reference (defaults to 0).
     */
    void addReadResource(
        uint32_t         passId,
        RGImageResource& resource,
        uint32_t         version = 0u);

    /**
     * @brief Adds a buffer resource to the specified pass as a read resource.
     * @param passId Handle of the pass to which the resource will be added.
     * @param resource Reference to the RGBufferResource to add as read dependency.
     */
    void addReadResource(uint32_t passId, RGBufferResource& resource);

    /**
     * @brief Registers an image write resource with the specified pass and returns the new version of that resource.
     * @param passId Handle of the pass to which the write resource will be added.
     * @param resource Reference to the RGImageResource to be added as a write target for the pass.
     * @return New version for the newly added write resource.
     */
    uint32_t addWriteResource(uint32_t passId, RGImageResource& resource);

    /**
     * @brief Adds a writable buffer resource to a pass.
     * @param passId Handle of the pass to which the resource will be added.
     * @param resource Reference to the buffer resource to add as a writable resource.
     * @return New version for the newly added write resource.
     */
    uint32_t addWriteResource(uint32_t passId, RGBufferResource& resource);

    /**
     * @brief Associates an image resource as the depth target for a specified pass.
     * @param passId Handle of the pass to which the depth resource will be added.
     * @param resource Reference to the image resource to register as the depth resource.
     */
    void addDepthResource(uint32_t passId, RGImageResource& resource);

    /**
     * @brief Marks a compute pass identified by the given pass ID.
     * @param passId Handle of the compute pass to mark.
     */
    void markAsCompute(uint32_t passId);

    /**
     * @brief Configures multiview settings for a specified pass.
     * @param passId Handle of the pass to configure.
     * @param mask View mask indicating which views to render.
     * @param numLayers Number of layers to render.
     */
    void setMulitView(uint32_t passId, uint32_t mask, uint32_t numLayers);

private:
    /**
     * @brief Builds the dependency graph.
     */
    void buildDependencyGraph();

    /**
     * @brief Builds the execution order of the passes.
     */
    void buildExecutionOrder();

    /**
     * @brief Generates barriers and load/store states for all resources
     * as per execution order.
     */
    void buildResourceStates();

    /**
     * @brief Begins or initializes a rendering pass using the provided frame data.
     * @param frameData
     * @param pass Reference to the pass node to begin the pass.
     */
    void beginPass(const FrameData& frameData, RGNode& pass);

    /**
     * @brief Ends the current ongoing rendering pass.
     * @param frameData
     */
    void endPass(const FrameData& frameData);

private:
    DynamicArray<RGNode>   m_passes;
    HashSet<std::string>   m_addedPasses;
    DynamicArray<uint32_t> m_passExecutionOrder;

    DynamicArray<uint64_t> m_inEdgesBitsets;
    DynamicArray<uint64_t> m_outEdgesBitsets;

    // states for tracking lifetime of images
    HashMap<uint32_t, RGImageLifeTimeState> m_imageLifeTimeStates;

    // states for images during graph execution time
    HashMap<uint32_t, RGImageExecState> m_imageExecStates;
};
} // namespace dusk