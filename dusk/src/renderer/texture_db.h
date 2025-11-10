#pragma once

#include "dusk.h"

#include "texture.h"
#include "image.h"

#include <taskflow/taskflow.hpp>
#include <thread>

#define COLOR_BINDING_INDEX   0
#define STORAGE_BINDING_INDEX 0

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
     * @brief Creates a new texture asynchronously for an image
     * and returns its identifier.
     * @params Paths of all images
     * @params type of the texture
     * @return The unique identifier of the loaded texture.
     */
    uint32_t createTextureAsync(
        const std::string& path,
        TextureType        type);

    /**
     * @brief Get descriptor set for color textures
     */
    VkGfxDescriptorSet& getTexturesDescriptorSet() const { return *m_textureDescriptorSet; };

    /**
     * @brief Get descriptor set for storage textures
     */
    VkGfxDescriptorSet& getStorageTexturesDescriptorSet() const { return *m_storageTextureDescriptorSet; };

    /**
     * @brief Get descriptor set layout for texture set
     */
    VkGfxDescriptorSetLayout& getTexturesDescriptorSetLayout() const { return *m_textureDescriptorSetLayout; };

    /**
     * @brief Get descriptor set layout for storage texture set
     */
    VkGfxDescriptorSetLayout& getStorageTexturesDescriptorSetLayout() const { return *m_storageTextureDescriptorSetLayout; };

    /**
     * @brief Get descriptor set layout for cubemap(color attachment)
     * texture descriptor set layout
     */
    VkGfxDescriptorSetLayout& getCubeTexturesDescriptorSetLayout() const { return *m_cubeTextureDescriptorSetLayout; };

    /**
     * @brief Per frame update call to upload pending textures
     */
    void onUpdate();

    /**
     * @brief Create a render texture for color attachment
     * @param name of the render target
     * @param width of the render target
     * @param height of the render target
     * @param format of the render target
     * @return id of the texture
     */
    uint32_t createColorTexture(
        const std::string& name,
        uint32_t           width,
        uint32_t           height,
        VkFormat           format);

    /**
     * @brief Create a cube texture for color attachment
     * @param name of the render target
     * @param width of the render target
     * @param height of the render target
     * @param format of the render target
     * @return id of the texture
     */
    uint32_t createCubeColorTexture(
        const std::string& name,
        uint32_t           width,
        uint32_t           height,
        VkFormat           format);

    /**
     * @brief Create a storage texture for compute pipeline
     * @param name of the texture
     * @param width of the texture
     * @param height of the texture
     * @param format of the texture
     * @return id of the texture
     */
    uint32_t createStorageTexture(
        const std::string& name,
        uint32_t           width,
        uint32_t           height,
        VkFormat           format);

    /**
     * @brief Create a render texture for depth attachment
     * @param name of the render target
     * @param width of the render target
     * @param height of the render target
     * @param format of the render target
     * @return id of the texture
     */
    uint32_t createDepthTexture(
        const std::string& name,
        uint32_t           width,
        uint32_t           height,
        VkFormat           format);

    /**
     * @brief Update the sampler of the texture with a new one
     * @param textureId of the texture for update
     * @param new sampler
     */
    void updateTextureSampler(uint32_t textureId, VkSampler sampler);

    /**
     * @brief Save texture in a ktx file. Different thread will be used to
     * save file
     * @param textureId
     * @param filePath
     */
    void saveTextureAsKTX(uint32_t textureId, const std::string& filePath);

    /**
     * @brief Check whether the texture has been uploaded to GPU
     * @param id of the texture
     * @return true if uploaded else false
     */
    bool isTextureUploaded(uint32_t id);

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
    std::mutex                           m_mutex;

    DynamicArray<GfxTexture>             m_textures                 = {};
    HashMap<size_t, uint32_t>            m_loadedTextures           = {};
    HashMap<size_t, uint32_t>            m_currentlyLoadingTextures = {};
    HashMap<uint32_t, Shared<ImageData>> m_pendingImages            = {};

    VkGfxDevice&                         m_gfxDevice;
    Unique<VkGfxDescriptorPool>          m_textureDescriptorPool             = nullptr;
    Unique<VkGfxDescriptorSetLayout>     m_textureDescriptorSetLayout        = nullptr;
    Unique<VkGfxDescriptorSet>           m_textureDescriptorSet              = nullptr;
    Unique<VkGfxDescriptorSetLayout>     m_storageTextureDescriptorSetLayout = nullptr;
    Unique<VkGfxDescriptorSet>           m_storageTextureDescriptorSet       = nullptr;
    Unique<VkGfxDescriptorSetLayout>     m_cubeTextureDescriptorSetLayout    = nullptr;
    Unique<VkGfxDescriptorSet>           m_cubeTextureDescriptorSet          = nullptr;

    VulkanSampler                        m_defaultSampler;
    DynamicArray<VkSampler>              m_extraSamplers; // TODO:: need uniqueness check

private:
    static TextureDB* s_db;
};
} // namespace dusk