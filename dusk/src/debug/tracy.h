#pragma once

#ifdef DUSK_ENABLE_PROFILING
#    define TRACY_CALLSTACK 32

#    include <volk.h>

#    include <tracy/Tracy.hpp>
#    include <tracy/TracyVulkan.hpp>
#endif