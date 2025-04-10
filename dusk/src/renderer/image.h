#pragma once

namespace dusk
{
struct Image
{
    int            width;
    int            height;
    int            channels;
    size_t         size;
    unsigned char* data;
};
} // namespace dusk
