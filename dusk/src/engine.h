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

struct RenderGraphResources
{
    DynamicArray<uint32_t>                   gbuffRenderTextureIds         = {};
    uint32_t                                 gbuffDepthTextureId           = {};
    Unique<VkGfxRenderPipeline>              gbuffPipeline                 = nullptr;
    Unique<VkGfxPipelineLayout>              gbuffPipelineLayout           = nullptr;
    Unique<VkGfxDescriptorPool>              gbuffModelDescriptorPool      = nullptr;
    Unique<VkGfxDescriptorSetLayout>         gbuffModelDescriptorSetLayout = nullptr;
    DynamicArray<Unique<VkGfxDescriptorSet>> gbuffModelDescriptorSet       = {};
    DynamicArray<GfxBuffer>                  gbuffModelsBuffer             = {};

    Unique<VkGfxRenderPipeline>              presentPipeline               = nullptr;
    Unique<VkGfxPipelineLayout>              presentPipelineLayout         = nullptr;

    uint32_t                                 lightingRenderTextureId       = {};
    Unique<VkGfxRenderPipeline>              lightingPipeline              = nullptr;
    Unique<VkGfxPipelineLayout>              lightingPipelineLayout        = nullptr;

    uint32_t                                 brdfLUTextureId               = {};
    Unique<VkGfxComputePipeline>              brdfLUTPipeline               = nullptr;
    Unique<VkGfxPipelineLayout>              brdfLUTPipelineLayout         = nullptr;
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

    TimeStep              getFrameDelta() const { return m_deltaTime; };

    void                  prepareRenderGraphResources();
    void                  releaseRenderGraphResources();
    RenderGraphResources& getRenderGraphResources() { return m_rgResources; };

    Environment&          getEnvironment() { return *m_environment; };

    tf::Executor&         getTfExecutor() { return m_tfExecutor; }

    void                  executeBRDFLUTcomputePipeline();

private:
    Config                           m_config;

    Unique<VkGfxDevice>              m_gfxDevice         = nullptr;
    Unique<VulkanRenderer>           m_renderer          = nullptr;

    Shared<Window>                   m_window            = nullptr;
    Shared<Application>              m_app               = nullptr;

    Unique<Environment>              m_environment       = nullptr;
    Unique<BasicRenderSystem>        m_basicRenderSystem = nullptr;
    Unique<GridRenderSystem>         m_gridRenderSystem  = nullptr;
    Unique<LightsSystem>             m_lightsSystem      = nullptr;

    Unique<TextureDB>                m_textureDB         = nullptr;

    Unique<EditorUI>                 m_editorUI          = nullptr;

    bool                             m_running           = false;
    bool                             m_paused            = false;

    Scene*                           m_currentScene      = nullptr;

    TimePoint                        m_lastFrameTime {};
    TimeStep                         m_deltaTime {};

    Unique<VkGfxDescriptorPool>      m_globalDescriptorPool      = nullptr;
    Unique<VkGfxDescriptorSetLayout> m_globalDescriptorSetLayout = nullptr;

    GfxBuffer                        m_globalUbos;
    Unique<VkGfxDescriptorSet>       m_globalDescriptorSet;

    Unique<VkGfxDescriptorPool>      m_materialDescriptorPool      = nullptr;
    Unique<VkGfxDescriptorSetLayout> m_materialDescriptorSetLayout = nullptr;

    GfxBuffer                        m_materialsBuffer;
    Unique<VkGfxDescriptorSet>       m_materialsDescriptorSet;

    RenderGraphResources             m_rgResources;

    tf::Executor                     m_tfExecutor;

private:
    static Engine* s_instance;
};
} // namespace dusk