#include "texture_db.h"

#include "image.h"
#include "engine.h"

#include "debug/profiler.h"
#include "loaders/stb_image_loader.h"
#include "utils/utils.h"

#include "backend/vulkan/vk_device.h"
#include "backend/vulkan/vk_descriptors.h"

namespace dusk
{
TextureDB*     TextureDB::s_db         = nullptr;

constexpr char texTransferBufferName[] = "tex_transfer_buffer";
constexpr char texGraphicBufferName[]  = "tex_graphic_buffer";

TextureDB::TextureDB(VkGfxDevice& device) :
    m_gfxDevice(device)
{
    DASSERT(!s_db, "Texture DB instance already exists");
    s_db = this;
};

bool TextureDB::init()
{
    setupDescriptors();

    Error err = initDefaultSampler();
    if (err != Error::Ok) return false;

    err = initDefaultTexture();
    if (err != Error::Ok) return false;

    return true;
}

void TextureDB::cleanup()
{
    freeAllResources();
}

void TextureDB::freeAllResources()
{
    m_gfxDevice.freeImageSampler(&m_defaultSampler);

    for (auto& sampler : m_extraSamplers)
    {
        VulkanSampler vksam = {sampler};
        m_gfxDevice.freeImageSampler(&vksam); //TODO: not very clean way
    }

    m_textureDescriptorPool->resetPool();
    m_textureDescriptorSetLayout = nullptr;
    m_textureDescriptorPool      = nullptr;
    m_textureDescriptorSet       = nullptr;

    auto& defaultTex             = m_textures[0];
    for (auto& tex : m_textures)
    {
        // some textures might be using default texture image and image views
        if (tex.image.vkImage != defaultTex.image.vkImage)
            tex.free();
    }
    defaultTex.free();

    m_textures.clear();
    m_loadedTextures.clear();
}

Error TextureDB::initDefaultTexture()
{
    Image defaultTextureImg {};
    defaultTextureImg.width    = 1;
    defaultTextureImg.height   = 1;
    defaultTextureImg.channels = 4;
    defaultTextureImg.size     = 4;
    defaultTextureImg.data     = new glm::u8vec4(255, 255, 255, 255);

    GfxTexture default2dTexture { 0 };
    Error      err = default2dTexture.init(
        defaultTextureImg,
        VK_FORMAT_R8G8B8A8_SRGB,
        TransferDstTexture | SampledTexture,
        "default_tex_2d");

    m_textures.push_back(default2dTexture);

    // update corrosponding descriptor with new image
    VkDescriptorImageInfo texDescInfos {};
    texDescInfos.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    texDescInfos.imageView   = default2dTexture.imageView;
    texDescInfos.sampler     = m_defaultSampler.sampler;

    m_textureDescriptorSet->configureImage(
        0,
        default2dTexture.id,
        1,
        &texDescInfos);

    m_textureDescriptorSet->applyConfiguration();
    return err;
}

Error TextureDB::initDefaultSampler()
{
    VulkanResult result = m_gfxDevice.createImageSampler(&m_defaultSampler);

    if (result.hasError())
    {
        return Error::InitializationFailed;
    }

    return Error::Ok;
}

bool TextureDB::setupDescriptors()
{
    auto& ctx               = m_gfxDevice.getSharedVulkanContext();

    m_textureDescriptorPool = VkGfxDescriptorPool::Builder(ctx)
                                  .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, maxAllowedTextures)
                                  .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, maxAllowedTextures)
                                  .setDebugName("texture_desc_pool")
                                  .build(2, VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT);
    CHECK_AND_RETURN_FALSE(!m_textureDescriptorPool);

    m_textureDescriptorSetLayout = VkGfxDescriptorSetLayout::Builder(ctx)
                                       .addBinding(
                                           COLOR_BINDING_INDEX,
                                           VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                           VK_SHADER_STAGE_FRAGMENT_BIT,
                                           maxAllowedTextures,
                                           true)
                                       .setDebugName("texture_desc_set_layout")
                                       .build();
    CHECK_AND_RETURN_FALSE(!m_textureDescriptorSetLayout);

    m_textureDescriptorSet = m_textureDescriptorPool->allocateDescriptorSet(*m_textureDescriptorSetLayout, "texture_desc_set");
    CHECK_AND_RETURN_FALSE(!m_textureDescriptorSet);

    m_storageTextureDescriptorSetLayout = VkGfxDescriptorSetLayout::Builder(ctx)
                                              .addBinding(
                                                  COLOR_BINDING_INDEX,
                                                  VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                                                  VK_SHADER_STAGE_COMPUTE_BIT,
                                                  maxAllowedTextures,
                                                  true)
                                              .setDebugName("texture_desc_set_layout")
                                              .build();
    CHECK_AND_RETURN_FALSE(!m_storageTextureDescriptorSetLayout);

    m_storageTextureDescriptorSet = m_textureDescriptorPool->allocateDescriptorSet(*m_storageTextureDescriptorSetLayout, "storage_texture_desc_set");
    CHECK_AND_RETURN_FALSE(!m_storageTextureDescriptorSet);
}

