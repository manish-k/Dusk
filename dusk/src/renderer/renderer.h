#pragma once

#include "dusk.h"
#include "platform/window.h"

namespace dusk
{
class Renderer
{
public:
    virtual ~Renderer()    = default;

    virtual bool init()    = 0;
    virtual void cleanup() = 0;
};
} // namespace dusk