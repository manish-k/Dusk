#pragma once

#include "core/buffer.h"

#include <filesystem>

namespace dusk
{
class FileSystem
{
public:
    static Buffer readFileBinary(const std::filesystem::path& filepath);
};
} // namespace dusk