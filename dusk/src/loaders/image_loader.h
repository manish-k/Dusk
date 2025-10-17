#pragma once

#include "dusk.h"

#include <string>

namespace dusk
{
class ImageData;

class ImageLoader
{
public:
    static Shared<ImageData> load(const std::string& filePath);
    static Shared<ImageData> loadSTB(const std::string& filePath);
    static Shared<ImageData> loadKTX2(const std::string& filePath);

    static bool              savePNG(const std::string& filePath);
    static bool              saveKTX2(const std::string& filePath);
};
} // namespace dusk