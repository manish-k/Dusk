#include "texture.h"

#include "engine.h"
#include "image.h"
#include "debug/profiler.h"
#include "backend/vulkan/vk_allocator.h"
#include "backend/vulkan/vk_types.h"
#include "backend/vulkan/vk_descriptors.h"
#include "backend/vulkan/vk_device.h"

namespace dusk
{

Error GfxTexture::init(
    ImageData&      texImage,
    VkFormat    format,
    uint32_t    usage,
    const char* debugName)
{
    DUSK_PROFILE_FUNCTION;

    auto& device      = Engine::get().getGfxDevice();
    auto& vkContext   = device.getSharedVulkanContext();

    this->width       = texImage.width;
    this->height      = texImage.height;
    this->format      = format;

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
    imageInfo.format        = format;
    imageInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage         = vulkan::getTextureUsageFlagBits(usage);
    imageInfo.samples       = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.flags         = 0;

    // create image
    VulkanResult result = vulkan::allocateGPUImage(
        vkContext.gpuAllocator,
        imageInfo,
        VMA_MEMORY_USAGE_AUTO,
        VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
        &image);

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
            (uint64_t)image.vkImage,
            debugName);
    }
#endif // VK_RENDERER_DEBUG

    device.transitionImageWithLayout(
        &image,
        format,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        1);

    device.copyBufferToImage(
        &stagingBuffer.vkBuffer,
        &image,
        texImage.width,
        texImage.height);

    device.transitionImageWithLayout(
        &image,
        format,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        1,
        1);

    stagingBuffer.free();

    result = device.createImageView(
        &image,
        VK_IMAGE_VIEW_TYPE_2D,
        format,
        VK_IMAGE_ASPECT_COLOR_BIT,
        1,
        1,
        &imageView);

    if (result.hasError())
    {
        return Error::InitializationFailed;
    }

    return Error::Ok;
}

