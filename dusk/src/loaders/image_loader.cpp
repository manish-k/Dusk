#include "image_loader.h"

#include "renderer/image.h"
#include "renderer/texture.h"
#include "utils/utils.h"
#include "backend/vulkan/vk.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>
#include <ktx.h>

namespace dusk
{
Shared<ImageData> ImageLoader::load(const std::string& filePath)
{
    const std::string fileEXT = getFileExtension(filePath);

    if (fileEXT == ".ktx")
    {
        return loadKTX2(filePath);
    }

    return loadSTB(filePath);
}

Shared<ImageData> ImageLoader::loadSTB(const std::string& filePath)
{
    DUSK_DEBUG("Reading texture file with stb - {}", filePath);

    stbi_set_flip_vertically_on_load(true);

    const char*    file               = filePath.c_str();
    auto           img                = createShared<ImageData>();
    uint32_t       channelSizeInBytes = 1u;
    const uint32_t defaultNumChannels = 4u;

    if (stbi_is_hdr(file))
    {
        channelSizeInBytes = 4u; // floats
        img->data          = stbi_loadf(
            file,
            &img->width,
            &img->height,
            nullptr,
            defaultNumChannels); // forcing 4 channels RGBA

        // currently using 32  bit per channel for hdr files
        // In case of 16 bit half format we need conversion
        // during loading hdr files
        img->format = PixelFormat::R32G32B32A32_sfloat;
    }
    else
    {
        img->data = stbi_load(
            file,
            &img->width,
            &img->height,
            nullptr,
            defaultNumChannels); // forcing 4 channels RGBA

        img->format = PixelFormat::R8G8B8A8_uint;
    }

    img->numMipLevels = 1; // base mip
    img->numLayers    = 1;
    img->size         = img->width * img->height * defaultNumChannels * channelSizeInBytes;

    return img;
}

Shared<ImageData> ImageLoader::loadKTX2(const std::string& filePath)
{
    DUSK_DEBUG("Reading ktx texture file - {}", filePath);

    ktxTexture2*   ktxTex;
    KTX_error_code result = ktxTexture2_CreateFromNamedFile(
        filePath.c_str(),
        KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,
        &ktxTex);

    if (result != KTX_SUCCESS)
    {
        DUSK_ERROR("Unable to read ktx file {}", filePath);
        return nullptr;
    }

    auto img          = createShared<ImageData>();

    img->width        = ktxTex->baseWidth;
    img->height       = ktxTex->baseHeight;
    img->data         = ktxTexture_GetData(ktxTexture(ktxTex));
    img->size         = ktxTexture_GetDataSize(ktxTexture(ktxTex));
    img->numMipLevels = ktxTex->numLevels;
    img->numFaces     = ktxTex->numFaces; // 6 for cubemap else 1
    img->numLayers    = ktxTex->numLayers;
    img->format       = vulkan::getPixelFormat((VkFormat)ktxTex->vkFormat);

    // we have taken ownership of pixel data in ImageData struct,
    // removing ownership from ktx so that pixel data won't be freed
    ktxTex->pData = nullptr;

    // destroy metadata
    ktxTexture_Destroy(ktxTexture(ktxTex));

    return img;
}

bool ImageLoader::savePNG(
    const std::string& filePath,
    GfxTexture&        texture)
{
    DUSK_DEBUG("Saving texture file as png - {}", filePath);

    DASSERT(texture.pixelData, "Pixel data not present");

    bool result = stbi_write_png(
        filePath.c_str(),
        texture.width,
        texture.height,
        4,
        texture.pixelData,
        0);

    return result; // 1 -> success, 0 -> failure
}

bool ImageLoader::saveKTX2(
    const std::string& filePath,
    GfxTexture&        texture)
{
    DUSK_DEBUG("Saving texture file as ktx2 - {}", filePath);

    DASSERT(texture.pixelData, "Pixel data not present");

    ktxTextureCreateInfo createInfo = {};
    createInfo.vkFormat             = texture.format;
    createInfo.baseWidth            = texture.width;
    createInfo.baseHeight           = texture.height;
    createInfo.baseDepth            = 1;
    createInfo.numDimensions        = 2;
    createInfo.numLevels            = texture.numMipLevels;
    createInfo.numFaces             = texture.numLayers; // ktx defintion of layers is different from vulkan
    createInfo.isArray              = KTX_FALSE;
    createInfo.generateMipmaps      = KTX_FALSE;

    // ktx2 tex
    ktxTexture2*   ktxTex = nullptr;
    KTX_error_code result = ktxTexture2_Create(
        &createInfo,
        KTX_TEXTURE_CREATE_ALLOC_STORAGE,
        &ktxTex);
    if (result != KTX_SUCCESS)
    {
        DUSK_ERROR("Unable to create ktx texture for saving {}", filePath);
        return false;
    }

    // copy mip levels and faces
    for (uint32_t mip = 0u; mip < texture.numMipLevels; ++mip)
    {
        uint32_t     mipWidth  = std::max(1u, texture.width >> mip);
        uint32_t     mipHeight = std::max(1u, texture.height >> mip);
        VkDeviceSize faceSize  = mipWidth * mipHeight * vulkan::getBytesPerPixel(texture.format);

        for (uint32_t face = 0u; face < texture.numLayers; face++)
        {
            uint8_t* mipData = (uint8_t*)texture.pixelData + texture.mipOffsets[mip] + face * faceSize;

            // save face
            result = ktxTexture_SetImageFromMemory(
                ktxTexture(ktxTex),
                mip,
                0, // ktx layer, not same as vulkan layer
                face,
                mipData,
                faceSize);

            if (result != KTX_SUCCESS)
            {
                DUSK_ERROR("Unable to save mip {} face {}for {}", mip, face, filePath);
                return false;
            }
        }
    }

    ktxTexture_WriteToNamedFile(ktxTexture(ktxTex), filePath.c_str());
    ktxTexture_Destroy(ktxTexture(ktxTex));
}

} // namespace dusk