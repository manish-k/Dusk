#pragma once

#include "dusk.h"
#include "gfx_buffer.h"

namespace dusk
{
class GfxDevice
{
public:
    virtual ~GfxDevice()                      = default;

    virtual Error      initGfxDevice()        = 0;
    virtual void       cleanupGfxDevice()     = 0;
    virtual GfxBuffer* createBuffer()         = 0;
    virtual void       freeBuffer(GfxBuffer*) = 0;
};
} // namespace dusk