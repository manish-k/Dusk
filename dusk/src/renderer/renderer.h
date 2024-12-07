#pragma once

#include "platform/window.h"

namespace dusk
{
class Renderer
{
public:
    virtual ~Renderer() = default;

    virtual bool init() = 0;
};
} // namespace dusk