#include "stb_image_loader.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace dusk
{
Unique<Image> ImageLoader::readImage(const std::string& filepath)
{
    stbi_set_flip_vertically_on_load(true);

    Image img {};

    auto  rawImgData = stbi_load(filepath.c_str(), &img.width, &img.height, &img.channels, 0);

    if (!rawImgData)
    {
        return nullptr;
    }

    DASSERT(img.channels == 3 || img.channels == 4);

    img.data = new unsigned char[img.width * img.height * 4];

    if (img.channels == 3)
    {
        for (uint32_t i = 0u; i < img.width * img.height; ++i)
        {
            img.data[i * 4 + 0] = rawImgData[i * 3 + 0];
            img.data[i * 4 + 1] = rawImgData[i * 3 + 1];
            img.data[i * 4 + 2] = rawImgData[i * 3 + 2];
            img.data[i * 4 + 3] = 255; // default opacity for rgb img
        }
    }
    else
    {
        // TODO:: extra copying might not be required
        memcpy(img.data, rawImgData, img.width * img.height * 4);
    }

    stbi_image_free(rawImgData);

    img.channels = 4;
    img.size     = img.width * img.height * img.channels;

    return createUnique<Image>(std::move(img));
}

void ImageLoader::freeImage(Image& img)
{
    img.width    = 0;
    img.height   = 0;
    img.channels = 0;
    img.size     = 0;

    delete[] img.data;
}

} // namespace dusk