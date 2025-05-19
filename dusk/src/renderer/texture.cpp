#include "texture.h"

#include "engine.h"
#include "image.h"
#include "backend/vulkan/vk_allocator.h"
#include "backend/vulkan/vk_types.h"
#include "backend/vulkan/vk_descriptors.h"
#include "backend/vulkan/vk_device.h"

namespace dusk
{

Error Texture2D::init(Image& texImage, const char* debugName)
{
    auto& device    = Engine::get().getGfxDevice();
    auto& vkContext = device.getSharedVulkanContext();

    width           = texImage.width;
    height          = texImage.height;
    numChannels     = texImage.channels;

    // staging buffer for transfer
    GfxBuffer stagingBuffer;
    GfxBuffer::createHostWriteBuffer(
        GfxBufferUsageFlags::TransferSource,
        texImage.size,
        1,
        "staging_texture_2d_buffer",
        &stagingBuffer);

    if (!stagingBuffer.isAllocated())
        return Error::InitializationFailed;

    stagingBuffer.writeAndFlush(0, texImage.data, texImage.size);

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
    VulkanResult result = vulkan::allocateGPUImage(
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

#ifdef VK_RENDERER_DEBUG
    if (debugName)
    {
        vkdebug::setObjectName(
            vkContext.device,
            VK_OBJECT_TYPE_IMAGE,
            (uint64_t)vkTexture.image.image,
            debugName);
    }
#endif // VK_RENDERER_DEBUG

    device.transitionImageWithLayout(
        &vkTexture.image,
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        1);

    device.copyBufferToImage(
        &stagingBuffer.vkBuffer,
        &vkTexture.image,
        texImage.width,
        texImage.height);

    device.transitionImageWithLayout(
        &vkTexture.image,
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        1,
        1);

    stagingBuffer.free();

    result = device.createImageView(
        &vkTexture.image,
        VK_IMAGE_VIEW_TYPE_2D,
        VK_FORMAT_R8G8B8A8_SRGB,
        1,
        1,
        &vkTexture.imageView);

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

void Texture2D::free()
{
    auto& device    = Engine::get().getGfxDevice();
    auto& vkContext = device.getSharedVulkanContext();

    device.freeImageSampler(&vkSampler);
    device.freeImageView(&vkTexture.imageView);

    vulkan::freeGPUImage(vkContext.gpuAllocator, &vkTexture.image);
}

Error Texture3D::init(DynamicArray<Image>& texImages)
{
    DASSERT(texImages.size() > 0);

    auto& device        = Engine::get().getGfxDevice();
    auto& vkContext     = device.getSharedVulkanContext();

    width               = texImages[texImages.size() - 1].width;
    height              = texImages[texImages.size() - 1].height;
    numChannels         = texImages[texImages.size() - 1].channels;

    uint32_t numImages  = texImages.size();
    size_t   layerSize  = width * height * numChannels;
    size_t   bufferSize = layerSize * numImages;

    // staging buffer params for transfer
    GfxBuffer stagingBuffer;
    GfxBuffer::createHostWriteBuffer(
        GfxBufferUsageFlags::TransferSource,
        layerSize,
        numImages,
        "staging_texture_3d_buffer",
        &stagingBuffer);

    if (!stagingBuffer.isAllocated())
        return Error::InitializationFailed;

    DynamicArray<VkBufferImageCopy> bufferCopyRegions(numImages);

    for (uint32_t imageIndex = 0u; imageIndex < texImages.size(); ++imageIndex)
    {
        DASSERT(width == texImages[imageIndex].width);
        DASSERT(height == texImages[imageIndex].height);
        DASSERT(numChannels == texImages[imageIndex].channels);

        Image& img = texImages[imageIndex];

        // copy image data
        stagingBuffer.writeAndFlushAtIndex(imageIndex, img.data, layerSize);

        VkBufferImageCopy& copyRegion              = bufferCopyRegions[imageIndex];
        copyRegion.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        copyRegion.imageSubresource.mipLevel       = 0;
        copyRegion.imageSubresource.baseArrayLayer = imageIndex;
        copyRegion.imageSubresource.layerCount     = 1;
        copyRegion.imageExtent.width               = img.width;
        copyRegion.imageExtent.height              = img.height;
        copyRegion.imageExtent.depth               = 1;
        copyRegion.bufferOffset                    = layerSize * imageIndex;
    }

    // create dest image
    VkImageCreateInfo imageInfo { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
    imageInfo.imageType     = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width  = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth  = 1;
    imageInfo.mipLevels     = 1;
    imageInfo.format        = VK_FORMAT_R8G8B8_SRGB;
    if (numChannels == 4)
        imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    imageInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage         = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.samples       = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.arrayLayers   = numImages;
    imageInfo.flags         = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

    // create image
    VulkanResult result = vulkan::allocateGPUImage(
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
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        numImages);

    device.copyBufferToImageRegions(
        &stagingBuffer.vkBuffer,
        &vkTexture.image,
        bufferCopyRegions);

    device.transitionImageWithLayout(
        &vkTexture.image,
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        1,
        numImages);

    stagingBuffer.free();

    result = device.createImageView(
        &vkTexture.image,
        VK_IMAGE_VIEW_TYPE_CUBE,
        VK_FORMAT_R8G8B8A8_SRGB,
        1,
        numImages,
        &vkTexture.imageView);

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

void Texture3D::free()
{
    auto& device    = Engine::get().getGfxDevice();
    auto& vkContext = device.getSharedVulkanContext();

    device.freeImageSampler(&vkSampler);
    device.freeImageView(&vkTexture.imageView);

    vulkan::freeGPUImage(vkContext.gpuAllocator, &vkTexture.image);
}

} // namespace dusk