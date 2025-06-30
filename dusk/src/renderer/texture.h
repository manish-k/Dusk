#pragma once

#include "dusk.h"
#include "backend/vulkan/vk_types.h"

namespace dusk
{
class Image;

struct Texture2D
{
    uint32_t      id;
    uint32_t      width       = 0;
    uint32_t      height      = 0;
    uint32_t      numChannels = 0;
    std::string   name        = "";

    VulkanTexture vkTexture {};

    Texture2D(uint32_t id) :
        id(id) { };
    ~Texture2D() = default;

    /**
     * @brief Initialize the texture with the given image and upload it to
     * gpu memory. It will upload the texture using single graphics queue
     * @params Reference to the image
     * @params Optional name for the texture resources
     * @return Error status for the complete operation
     */
    Error init(Image& texImage, const char* debugName = nullptr);
    
    /**
     * @brief Initialize texture and record upload commands into the given
     * buffers. It will use texture queue to upload and then transfer ownership
     * to graphics queue for usage.
     * @params command buffer corresponding to graphics queue
     * @params command buffer corresponding to transfer queue
     * @params Optional name for the texture resources
     * @return Error status for the complete operation
     */
    Error initAndRecordUpload(
        Image&          texImage,
        VkCommandBuffer graphicsBuffer,
        VkCommandBuffer transferBuffer,
        const char*     debugName = nullptr);
    
    /**
     * @brief free the allocated Image and ImageView for the texture
     */
    void free();
};

struct Texture3D
{
    uint32_t      id;
    uint32_t      width       = 0;
    uint32_t      height      = 0;
    uint32_t      numChannels = 0;
    uint32_t      numImages   = 0;

    VulkanTexture vkTexture {};
    VulkanSampler vkSampler {}; // TODO: sampler can be a common one instead of per texture

    Texture3D(uint32_t id) :
        id(id) { };
    ~Texture3D() = default;

    Error init(DynamicArray<Image>& texImages); // TODO: input params not ideal, maybe support with shared ptr?
    void  free();
};

} // namespace dusk