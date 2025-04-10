#include "stb_image_loader.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace dusk
{
Unique<Image> ImageLoader::readImage(const std::string& filepath)
{
    stbi_set_flip_vertically_on_load(true);

    Image img {};

    img.data = stbi_load(filepath.c_str(), &img.width, &img.height, &img.channels, 0);

    if (!img.data)
    {
        return nullptr;
    }

    img.size = img.width * img.height * img.channels;

    return createUnique<Image>(std::move(img));
}

void ImageLoader::freeImage(Image& img)
{
    stbi_image_free(img.data);

    img.data     = nullptr;
    img.width    = 0;
    img.height   = 0;
    img.channels = 0;
    img.size     = 0;
}

} // namespace dusk