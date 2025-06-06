#pragma once

#ifdef DUSK_ENABLE_PROFILING
#    include "backend/vulkan/vk_device.h"

#    include <volk.h>
#    include <tracy/Tracy.hpp>
#    include <tracy/TracyVulkan.hpp>

#    define DUSK_PROFILE_FUNCTION      ZoneScoped
#    define DUSK_PROFILE_FRAME         FrameMark
#    define DUSK_PROFILE_THREAD(name)  tracy::SetThreadName(name)
#    define DUSK_PROFILE_SECTION(name) ZoneScopedN(name)
#    define DUSK_PROFILE_GPU_ZONE(name, cmdBuffer) \
        TracyVkZoneC(VkGfxDevice::getProfilerContext(), cmdBuffer, name);

#    define DUSK_PROFILE_TAG(y, x)          ZoneText(x, strlen(x))
#    define DUSK_PROFILE_LOG(text, size)    TracyMessage(text, size)
#    define DUSK_PROFILE_VALUE(text, value) TracyPlot(text, value)

#else

#    define DUSK_PROFILE_FUNCTION
#    define DUSK_PROFILE_FRAME(name)
#    define DUSK_PROFILER_THREAD(name)
#    define DUSK_PROFILE_SECTION(name)
#    define DUSK_PROFILE_GPU_ZONE(name, cmdBuffer)

#    define DUSK_PROFILE_TAG(y, x)
#    define DUSK_PROFILE_LOG(text, size)
#    define DUSK_PROFILE_VALUE(text, value)

#endif