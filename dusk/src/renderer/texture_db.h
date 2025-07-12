#pragma once

#include "dusk.h"

#include "texture.h"
#include "image.h"

#include <taskflow/taskflow.hpp>
#include <thread>

#define COLOR_BINDING_INDEX 0

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

    /**
     * @brief Initialize the texture database
     */
    bool init();

    /**
     * @brief Start cleanup of the texture database
     */
    void cleanup();

    /**
     * @brief Get Texture for the given Id
     * @params texture id
     */
    GfxTexture& getTexture2D(uint32_t texId) { return m_textures[texId]; };

    /**
     * @brief Get default texture
     */
    GfxTexture& getDefaultTexture2D() { return m_textures[0]; };

    /**
     * @brief Creates a new texture asynchronously from one or mulitple images
     * and returns its identifier.
     * @params Paths of all images
     * @params type of the texture
     * @return The unique identifier of the loaded texture.
     */
    uint32_t createTextureAsync(const DynamicArray<std::string>& paths, TextureType type);

    /**
     * @brief Get descriptor set for the textures
     */
    VkGfxDescriptorSet& getTexturesDescriptorSet() const { return *m_textureDescriptorSet; };

    /**
     * @brief Get descriptor set layout for texture set
     */
    VkGfxDescriptorSetLayout& getTexturesDescriptorSetLayout() const { return *m_textureDescriptorSetLayout; };

    /**
     * @brief Per frame update call to upload pending textures
     */
    void onUpdate();

    /**
     * @brief clear a render texture for color attachment
     * @param name of the render target
     * @param width of the render target
     * @param height of the render target
     * @param format of the render target
     * @param clearValue of the render target
     * @return id of the texture
     */
    uint32_t createColorTexture(
        const std::string& name,
        uint32_t           width,
        uint32_t           height,
        VkFormat           format,
        VkClearValue       clearValue);

    /**
     * @brief clear a render texture for depth attachment
     * @param name of the render target
     * @param width of the render target
     * @param height of the render target
     * @param format of the render target
     * @param clearValue of the render target
     * @return id of the texture
     */
    uint32_t createDepthTexture(
        const std::string& name,
        uint32_t           width,
        uint32_t           height,
        VkFormat           format,
        VkClearValue       clearValue);

public:
    /**
     * @brief Static method to get texture db cache instance
     */
    static TextureDB* cache() { return s_db; };

private:
    /**
     * @brief Initialize a default texture of 1x1 image
     */
    Error initDefaultTexture();

    /**
     * @brief Initialize a common sampler for all textures
     */
    Error initDefaultSampler();

    /**
     * @brief Setup the texture descriptor
     */
    bool setupDescriptors();

    /**
     * @brief Free up all the allocated resources
     */
    void freeAllResources();

private:
    uint32_t                         m_idCounter = 0;
    std::mutex                       m_mutex;

    DynamicArray<GfxTexture>         m_textures                 = {};
    HashMap<size_t, uint32_t>        m_loadedTextures           = {};
    HashMap<size_t, uint32_t>        m_currentlyLoadingTextures = {};
    HashMap<uint32_t, ImagesBatch>   m_pendingImageBatches      = {};

    VkGfxDevice&                     m_gfxDevice;
    Unique<VkGfxDescriptorPool>      m_textureDescriptorPool      = nullptr;
    Unique<VkGfxDescriptorSetLayout> m_textureDescriptorSetLayout = nullptr;
    Unique<VkGfxDescriptorSet>       m_textureDescriptorSet       = nullptr;

    VulkanSampler                    m_defaultSampler;

private:
    static TextureDB* s_db;
};
} // namespace dusk