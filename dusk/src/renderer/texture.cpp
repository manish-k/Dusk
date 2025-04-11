#include "texture.h"

#include "engine.h"
#include "image.h"
#include "backend/vulkan/vk_allocator.h"
#include "backend/vulkan/vk_types.h"

namespace dusk
{

Error Texture::init(Image& texImage)
{
    auto& device    = Engine::get().getGfxDevice();
    auto& vkContext = device.getSharedVulkanContext();

    m_width         = texImage.width;
    m_height        = texImage.height;
    m_numChannels   = texImage.channels;

    // staging buffer params for creation
    GfxBufferParams stagingBufferParams {};
    stagingBufferParams.sizeInBytes = texImage.size;
    stagingBufferParams.usage       = GfxBufferUsageFlags::TransferSource;
    stagingBufferParams.memoryType  = GfxBufferMemoryTypeFlags::HostSequentialWrite | GfxBufferMemoryTypeFlags::PersistentlyMapped;

    // create staging
    VulkanGfxBuffer stagingBuffer {};

    VulkanResult    result = device.createBuffer(stagingBufferParams, &stagingBuffer);
    if (result.hasError())
    {
        DUSK_ERROR("Unable to create staging buffer for uploading image  data {}", result.toString());
        return Error::OutOfMemory;
    }

    memcpy(stagingBuffer.mappedMemory, texImage.data, texImage.size);

    // create dest image
    VkImageCreateInfo imageInfo { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
    imageInfo.imageType     = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width  = m_width;
    imageInfo.extent.height = m_height;
    imageInfo.extent.depth  = 1;
    imageInfo.mipLevels     = 1;
    imageInfo.arrayLayers   = 1;
    imageInfo.format        = VK_FORMAT_R8G8B8A8_SRGB;
    imageInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage         = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.samples       = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.flags         = 0;

    // create image
    result = vulkan::allocateGPUImage(
        vkContext.gpuAllocator,
        imageInfo,
        VMA_MEMORY_USAGE_AUTO,
        VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
        &m_vkTexture.image);

    if (result.hasError())
    {
        DUSK_ERROR("Unable to create image for texture {}", result.toString());
        return Error::InitializationFailed;
    }

    device.transitionImageWithLayout(
        &m_vkTexture.image,
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    device.copyBufferToImage(
        &stagingBuffer,
        &m_vkTexture.image,
        texImage.width,
        texImage.height);

    device.transitionImageWithLayout(
        &m_vkTexture.image,
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    device.freeBuffer(&stagingBuffer);

    result = device.createImageView(&m_vkTexture.image, VK_FORMAT_R8G8B8A8_SRGB, &m_vkTexture.imageView);

    if (result.hasError())
    {
        return Error::InitializationFailed;
    }

    result = device.createImageSampler(&m_vkSampler);

    if (result.hasError())
    {
        return Error::InitializationFailed;
    }

    return Error::Ok;
}

void Texture::free()
{
    auto& device = Engine::get().getGfxDevice();
    auto& vkContext = device.getSharedVulkanContext();

    device.freeImageSampler(&m_vkSampler);
    device.freeImageView(&m_vkTexture.imageView);

    vulkan::freeGPUImage(vkContext.gpuAllocator, &m_vkTexture.image);
}

} // namespace dusk