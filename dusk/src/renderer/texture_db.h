#pragma once

#include "dusk.h"

#include "texture.h"

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
    TextureDB(VkGfxDevice& device) :
        m_gfxDevice(device) { };
    ~TextureDB() = default;

    Error    init();
    void     cleanup();
    int      getTexture();
    uint32_t loadTexture();

public:
    static TextureDB* cache();

private:
    Error initDefaultTexture();
    Error initDefaultSampler();
    bool  setupDescriptors();
    void  freeAllResources();

private:
    uint32_t                             m_idCounter = 0;
    std::mutex                           m_idMutex;

    tf::Executor                         m_tfExecutor;

    DynamicArray<Texture2D>              m_textures = {};

    VkGfxDevice&                         m_gfxDevice;
    Unique<VkGfxDescriptorPool>          m_textureDescriptorPool      = nullptr;
    Unique<VkGfxDescriptorSetLayout>     m_textureDescriptorSetLayout = nullptr;
    Unique<VkGfxDescriptorSet>           m_textureDescriptorSet       = nullptr;

    Texture2D                            m_default2dTexture           = { 0 }; // id 0 is reserved for default Texture
    VulkanSampler                        m_defaultSampler;

private:
    static TextureDB* s_db;
};
} // namespace dusk