uint32_t TextureDB::createTextureAsync(const DynamicArray<std::string>& paths, TextureType type)
{
    DASSERT(paths.size() != 0);

    std::string combinedPaths = combineStrings(paths);
    auto        uploadHash    = hash(combinedPaths.c_str());

    if (m_currentlyLoadingTextures.has(uploadHash)) return m_currentlyLoadingTextures[uploadHash];

    if (m_loadedTextures.has(uploadHash)) return m_loadedTextures[uploadHash];

    uint32_t newId = 0;

    {
        std::lock_guard<std::mutex> updateLock(m_mutex);
        newId = m_textures.size();

        GfxTexture newTex { newId };
        newTex.uploadHash = uploadHash;
        newTex.name       = combinedPaths;
        newTex.type       = type;

        // default texture image till actual tex is uploaded
        newTex.image     = m_textures[0].image;
        newTex.imageView = m_textures[0].imageView;

        m_textures.push_back(newTex);
        m_currentlyLoadingTextures.emplace(uploadHash, newId);
    }

    DASSERT(newId != 0);

    GfxTexture& newTexture = m_textures[newId];

    auto&       executor   = Engine::get().getTfExecutor();
    executor.silent_async([&, newId, paths]()
                          {
        DUSK_PROFILE_SECTION("texture_file_read");
        ImagesBatch batch;
        for (auto& path : paths)
        {
            DASSERT(!path.empty());
            Shared<Image> img = ImageLoader::readImage(path);
            if (img)
                batch.push_back(img);
        }

        DASSERT(batch.size() == paths.size());

        {
            std::lock_guard<std::mutex> updateLock(m_mutex);
            DUSK_DEBUG("Queuing texture {} for gpu upload", newId);
            m_pendingImageBatches.emplace(newId, batch);
        } });

    // update corrosponding descriptor to use default image
    VkDescriptorImageInfo texDescInfos {};
    texDescInfos.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    texDescInfos.imageView   = newTexture.imageView;
    texDescInfos.sampler     = m_defaultSampler.sampler;

    m_textureDescriptorSet->configureImage(
        COLOR_BINDING_INDEX,
        newId,
        1,
        &texDescInfos);

    m_textureDescriptorSet->applyConfiguration();

    return newId;
}

void TextureDB::onUpdate()
{
    DUSK_PROFILE_FUNCTION;

    if (m_pendingImageBatches.size() > 0)
    {
        DUSK_DEBUG("pending texture to upload {}", m_pendingImageBatches.size());

        std::lock_guard<std::mutex> updateLock(m_mutex);

        auto&                       vkContext = m_gfxDevice.getSharedVulkanContext();
        VkCommandBuffer             gfxBuffer;
        VkCommandBuffer             transferBuffer;

        VkCommandBufferAllocateInfo allocInfo {};
        allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool        = vkContext.transferCommandPool;
        allocInfo.commandBufferCount = 1;

        vkAllocateCommandBuffers(vkContext.device, &allocInfo, &transferBuffer);

#ifdef VK_RENDERER_DEBUG
        vkdebug::setObjectName(
            vkContext.device,
            VK_OBJECT_TYPE_COMMAND_BUFFER,
            (uint64_t)transferBuffer,
            texTransferBufferName);
#endif // VK_RENDERER_DEBUG

        allocInfo                    = {};
        allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool        = vkContext.commandPool;
        allocInfo.commandBufferCount = 1;

        vkAllocateCommandBuffers(vkContext.device, &allocInfo, &gfxBuffer);

#ifdef VK_RENDERER_DEBUG
        vkdebug::setObjectName(
            vkContext.device,
            VK_OBJECT_TYPE_COMMAND_BUFFER,
            (uint64_t)gfxBuffer,
            texGraphicBufferName);
#endif // VK_RENDERER_DEBUG

        for (uint32_t key : m_pendingImageBatches.keys())
        {
            GfxTexture&  tex   = m_textures[key];
            ImagesBatch& batch = m_pendingImageBatches[key];

            // deducing format for images in the batch under the
            // assumption that all images are of same format
            VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;
            if (batch[0]->isHDR)
            {
                // currently using 32  bit per channel for hdr files
                // In case of 16 bit half format we need conversion
                // during loading hdr files
                format = VK_FORMAT_R32G32B32A32_SFLOAT;
            }

            Error err = tex.initAndRecordUpload(
                batch,
                tex.type,
                format,
                TransferDstTexture | SampledTexture,
                gfxBuffer,
                transferBuffer,
                tex.name.c_str());

            if (err != Error::Ok)
            {
                DUSK_ERROR("Unable to record texture upload cmds for {}", tex.name);
                break;
            }

            m_currentlyLoadingTextures.erase(tex.uploadHash);
            m_loadedTextures.emplace(tex.uploadHash, tex.id);

            DUSK_DEBUG("Texture {} loaded to gpu", tex.name);

            // update corrosponding descriptor with new image
            VkDescriptorImageInfo texDescInfos {};
            texDescInfos.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            texDescInfos.imageView   = tex.imageView;
            texDescInfos.sampler     = m_defaultSampler.sampler;

            m_textureDescriptorSet->configureImage(
                COLOR_BINDING_INDEX,
                tex.id,
                1,
                &texDescInfos);

            m_textureDescriptorSet->applyConfiguration();
        }

        vkFreeCommandBuffers(vkContext.device, vkContext.transferCommandPool, 1, &transferBuffer);
        vkFreeCommandBuffers(vkContext.device, vkContext.commandPool, 1, &gfxBuffer);

        m_pendingImageBatches.clear();
    }
}

