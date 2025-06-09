#pragma once

#include "dusk.h"

namespace dusk
{
// TODO:: make class uncopyable
struct Image
{
    int            width    = 0;
    int            height   = 0;
    int            channels = 0;
    size_t         size     = 0;
    unsigned char* data     = nullptr;

    Image()                 = default;
};
} // namespace dusk
