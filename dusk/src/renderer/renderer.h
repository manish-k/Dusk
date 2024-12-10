#pragma once

#include "dusk.h"
#include "platform/window.h"

namespace dusk
{
class Renderer
{
public:
    virtual ~Renderer()                                      = default;

    virtual bool init(const char* appName, uint32_t version) = 0;
};
} // namespace dusk