uint32_t TextureDB::createColorTexture(
    const std::string& name,
    uint32_t           width,
    uint32_t           height,
    VkFormat           format)
{
    std::lock_guard<std::mutex> updateLock(m_mutex);

    // initialize texture for render target
    uint32_t   newId = m_textures.size();

    GfxTexture newTex { newId };
    newTex.init(
        width,
        height,
        format,
        SampledTexture | ColorTexture | TransferDstTexture,
        name.c_str());
    m_textures.push_back(newTex);

    // update corrosponding descriptor with new image
    VkDescriptorImageInfo texDescInfos {};
    texDescInfos.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    texDescInfos.imageView   = newTex.imageView;
    texDescInfos.sampler     = m_defaultSampler.sampler;

    m_textureDescriptorSet->configureImage(
        COLOR_BINDING_INDEX,
        newTex.id,
        1,
        &texDescInfos);

    m_textureDescriptorSet->applyConfiguration();

    return newId;
}

uint32_t TextureDB::createStorageTexture(
    const std::string& name,
    uint32_t           width,
    uint32_t           height,
    VkFormat           format)
{
    std::lock_guard<std::mutex> updateLock(m_mutex);

    // initialize texture for render target
    uint32_t   newId = m_textures.size();

    GfxTexture newTex { newId };
    newTex.init(
        width,
        height,
        format,
        SampledTexture | ColorTexture | TransferDstTexture | StorageTexture,
        name.c_str());
    m_textures.push_back(newTex);

    // update corrosponding combined image sampler descriptor with new 
    // image so that we can sample images used as storage.
    VkDescriptorImageInfo texDescInfos {};
    texDescInfos.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    texDescInfos.imageView   = newTex.imageView;
    texDescInfos.sampler     = m_defaultSampler.sampler;

    m_textureDescriptorSet->configureImage(
        COLOR_BINDING_INDEX,
        newTex.id,
        1,
        &texDescInfos);

    m_textureDescriptorSet->applyConfiguration();

    // update corrosponding storage descriptor with new image
    VkDescriptorImageInfo storageTexDescInfos {};
    storageTexDescInfos.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    storageTexDescInfos.imageView   = newTex.imageView;

    m_storageTextureDescriptorSet->configureImage(
        STORAGE_BINDING_INDEX,
        newTex.id,
        1,
        &storageTexDescInfos);

    m_storageTextureDescriptorSet->applyConfiguration();

    return newId;
}

uint32_t TextureDB::createDepthTexture(
    const std::string& name,
    uint32_t           width,
    uint32_t           height,
    VkFormat           format)
{
    std::lock_guard<std::mutex> updateLock(m_mutex);

    // initialize texture for render target
    uint32_t   newId = m_textures.size();

    GfxTexture newTex { newId };
    newTex.init(
        width,
        height,
        format,
        DepthStencilTexture | SampledTexture,
        name.c_str());
    m_textures.push_back(newTex);

    // update corrosponding descriptor with new image
    VkDescriptorImageInfo texDescInfos {};
    texDescInfos.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    texDescInfos.imageView   = newTex.imageView;
    texDescInfos.sampler     = m_defaultSampler.sampler;

    m_textureDescriptorSet->configureImage(
        COLOR_BINDING_INDEX,
        newTex.id,
        1,
        &texDescInfos);

    m_textureDescriptorSet->applyConfiguration();

    return newId;
}

void TextureDB::updateTextureSampler(uint32_t textureId, VkSampler sampler)
{
    GfxTexture&           tex = m_textures[textureId];

    VkDescriptorImageInfo texDescInfos {};
    texDescInfos.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    texDescInfos.imageView   = tex.imageView;
    texDescInfos.sampler     = sampler;

    m_textureDescriptorSet->configureImage(
        COLOR_BINDING_INDEX,
        tex.id,
        1,
        &texDescInfos);

    m_textureDescriptorSet->applyConfiguration();

    m_extraSamplers.push_back(sampler);
}

} // namespace dusk