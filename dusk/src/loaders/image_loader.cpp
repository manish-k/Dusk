#include "image_loader.h"

#include "renderer/image.h"
#include "utils/utils.h"
#include "backend/vulkan/vk.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <ktx.h>
#include <ktxvulkan.h>

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
    DUSK_DEBUG("Reading texture file - {}", filePath);

    stbi_set_flip_vertically_on_load(true);

    const char* file               = filePath.c_str();
    auto        img                = createShared<ImageData>();
    int         channelSizeInBytes = 1;
    const int   defaultNumChannels = 4;

    if (stbi_is_hdr(file))
    {
        channelSizeInBytes = 4; // floats
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

    // we have taken ownership of pixel data in our struct,
    // removing ownership from ktx so data won't be freed
    ktxTex->pData = nullptr;

    // destroy metadata
    ktxTexture_Destroy(ktxTexture(ktxTex));

    return img;
}
} // namespace dusk