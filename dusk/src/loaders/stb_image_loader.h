#pragma once

#include "dusk.h"
#include "renderer/image.h"

#include <string>

namespace dusk
{

class ImageLoader
{
public:
    ImageLoader() = default;
    ~ImageLoader() = default;

    static Unique<Image> readImage(const std::string& filepath);
};

} // namespace dusk