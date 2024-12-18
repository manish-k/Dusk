#include "file_system.h"

#include <fstream>

namespace dusk
{
Buffer FileSystem::readFileBinary(const std::filesystem::path& filepath)
{
    std::ifstream stream(filepath, std::ios::binary | std::ios::ate);

    if (!stream)
    {
        return Buffer(0);
    }

    std::streampos end = stream.tellg();
    stream.seekg(0, std::ios::beg);
    size_t size = end - stream.tellg();

    if (size == 0)
    {
        return Buffer(0);
    }

    Buffer buffer(size);
    stream.read(buffer.as<char>(), size);
    stream.close();
    return buffer;
}

} // namespace dusk