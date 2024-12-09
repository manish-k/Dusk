#pragma once

#ifdef DDEBUG
#    define VK_RENDERER_DEBUG
#    define VK_ENABLE_VALIDATION_LAYERS
#else
#    define VK_ENABLE_VALIDATION_LAYERS
#endif
