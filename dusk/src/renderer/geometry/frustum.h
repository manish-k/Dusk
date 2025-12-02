#pragma once

#include "dusk.h"

#include "plane.h"

namespace dusk
{
struct Frustum
{
    Plane top    = {};
    Plane bottom = {};

    Plane left   = {};
    Plane right  = {};

    Plane near   = {};
    Plane far    = {};
};
} // namespace dusk
