#include "file_system.h"

#include "dusk.h"

#include <fstream>

namespace dusk
{
DynamicArray<char> FileSystem::readFileBinary(const std::filesystem::path& filepath)
{
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);

    if (!file)
    {
        DUSK_ERROR("Failed to open file {}", filepath.generic_string());
        return DynamicArray<char>(0);
    }

    size_t filesize = static_cast<size_t>(file.tellg());

    file.seekg(0, std::ios::beg);

    if (filesize == 0)
    {
        return DynamicArray<char>(0);
    }

    DynamicArray<char> buffer(filesize);

    file.read(buffer.data(), filesize);

    file.close();

    return buffer;
}

} // namespace dusk