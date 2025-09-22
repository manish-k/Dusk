#include "stb_image_loader.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace dusk
{
Unique<Image> ImageLoader::readImage(const std::string& filepath)
{
    DUSK_DEBUG("Reading texture file {}", filepath);

    stbi_set_flip_vertically_on_load(true);

    const char* file = filepath.c_str();
    auto        img  = createUnique<Image>();

    int         channelSizeInBytes = 1;

    if (stbi_is_hdr(file))
    {
        img->isHDR = true;
        channelSizeInBytes = 4;
        img->data  = stbi_loadf(
            file,
            &img->width,
            &img->height,
            &img->channels,
            4); // default 4 channels RGBA
    }
    else
    {
        img->data = stbi_load(
            file,
            &img->width,
            &img->height,
            &img->channels,
            4); // default 4 channels RGBA
    }

    // we have already forced to read into 4 channels
    img->channels = 4;

    img->size     = img->width * img->height * img->channels * channelSizeInBytes;

    return img;
}

} // namespace dusk