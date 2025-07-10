#pragma once

#include "dusk.h"
#include "backend/vulkan/vk_types.h"

namespace dusk
{
class Image;

enum TextureUsageFlags : uint32_t
{
    TransferSrcTexture  = 0x00000001,
    TransferDstTexture  = 0x00000002,
    SampledTexture      = 0x00000004,
    StorageTexture      = 0x00000008,
    ColorTexture        = 0x00000010,
    DepthStencilTexture = 0x00000020,
};

struct Texture2D
{
    uint32_t       id;
    uint32_t       width       = 0;
    uint32_t       height      = 0;
    uint32_t       numChannels = 0;
    std::string    name        = "";
    uint32_t       usage       = 0u;

    VulkanGfxImage image       = {};
    VkImageView    imageView   = {};
    VkFormat       format      = {};

    Texture2D(uint32_t id) :
        id(id) { };
    Texture2D()  = default;
    ~Texture2D() = default;

    /**
     * @brief Initialize the texture with the given image and upload it to
     * gpu memory. It will upload the texture using single graphics queue
     * @params Reference to the image
     * @param usage  flags of the texture
     * @params Optional name for the texture resources
     * @return Error status for the complete operation
     */
    Error init(
        Image&      texImage,
        VkFormat    format,
        uint32_t    usage,
        const char* name = nullptr);

    /**
     * @brief Initialize an empty texture
     * @param width of the texture
     * @param height of the texture
     * @param format of the texture
     * @param usage  flags of the texture
     * @param name  of the texture
     * @return Error value of the creation call
     */
    Error init(
        uint32_t    width,
        uint32_t    height,
        VkFormat    format,
        uint32_t    usage,
        const char* name = nullptr);

    /**
     * @brief Initialize texture and record upload commands into the given
     * buffers. It will use texture queue to upload and then transfer ownership
     * to graphics queue for usage.
     * @param texImage Image object
     * @param usage flags
     * @param graphicsBuffer command buffer corresponding to graphics queue
     * @param transferBuffer command buffer corresponding to transfer queue
     * @param name Optional name for the texture resources
     * @return Error status for the complete operation
     */
    Error initAndRecordUpload(
        Image&          texImage,
        VkFormat        format,
        uint32_t        usage,
        VkCommandBuffer graphicsBuffer,
        VkCommandBuffer transferBuffer,
        const char*     name = nullptr);

    /**
     * @brief free the allocated Image and ImageView for the texture
     */
    void free();
};

struct Texture3D
{
    uint32_t       id;
    uint32_t       width       = 0;
    uint32_t       height      = 0;
    uint32_t       numChannels = 0;
    uint32_t       numImages   = 0;

    VulkanGfxImage image       = {};
    VkImageView    imageView   = {};
    VulkanSampler  vkSampler   = {}; // TODO: sampler can be a common one instead of per texture

    Texture3D(uint32_t id) :
        id(id) { };
    ~Texture3D() = default;

    Error init(DynamicArray<Image>& texImages); // TODO: input params not ideal, maybe support with shared ptr?
    void  free();
};

} // namespace dusk