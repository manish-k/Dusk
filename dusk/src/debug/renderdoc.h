#pragma once

#include "dusk.h"
#include "platform/platform.h"

#include <renderdoc_app.h>

namespace dusk
{
namespace renderdoc
{
RENDERDOC_API_1_6_0* rdoc_api = NULL;

/**
 * @brief Initialize the renderdoc api
 * @return true if initialization is successful
 */
inline bool init()
{
#ifdef _WIN32
    if (HMODULE mod = GetModuleHandleA("renderdoc.dll"))
    {
        pRENDERDOC_GetAPI RENDERDOC_GetAPI = (pRENDERDOC_GetAPI)GetProcAddress(mod, "RENDERDOC_GetAPI");
        int               ret              = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_1_2, (void**)&rdoc_api);
        if (ret == 1) return true;
        return false;
    }
#elif DUSK_PLATFORM_UNIX || DUSK_PLATFORM_ANDROID

    if (void* mod = dlopen("librenderdoc.so", RTLD_NOW | RTLD_NOLOAD))
    {
        pRENDERDOC_GetAPI RENDERDOC_GetAPI = (pRENDERDOC_GetAPI)dlsym(mod, "RENDERDOC_GetAPI");
        int               ret              = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_4_1, (void**)&rdoc_api);
        if (ret == 1) return true;
        return false;
    }
#endif

    return false;
}

} // namespace renderdoc
} // namespace dusk