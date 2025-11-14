#pragma once

#include "dusk.h"
#include "renderer/image.h"

#include <string>

namespace dusk
{
class GfxTexture;

struct ImageData;

class ImageLoader
{
public:
    /**
     * @brief load an image file
     * @param path to file 
     * @return shared ptr for image data object
     */
    static Shared<ImageData> load(const std::string& filePath, PixelFormat format);
    
    /**
     * @brief load an image file with stb loader
     * @param path to file 
     * @return shared ptr for image data object
     */
    static Shared<ImageData> loadSTB(const std::string& filePath, PixelFormat format);
    
    /**
     * @brief load ktx and ktx2 image files
     * @param path to file 
     * @return shared ptr for image data object
     */
    static Shared<ImageData> loadKTX(const std::string& filePath, PixelFormat format);

    /**
     * @brief Save the texture as png file on disk
     * @param path to file
     * @param texture
     * @return true for success else false
     */
    static bool savePNG(
        const std::string& filePath,
        GfxTexture&        texture);
    /**
     * @brief Save the texture as ktx2 file on disk
     * @param path to file
     * @param texture
     * @return true for success else false
     */
    static bool saveKTX2(
        const std::string& filePath,
        GfxTexture&        texture);
};
} // namespace dusk