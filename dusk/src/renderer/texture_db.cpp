#include "texture_db.h"

#include "image.h"
#include "engine.h"

#include "debug/profiler.h"
#include "loaders/stb_image_loader.h"

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
    DASSERT(!s_db, "Engine instance already exists");
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

    m_textureDescriptorPool->resetPool();
    m_textureDescriptorSetLayout = nullptr;
    m_textureDescriptorPool      = nullptr;
    m_textureDescriptorSet       = nullptr;

    auto& defaultTex             = m_textures[0];
    for (auto& tex : m_textures)
    {
        // some textures might be using default texture image and image views
        if (tex.vkTexture.image.image != defaultTex.vkTexture.image.image)
            tex.free();
    }
    defaultTex.free();

    m_textures.clear();
    m_loadedTextures.clear();
}

Error TextureDB::initDefaultTexture()
{
    glm::u8vec4 whiteColor { 255 };

    Image       defaultTextureImg {};
    defaultTextureImg.width    = 1;
    defaultTextureImg.height   = 1;
    defaultTextureImg.channels = 4;
    defaultTextureImg.size     = 4;
    defaultTextureImg.data     = (unsigned char*)&whiteColor;

    Texture2D default2dTexture { 0 };
    Error     err = default2dTexture.init(defaultTextureImg, "default_tex_2d");

    m_textures.push_back(default2dTexture);

    // update corrosponding descriptor with new image
    VkDescriptorImageInfo texDescInfos {};
    texDescInfos.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    texDescInfos.imageView   = default2dTexture.vkTexture.imageView;
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
                                  .setDebugName("texture_desc_pool")
                                  .build(1, VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT);
    CHECK_AND_RETURN_FALSE(!m_textureDescriptorPool);

    m_textureDescriptorSetLayout = VkGfxDescriptorSetLayout::Builder(ctx)
                                       .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, maxAllowedTextures, true)
                                       .setDebugName("texture_desc_set_layout")
                                       .build();
    CHECK_AND_RETURN_FALSE(!m_textureDescriptorSetLayout);

    m_textureDescriptorSet = m_textureDescriptorPool->allocateDescriptorSet(*m_textureDescriptorSetLayout, "texture_desc_set");
    CHECK_AND_RETURN_FALSE(!m_textureDescriptorSet);
}

uint32_t TextureDB::loadTextureAsync(std::string& path)
{
    DASSERT(!path.empty());

    if (m_currentlyLoadingTextures.has(path)) return m_currentlyLoadingTextures[path];

    if (m_loadedTextures.has(path)) return m_loadedTextures[path];

    uint32_t newId = 0;

    {
        std::lock_guard<std::mutex> updateLock(m_mutex);
        newId = m_textures.size();

        Texture2D newTex { newId };
        newTex.name = path;

        // default texture image till actual tex is uploaded
        newTex.vkTexture.image     = m_textures[0].vkTexture.image;
        newTex.vkTexture.imageView = m_textures[0].vkTexture.imageView;

        m_textures.push_back(newTex);
        m_currentlyLoadingTextures.emplace(path, newId);
    }

    DASSERT(newId != 0);

    Texture2D& newTexture = m_textures[newId];

    auto&      executor   = Engine::get().getTfExecutor();
    executor.silent_async([&, newId]()
                          {
        auto filePath = m_textures[newId].name;

        Shared<Image> img = nullptr;
        {
            DUSK_PROFILE_SECTION("texture_file_read");
            img = ImageLoader::readImage(filePath);
            if (img)
            {
                {
                    std::lock_guard<std::mutex> updateLock(m_mutex);
                    DUSK_DEBUG("Queuing texture {} for gpu upload", newId);
                    m_pendingImages.emplace(newId, img);
                }
            }
        } });

    // update corrosponding descriptor to use default image
    VkDescriptorImageInfo texDescInfos {};
    texDescInfos.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    texDescInfos.imageView   = newTexture.vkTexture.imageView;
    texDescInfos.sampler     = m_defaultSampler.sampler;

    m_textureDescriptorSet->configureImage(
        0,
        newId,
        1,
        &texDescInfos);

    m_textureDescriptorSet->applyConfiguration();

    return newId;
}

void TextureDB::onUpdate()
{
    DUSK_PROFILE_FUNCTION;

    if (m_pendingImages.size() > 0)
    {
        DUSK_DEBUG("pending texture to upload {}", m_pendingImages.size());

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

        for (uint32_t key : m_pendingImages.keys())
        {
            Texture2D& tex = m_textures[key];
            Image&     img = *m_pendingImages[key];
            Error      err = tex.initAndRecordUpload(img, gfxBuffer, transferBuffer, tex.name.c_str());
            if (err != Error::Ok)
            {
                DUSK_ERROR("Unable to record texture upload cmds for {}", tex.name);
                break;
            }

            m_currentlyLoadingTextures.erase(tex.name);
            m_loadedTextures.emplace(tex.name, tex.id);

            DUSK_DEBUG("Texture {} loaded to gpu", tex.name);

            // update corrosponding descriptor with new image
            VkDescriptorImageInfo texDescInfos {};
            texDescInfos.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            texDescInfos.imageView   = tex.vkTexture.imageView;
            texDescInfos.sampler     = m_defaultSampler.sampler;

            m_textureDescriptorSet->configureImage(
                0,
                tex.id,
                1,
                &texDescInfos);

            m_textureDescriptorSet->applyConfiguration();

            ImageLoader::freeImage(img);
        }

        vkFreeCommandBuffers(vkContext.device, vkContext.transferCommandPool, 1, &transferBuffer);
        vkFreeCommandBuffers(vkContext.device, vkContext.commandPool, 1, &gfxBuffer);

        m_pendingImages.clear();
    }
}

} // namespace dusk