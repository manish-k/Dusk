#pragma once

#include "dusk.h"

#include "renderer/render_api.h"
#include "renderer/gfx_buffer.h"
#include "renderer/frame_data.h"
#include "renderer/gfx_types.h"

#include "backend/vulkan/vk_descriptors.h"
#include "backend/vulkan/vk_pipeline.h"
#include "backend/vulkan/vk_pipeline_layout.h"

#include <taskflow/taskflow.hpp>

namespace dusk
{

// fwd declarations
class Application;
class Window;
class Event;
class EditorUI;
class Scene;
class Environment;
class BasicRenderSystem;
class GridRenderSystem;
class LightsSystem;
class TextureDB;
class VulkanRenderer;
class VkGfxDevice;
class SubMesh;

struct Material;
struct VkGfxDescriptorPool;
struct VkGfxDescriptorSetLayout;
struct VkGfxDescriptorSet;

constexpr uint32_t maxMaterialCount = 1000;
constexpr uint32_t maxModelCount    = 10000;

struct RenderGraphResources
{
    DynamicArray<GfxBuffer>                  frameIndirectDrawCommandsBuffers    = {};
    DynamicArray<GfxBuffer>                  frameIndirectDrawCountBuffers       = {};
    Unique<VkGfxDescriptorPool>              indirectDrawDescriptorPool          = nullptr;
    Unique<VkGfxDescriptorSetLayout>         indirectDrawDescriptorSetLayout     = nullptr;
    DynamicArray<Unique<VkGfxDescriptorSet>> indirectDrawDescriptorSet           = {};

    DynamicArray<uint32_t>                   gbuffRenderTextureIds               = {};
    uint32_t                                 gbuffDepthTextureId                 = {};
    Unique<VkGfxRenderPipeline>              gbuffPipeline                       = nullptr;
    Unique<VkGfxPipelineLayout>              gbuffPipelineLayout                 = nullptr;
    Unique<VkGfxDescriptorPool>              gbuffModelDescriptorPool            = nullptr;
    Unique<VkGfxDescriptorSetLayout>         gbuffModelDescriptorSetLayout       = nullptr;
    DynamicArray<Unique<VkGfxDescriptorSet>> gbuffModelDescriptorSet             = {};
    DynamicArray<GfxBuffer>                  gbuffModelsBuffer                   = {};

    Unique<VkGfxDescriptorPool>              meshInstanceDataDescriptorPool      = nullptr;
    Unique<VkGfxDescriptorSetLayout>         meshInstanceDataDescriptorSetLayout = nullptr;
    DynamicArray<Unique<VkGfxDescriptorSet>> meshInstanceDataDescriptorSet       = {};
    DynamicArray<GfxBuffer>                  meshInstanceDataBuffers             = {};

    Unique<VkGfxRenderPipeline>              presentPipeline                     = nullptr;
    Unique<VkGfxPipelineLayout>              presentPipelineLayout               = nullptr;

    uint32_t                                 lightingRenderTextureId             = {};
    Unique<VkGfxRenderPipeline>              lightingPipeline                    = nullptr;
    Unique<VkGfxPipelineLayout>              lightingPipelineLayout              = nullptr;

    uint32_t                                 brdfLUTextureId                     = {};
    Unique<VkGfxComputePipeline>             brdfLUTPipeline                     = nullptr;
    Unique<VkGfxPipelineLayout>              brdfLUTPipelineLayout               = nullptr;
    bool                                     brdfLUTGenerated                    = false;

    Unique<VkGfxRenderPipeline>              shadow2DMapPipeline                 = nullptr;
    Unique<VkGfxPipelineLayout>              shadow2DMapPipelineLayout           = nullptr;
    uint32_t                                 dirShadowMapsTextureId              = {};

    Unique<VkGfxComputePipeline>             cullLodPipeline                     = nullptr;
    Unique<VkGfxPipelineLayout>              cullLodPipelineLayout               = nullptr;
};

struct BRDFLUTPushConstant
{
    uint32_t lutTextureIdx;
};

class Engine final
{
public:
    struct Config
    {
        RenderAPI::API renderAPI;

        static Config  defaultConfig()
        {
            auto config      = Config {};
            config.renderAPI = RenderAPI::API::VULKAN;
            return config;
        }
    };

public:
    Engine(const Config& conf);
    ~Engine();

    CLASS_UNCOPYABLE(Engine)

    bool                  start(Shared<Application> app);
    void                  run();
    void                  stop();
    void                  shutdown();
    void                  onUpdate(TimeStep dt);
    void                  onEvent(Event& ev);
    void                  loadScene(Scene* scene);
    void                  renderFrame(FrameData& frameData);

