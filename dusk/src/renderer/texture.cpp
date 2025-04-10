#include "texture.h"

#include "loaders/stb_image_loader.h"

namespace dusk
{
Unique<Texture> Texture::loadFromFile(const std::string& filepath)
{
    Unique<Image> img = ImageLoader::readImage(filepath);

    ImageLoader::freeImage(*img);
    
    return nullptr;
}

} // namespace dusk