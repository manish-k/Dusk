#pragma once

#include "dusk.h"

#include "texture.h"
#include "image.h"

#include <taskflow/taskflow.hpp>
#include <thread>

namespace dusk
{
class VkGfxDevice;

struct VkGfxDescriptorPool;
struct VkGfxDescriptorSetLayout;
struct VkGfxDescriptorSet;

constexpr uint32_t maxAllowedTextures = 1000;

class TextureDB
{
public:
    TextureDB(VkGfxDevice& device);
    ~TextureDB() = default;

    bool     init();
    void     cleanup();
    int      getTexture();
    uint32_t loadTextureAsync(std::string& path);
    VkGfxDescriptorSet& getTextureDescriptorSet() const { return *m_textureDescriptorSet; };
    VkGfxDescriptorSetLayout& getTextureDescriptorSetLayout() const { return *m_textureDescriptorSetLayout; };
    void     onUpdate();

public:
    static TextureDB* cache() { return s_db; };

private:
    Error initDefaultTexture();
    Error initDefaultSampler();
    bool  setupDescriptors();
    void  freeAllResources();

private:
    uint32_t                         m_idCounter = 0;
    std::mutex                       m_mutex;

    tf::Executor                     m_tfExecutor;

    DynamicArray<Texture2D>          m_textures                 = {};
    HashMap<std::string, uint32_t>   m_loadedTextures           = {};
    HashMap<std::string, uint32_t>   m_currentlyLoadingTextures = {};
    HashMap<uint32_t, Shared<Image>> m_pendingImages            = {};

    VkGfxDevice&                     m_gfxDevice;
    Unique<VkGfxDescriptorPool>      m_textureDescriptorPool      = nullptr;
    Unique<VkGfxDescriptorSetLayout> m_textureDescriptorSetLayout = nullptr;
    Unique<VkGfxDescriptorSet>       m_textureDescriptorSet       = nullptr;

    VulkanSampler                    m_defaultSampler;

private:
    static TextureDB* s_db;
};
} // namespace dusk