    static Engine&        get() { return *s_instance; }
    static RenderAPI::API getRenderAPI() { return s_instance->m_config.renderAPI; }

    VulkanRenderer&       getRenderer() { return *m_renderer; }
    VkGfxDevice&          getGfxDevice() { return *m_gfxDevice; }
    EditorUI&             getEditorUI() { return *m_editorUI; }

    bool                  setupGlobals();
    void                  cleanupGlobals();

    void                  registerMaterials(DynamicArray<Material>& materials);

    void                  updateMaterialsBuffer(DynamicArray<Material>& materials);
    void                  updateMeshDataBuffer(DynamicArray<SubMesh>& subMeshes);

    TimeStep              getFrameDelta() const { return m_deltaTime; };

    void                  prepareRenderGraphResources();
    void                  releaseRenderGraphResources();
    RenderGraphResources& getRenderGraphResources() { return m_rgResources; };

    Environment&          getEnvironment() { return *m_environment; };

    tf::Executor&         getTfExecutor() { return m_tfExecutor; }

    /**
     * @brief Returns a non-const reference to the global vertex buffer.
     * @return A reference to the global vertex buffer.
     */
    GfxBuffer& getVertexBuffer() { return m_vertexBuffer; };

    /**
     * @brief Copies src vertex data buffer to the engine's vertex buffer.
     * @param A reference to the source vertex buffer containing vertex data to be copied.
     * @param Size in bytes of the data to be copied from the source buffer.
     * @return Vertex buffer offset where the data has been copied. This is not byte offset.
     */
    size_t copyToVertexBuffer(const GfxBuffer& srcVertexBuffer, size_t size);

    /**
     * @brief Returns a non-const reference to the global index buffer.
     * @return A reference to the global index buffer.
     */
    GfxBuffer& getIndexBuffer() { return m_indexBuffer; };

    /**
     * @brief Copies src index data buffer to the engine's index buffer.
     * @param A reference to the source index buffer containing index data to be copied.
     * @param Size in bytes of the data to be copied from the source buffer.
     * @return Base index within the index buffer where the data has been copied. This is not byte offset.
     */
    size_t copyToIndexBuffer(const GfxBuffer& srcIndexbuffer, size_t size);

    void   executeBRDFLUTcomputePipeline();

private:
    Config                           m_config;

    Unique<VkGfxDevice>              m_gfxDevice         = nullptr;
    Unique<VulkanRenderer>           m_renderer          = nullptr;

    Shared<Window>                   m_window            = nullptr;
    Shared<Application>              m_app               = nullptr;

    Unique<Environment>              m_environment       = nullptr;
    Unique<BasicRenderSystem>        m_basicRenderSystem = nullptr;
    Unique<LightsSystem>             m_lightsSystem      = nullptr;

    Unique<TextureDB>                m_textureDB         = nullptr;

    Unique<EditorUI>                 m_editorUI          = nullptr;

    bool                             m_running           = false;
    bool                             m_paused            = false;

    Scene*                           m_currentScene      = nullptr;

    TimePoint                        m_lastFrameTime {};
    TimeStep                         m_deltaTime {};

    GfxBuffer                        m_vertexBuffer;
    GfxBuffer                        m_indexBuffer;

    size_t                           m_availableVertexOffset     = 0u;
    size_t                           m_availableIndexOffset      = 0u;

    Unique<VkGfxDescriptorPool>      m_globalDescriptorPool      = nullptr;
    Unique<VkGfxDescriptorSetLayout> m_globalDescriptorSetLayout = nullptr;

    GfxBuffer                        m_globalUbos;
    Unique<VkGfxDescriptorSet>       m_globalDescriptorSet;

    Unique<VkGfxDescriptorPool>      m_materialDescriptorPool      = nullptr;
    Unique<VkGfxDescriptorSetLayout> m_materialDescriptorSetLayout = nullptr;

    GfxBuffer                        m_materialsBuffer;
    Unique<VkGfxDescriptorSet>       m_materialsDescriptorSet;

    Unique<VkGfxDescriptorPool>      m_meshDataDescriptorPool      = nullptr;
    Unique<VkGfxDescriptorSetLayout> m_meshDataDescriptorSetLayout = nullptr;

    GfxBuffer                        m_meshDataBuffer;
    Unique<VkGfxDescriptorSet>       m_meshDataDescriptorSet;

    RenderGraphResources             m_rgResources;

    tf::Executor                     m_tfExecutor;

private:
    static Engine* s_instance;
};
} // namespace dusk