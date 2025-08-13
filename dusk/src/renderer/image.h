#pragma once

#include "dusk.h"

namespace dusk
{
// TODO:: make class uncopyable. Currently doesn't follow RAII
struct Image
{
    int    width    = 0;
    int    height   = 0;
    int    channels = 0;
    size_t size     = 0;
    void*  data     = nullptr;
    bool   isHDR    = false;

    Image()         = default;
    ~Image()
    {
        if (data) delete[] data;
    }
};

} // namespace dusk