Error GfxTexture::init(
    uint32_t    width,
    uint32_t    height,
    VkFormat    format,
    uint32_t    usage,
    const char* name)
{
    DUSK_PROFILE_FUNCTION;

    this->width                 = width;
    this->height                = height;
    this->usage                 = usage;
    this->name                  = name;
    this->format                = format;

    auto&             device    = Engine::get().getGfxDevice();
    auto&             vkContext = VkGfxDevice::getSharedVulkanContext();

    VkImageCreateInfo imageInfo { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
    imageInfo.imageType     = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width  = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth  = 1;
    imageInfo.mipLevels     = 1;
    imageInfo.arrayLayers   = 1;
    imageInfo.format        = format;
    imageInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage         = vulkan::getTextureUsageFlagBits(usage);
    imageInfo.samples       = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.flags         = 0;

    this->currentLayout     = VK_IMAGE_LAYOUT_UNDEFINED;

    // create image
    VulkanResult result = vulkan::allocateGPUImage(
        vkContext.gpuAllocator,
        imageInfo,
        VMA_MEMORY_USAGE_AUTO,
        VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
        &image);

    if (result.hasError())
    {
        DUSK_ERROR("Unable to create image for texture {}", result.toString());
        return Error::InitializationFailed;
    }

#ifdef VK_RENDERER_DEBUG
    vkdebug::setObjectName(
        vkContext.device,
        VK_OBJECT_TYPE_IMAGE,
        (uint64_t)image.vkImage,
        name);

#endif // VK_RENDERER_DEBUG

    auto imageAspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
    if (usage & DepthStencilTexture)
    {
        imageAspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;
    }

    result = device.createImageView(
        &image,
        VK_IMAGE_VIEW_TYPE_2D,
        format,
        imageAspectFlags,
        1,
        1,
        &imageView);

    if (result.hasError())
    {
        DUSK_ERROR("Unable to create image view for texture {}", result.toString());
        return Error::InitializationFailed;
    }

#ifdef VK_RENDERER_DEBUG
    vkdebug::setObjectName(
        vkContext.device,
        VK_OBJECT_TYPE_IMAGE_VIEW,
        (uint64_t)imageView,
        name);
#endif // VK_RENDERER_DEBUG

    return Error::Ok;
}

Error GfxTexture::initAndRecordUpload(
    ImagesBatch&    texImages,
    TextureType     type,
    VkFormat        format,
    uint32_t        usage,
    VkCommandBuffer graphicsBuffer,
    VkCommandBuffer transferBuffer,
    const char*     debugName)
{
    DUSK_PROFILE_FUNCTION;

    DASSERT(texImages.size() > 0);

    auto&    device    = Engine::get().getGfxDevice();
    auto&    vkContext = device.getSharedVulkanContext();

    uint32_t numImages = texImages.size();

    this->width        = texImages[0]->width;
    this->height       = texImages[0]->height;
    this->usage        = usage;
    this->format       = format;
    this->numLayers    = numImages;

    size_t   layerSize = texImages[0]->size;

    this->numMipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;

    if (type == TextureType::Cube) DASSERT(numImages == 6);

    for (auto& img : texImages)
    {
        DASSERT(img->width == width && img->height == height);
    }

    auto imageAspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
    if (usage & DepthStencilTexture)
    {
        imageAspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;
    }

    // staging buffer for transfer
    GfxBuffer stagingBuffer;
    GfxBuffer::createHostWriteBuffer(
        GfxBufferUsageFlags::TransferSource,
        layerSize,
        numImages,
        "staging_texture_2d_buffer",
        &stagingBuffer);

    if (!stagingBuffer.isAllocated())
        return Error::InitializationFailed;

    DynamicArray<VkBufferImageCopy> bufferCopyRegions(numImages);
    for (uint32_t imageIndex = 0u; imageIndex < numImages; ++imageIndex)
    {
        auto& img = texImages[imageIndex];

        // copy image data
        stagingBuffer.writeAndFlushAtIndex(imageIndex, img->data, layerSize);

        VkBufferImageCopy& copyRegion              = bufferCopyRegions[imageIndex];
        copyRegion.imageSubresource.aspectMask     = imageAspectFlags;
        copyRegion.imageSubresource.mipLevel       = 0;
        copyRegion.imageSubresource.baseArrayLayer = imageIndex;
        copyRegion.imageSubresource.layerCount     = 1;
        copyRegion.imageExtent.width               = width;
        copyRegion.imageExtent.height              = height;
        copyRegion.imageExtent.depth               = 1;
        copyRegion.bufferOffset                    = layerSize * imageIndex;
    }

    // create dest image
    VkImageCreateInfo imageInfo { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
    imageInfo.imageType     = vulkan::getImageType(type);
    imageInfo.extent.width  = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth  = 1;
    imageInfo.mipLevels     = numMipLevels;
    imageInfo.arrayLayers   = numImages;
    imageInfo.format        = format;
    imageInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage         = vulkan::getTextureUsageFlagBits(usage) | VK_IMAGE_USAGE_TRANSFER_SRC_BIT; // transfer src for vkCmdBlitImage
    imageInfo.samples       = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;

    if (type == TextureType::Cube)
        imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

    // create image
    VulkanResult result = vulkan::allocateGPUImage(
        vkContext.gpuAllocator,
        imageInfo,
        VMA_MEMORY_USAGE_AUTO,
        VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
        &image);

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
            (uint64_t)image.vkImage,
            debugName);
    }
#endif // VK_RENDERER_DEBUG

    VkCommandBufferBeginInfo beginInfo {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(transferBuffer, &beginInfo);

    VkImageMemoryBarrier barrier {};
    barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout                       = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout                       = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barrier.image                           = image.vkImage;
    barrier.subresourceRange.aspectMask     = imageAspectFlags;
    barrier.subresourceRange.baseMipLevel   = 0;
    barrier.subresourceRange.levelCount     = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount     = numImages;
    barrier.srcAccessMask                   = 0;
    barrier.dstAccessMask                   = VK_ACCESS_MEMORY_WRITE_BIT;

    vkCmdPipelineBarrier(
        transferBuffer,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        0,
        0,
        nullptr,
        0,
        nullptr,
        1,
        &barrier);

    vkCmdCopyBufferToImage(
        transferBuffer,
        stagingBuffer.vkBuffer.buffer,
        image.vkImage,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        bufferCopyRegions.size(),
        bufferCopyRegions.data());

    // release ownership from transfer queue
    barrier                                 = {};
    barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout                       = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout                       = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcQueueFamilyIndex             = vkContext.transferQueueFamilyIndex;
    barrier.dstQueueFamilyIndex             = vkContext.graphicsQueueFamilyIndex;
    barrier.image                           = image.vkImage;
    barrier.subresourceRange.aspectMask     = imageAspectFlags;
    barrier.subresourceRange.baseMipLevel   = 0;
    barrier.subresourceRange.levelCount     = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount     = numImages;
    barrier.srcAccessMask                   = VK_ACCESS_TRANSFER_WRITE_BIT;

    vkCmdPipelineBarrier(
        transferBuffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
        0,
        0,
        nullptr,
        0,
        nullptr,
        1,
        &barrier);

    vkEndCommandBuffer(transferBuffer);

    VkSemaphoreCreateInfo semInfo {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
    };

    VkSemaphore uploadFinishedSemaphore;
    vkCreateSemaphore(vkContext.device, &semInfo, nullptr, &uploadFinishedSemaphore);

    // submit transfer work
    VkSubmitInfo submitInfo {};
    submitInfo.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount   = 1;
    submitInfo.pCommandBuffers      = &transferBuffer;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores    = &uploadFinishedSemaphore;

    vkQueueSubmit(vkContext.transferQueue, 1, &submitInfo, VK_NULL_HANDLE);

    vkBeginCommandBuffer(graphicsBuffer, &beginInfo);

    // graphics queue will require ownership
    barrier                                 = {};
    barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout                       = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout                       = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcQueueFamilyIndex             = vkContext.transferQueueFamilyIndex;
    barrier.dstQueueFamilyIndex             = vkContext.graphicsQueueFamilyIndex;
    barrier.image                           = image.vkImage;
    barrier.subresourceRange.aspectMask     = imageAspectFlags;
    barrier.subresourceRange.baseMipLevel   = 0;
    barrier.subresourceRange.levelCount     = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount     = numImages;
    barrier.dstAccessMask                   = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(
        graphicsBuffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0,
        0,
        nullptr,
        0,
        nullptr,
        1,
        &barrier);

    // start mip genration
    // Initialize remaining mip levels to TRANSFER_DST_OPTIMAL
    barrier                                 = {};
    barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout                       = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout                       = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barrier.image                           = image.vkImage;
    barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel   = 1; // 0 level already transitioned
    barrier.subresourceRange.levelCount     = numMipLevels - 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount     = numImages;
    barrier.srcAccessMask                   = 0;
    barrier.dstAccessMask                   = VK_ACCESS_TRANSFER_WRITE_BIT;

    vkCmdPipelineBarrier(
        graphicsBuffer,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        0,
        0,
        nullptr,
        0,
        nullptr,
        1,
        &barrier);

    // Generate mipmaps from level 1
    barrier.subresourceRange.levelCount = 1;

    int32_t mipWidth                    = width;
    int32_t mipHeight                   = height;

    for (uint32_t i = 1; i < numMipLevels; i++)
    {
        // Transition previous mip level to TRANSFER_SRC_OPTIMAL
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout                     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout                     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask                 = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask                 = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(
            graphicsBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            0,
            0,
            nullptr,
            0,
            nullptr,
            1,
            &barrier);

        // Blit from previous level to current level
        VkImageBlit blit {};
        blit.srcOffsets[0]                 = { 0, 0, 0 };
        blit.srcOffsets[1]                 = { mipWidth, mipHeight, 1 };
        blit.srcSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel       = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount     = numImages;
        blit.dstOffsets[0]                 = { 0, 0, 0 };
        blit.dstOffsets[1]                 = {
            mipWidth > 1 ? mipWidth / 2 : 1,
            mipHeight > 1 ? mipHeight / 2 : 1,
            1
        };
        blit.dstSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel       = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount     = numImages;

        vkCmdBlitImage(
            graphicsBuffer,
            image.vkImage,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            image.vkImage,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &blit,
            VK_FILTER_LINEAR);

        // Transition previous mip to SHADER_READ_ONLY_OPTIMAL
        barrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(
            graphicsBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0,
            0,
            nullptr,
            0,
            nullptr,
            1,
            &barrier);

        if (mipWidth > 1) mipWidth /= 2;
        if (mipHeight > 1) mipHeight /= 2;
    }

    // Transition last mip level to SHADER_READ_ONLY_OPTIMAL
    barrier.subresourceRange.baseMipLevel = numMipLevels - 1;
    barrier.oldLayout                     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout                     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask                 = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask                 = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(
        graphicsBuffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0,
        0,
        nullptr,
        0,
        nullptr,
        1,
        &barrier);

    vkEndCommandBuffer(graphicsBuffer);

    VkPipelineStageFlags waitStages[] = {
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT
    };

    submitInfo                    = {};
    submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers    = &graphicsBuffer;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores    = &uploadFinishedSemaphore;
    submitInfo.pWaitDstStageMask  = waitStages;

    vkQueueSubmit(vkContext.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);

    this->currentLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    // wait for graphics queue operation to finish
    {
        DUSK_PROFILE_SECTION("graphics_queue_wait");
        vkQueueWaitIdle(vkContext.graphicsQueue);
    }

    vkDestroySemaphore(vkContext.device, uploadFinishedSemaphore, nullptr);

    stagingBuffer.free();

    result = device.createImageView(
        &image,
        vulkan::getImageViewType(type),
        format,
        imageAspectFlags,
        numMipLevels,
        numImages,
        &imageView);

    if (result.hasError())
    {
        return Error::InitializationFailed;
    }

    return Error::Ok;
}

void GfxTexture::free()
{
    auto& device    = Engine::get().getGfxDevice();
    auto& vkContext = device.getSharedVulkanContext();

    device.freeImageView(&imageView);

    vulkan::freeGPUImage(vkContext.gpuAllocator, &image);
}

void GfxTexture::recordTransitionLayout(
    VkCommandBuffer cmdbuff,
    VkImageLayout   newLayout)
{
    DASSERT(newLayout != VK_IMAGE_LAYOUT_UNDEFINED);

    if (currentLayout == newLayout) return;

    VkImageAspectFlags imageAspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
    if (vulkan::getTextureUsageFlagBits(usage) & DepthStencilTexture)
    {
        imageAspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    }

    VkImageMemoryBarrier barrier {};
    barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout                       = currentLayout;
    barrier.newLayout                       = newLayout;
    barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barrier.image                           = image.vkImage;
    barrier.subresourceRange.aspectMask     = imageAspectFlags;
    barrier.subresourceRange.baseMipLevel   = 0;
    barrier.subresourceRange.levelCount     = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount     = numLayers;

    VkPipelineStageFlags srcStage           = VK_PIPELINE_STAGE_FLAG_BITS_MAX_ENUM;
    VkPipelineStageFlags dstStage           = VK_PIPELINE_STAGE_FLAG_BITS_MAX_ENUM;

    switch (currentLayout)
    {
        case VK_IMAGE_LAYOUT_UNDEFINED:
        {
            srcStage              = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            barrier.srcAccessMask = 0;
            break;
        }
        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
        {
            srcStage              = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            break;
        }
        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        {
            srcStage              = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            barrier.srcAccessMask = 0;
            break;
        }
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
        {
            srcStage              = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
            barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            break;
        }
        case VK_IMAGE_LAYOUT_GENERAL:
        {
            srcStage              = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
            barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
            break;
        }
        default:
        {
            DUSK_ERROR("Unhandled current layout");
        }
    }

    switch (newLayout)
    {
        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
        {
            dstStage              = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
            break;
        }
        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        {
            dstStage              = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            break;
        }
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
        {
            dstStage              = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
            barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
            break;
        }
        case VK_IMAGE_LAYOUT_GENERAL:
        {
            dstStage              = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
            break;
        }
        default:
        {
            DUSK_ERROR("Unhandled new layout");
        }
    }

    DASSERT(srcStage != VK_PIPELINE_STAGE_FLAG_BITS_MAX_ENUM);
    DASSERT(dstStage != VK_PIPELINE_STAGE_FLAG_BITS_MAX_ENUM);

    vkCmdPipelineBarrier(
        cmdbuff,
        srcStage,
        dstStage,
        0,
        0,
        nullptr,
        0,
        nullptr,
        1,
        &barrier);

    // this layout is not transitioned ye. This is the next
    // state of the layout and any further transition should respect
    // this
    currentLayout = newLayout;
}

} // namespace dusk