#pragma once

#include "dusk.h"
#include "core/buffer.h"

#include <filesystem>

namespace dusk
{
class FileSystem
{
public:
    static DynamicArray<char> readFileBinary(const std::filesystem::path& filepath);
};
} // namespace dusk