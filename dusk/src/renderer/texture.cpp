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

    width         = texImage.width;
    height        = texImage.height;
    numChannels   = texImage.channels;

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
    imageInfo.extent.width  = width;
    imageInfo.extent.height = height;
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
        &vkTexture.image);

    if (result.hasError())
    {
        DUSK_ERROR("Unable to create image for texture {}", result.toString());
        return Error::InitializationFailed;
    }

    device.transitionImageWithLayout(
        &vkTexture.image,
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    device.copyBufferToImage(
        &stagingBuffer,
        &vkTexture.image,
        texImage.width,
        texImage.height);

    device.transitionImageWithLayout(
        &vkTexture.image,
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    device.freeBuffer(&stagingBuffer);

    result = device.createImageView(&vkTexture.image, VK_FORMAT_R8G8B8A8_SRGB, &vkTexture.imageView);

    if (result.hasError())
    {
        return Error::InitializationFailed;
    }

    result = device.createImageSampler(&vkSampler);

    if (result.hasError())
    {
        return Error::InitializationFailed;
    }

    return Error::Ok;
}

void Texture::free()
{
    auto& device    = Engine::get().getGfxDevice();
    auto& vkContext = device.getSharedVulkanContext();

    device.freeImageSampler(&vkSampler);
    device.freeImageView(&vkTexture.imageView);

    vulkan::freeGPUImage(vkContext.gpuAllocator, &vkTexture.image);
}

} // namespace dusk