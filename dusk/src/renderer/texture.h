#pragma once

#include "dusk.h"
#include "backend/vulkan/vk_types.h"

#include <thread>

namespace dusk
{
struct ImageData;
class GfxBuffer;

enum TextureUsageFlags : uint32_t
{
    TransferSrcTexture  = 0x00000001,
    TransferDstTexture  = 0x00000002,
    SampledTexture      = 0x00000004,
    StorageTexture      = 0x00000008,
    ColorTexture        = 0x00000010,
    DepthStencilTexture = 0x00000020,
};

enum class TextureType : uint32_t
{
    Texture1D,
    Texture2D,
    Texture2DArray,
    Texture3D,
    Cube,
    CubeArray,
};

struct GfxTexture
{
    uint32_t       id;
    uint32_t       width         = 0u;
    uint32_t       height        = 0u;
    uint32_t       numLayers     = 1u;
    uint32_t       numMipLevels  = 1u;
    std::string    name          = "";
    uint32_t       usage         = 0u;
    TextureType    type          = TextureType::Texture2D;
    size_t         uploadHash    = {}; // used for tracking ongoing uploads

    VulkanGfxImage image         = {};
    VkImageView    imageView     = {};
    VkFormat       format        = {};
    VkImageLayout  currentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkSampler      sampler       = {};

    // array image view for using cubemaps/texture arrays as output attachments because
    // image view type is different from view that samples the image.
    // We are going to store per mip image views of all faces
    DynamicArray<VkImageView> perMipArrayImageViews = {};

    // Pixel data will be stored in mip-major order
    // [Mip 0, Face 0] [Mip 0, Face 1] ... [Mip 0, Face 5]
    // [Mip 1, Face 0] [Mip 1, Face 1] ... [Mip 1, Face 5]
    // ...
    Shared<GfxBuffer>          pixelData  = nullptr;
    DynamicArray<VkDeviceSize> mipOffsets = {};

    GfxTexture(uint32_t id) :
        id(id) { };
    GfxTexture()  = default;
    ~GfxTexture() = default;

    /**
     * @brief Initialize the texture with the given image and upload it to
     * gpu memory. It will upload the texture using single graphics queue
     * @params Reference to the image
     * @param usage  flags of the texture
     * @params Optional name for the texture resources
     * @return Error status for the complete operation
     */
    Error init(
        ImageData&  texImage,
        VkFormat    format,
        uint32_t    usage,
        const char* name = nullptr);

    /**
     * @brief Initialize an empty texture
     * @param type of texture
     * @param width of the texture
     * @param height of the texture
     * @param number of mip levels in the texture
     * @param number of layers in the texture
     * @param format of the texture
     * @param usage  flags of the texture
     * @param name  of the texture
     * @return Error value of the creation call
     */
    Error init(
        TextureType type,
        uint32_t    width,
        uint32_t    height,
        uint32_t    mipLevels,
        uint32_t    layers,
        VkFormat    format,
        uint32_t    usage,
        const char* name = nullptr);

    /**
     * @brief Initialize texture and use given command buffers for upload.
     * It will use texture queue to upload and then transfer ownership
     * to graphics queue for usage.
     * @param Image data
     * @param type of texture
     * @param usage flags
     * @param graphicsBuffer command buffer corresponding to graphics queue
     * @param transferBuffer command buffer corresponding to transfer queue
     * @param name Optional name for the texture resources
     * @return Error status for the complete operation
     */
    Error init(
        ImageData&      texImage,
        TextureType     type,
        VkFormat        format,
        uint32_t        usage,
        VkCommandBuffer graphicsBuffer,
        VkCommandBuffer transferBuffer,
        bool            generateMips,
        const char*     name = nullptr);

    /**
     * @brief Record cmds for generating mip maps
     * @param cmdBuff 
     */
    void recordMipGenerationCmds(VkCommandBuffer cmdBuff);

    /**
     * @brief free the allocated Image and ImageView for the texture
     */
    void free();

    /**
     * @brief Download pixel data into a host visible buffer
     */
    void downloadPixelData();

    /**
     * @brief Write host visible pixel data in a png file
     * @param filePath
     */
    void writePixelDataAsPNG(const std::string& filePath);

    /**
     * @brief Write host visible pixel data in a ktx2 file
     * @param filePath
     */
    void writePixelDataAsKTX(const std::string& filePath);

    /**
     * @brief get vulkan image
     * @return VkImage
     */
    VkImage getVkImage() const { return image.vkImage; };

    /**
     * @brief get vulkan image views
     * @return VkImageView
     */
    VkImageView getVkImagView() const { return imageView; };

    /**
     * @brief record transition layout cmd in the cmdbuffer
     * @param recording command buffer
     * @param new layout for the image
     */
    void recordTransitionLayout(
        VkCommandBuffer cmdbuff,
        VkImageLayout   newLayout);
};

} // namespace dusk