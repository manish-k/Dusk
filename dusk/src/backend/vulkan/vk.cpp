#include "vk.h"

namespace dusk
{
Error VulkanResult::getErrorId() const
{
    switch (vkResult)
    {
        case VK_SUCCESS:                     return Error::Ok;
        case VK_NOT_READY:                   return Error::TimeOut;
        case VK_TIMEOUT:                     return Error::TimeOut;
        case VK_INCOMPLETE:                  return Error::BufferTooSmall;
        case VK_ERROR_OUT_OF_HOST_MEMORY:    return Error::OutOfMemory;
        case VK_ERROR_OUT_OF_DEVICE_MEMORY:  return Error::OutOfMemory;
        case VK_ERROR_INITIALIZATION_FAILED: return Error::InitializationFailed;
        case VK_ERROR_DEVICE_LOST:           return Error::DeviceLost;
        case VK_ERROR_MEMORY_MAP_FAILED:     return Error::MemoryMapFailed;
        case VK_ERROR_LAYER_NOT_PRESENT:     return Error::NotFound;
        case VK_ERROR_EXTENSION_NOT_PRESENT: return Error::NotFound;
        case VK_ERROR_FEATURE_NOT_PRESENT:   return Error::NotFound;
        case VK_ERROR_INCOMPATIBLE_DRIVER:   return Error::WrongVersion;
        case VK_ERROR_TOO_MANY_OBJECTS:      return Error::OutOfMemory;
        case VK_ERROR_FORMAT_NOT_SUPPORTED:
            return Error::NotSupported;
        case VK_EVENT_SET: return Error::Generic;
        case VK_EVENT_RESET:
            return Error::Generic;

        default: return Error::Generic;
    }
}

std::string VulkanResult::toString() const
{
    std::stringstream ss;

    switch (vkResult)
    {
        case VK_SUCCESS:                                            ss << "VK_SUCCESS";
        case VK_NOT_READY:                                          ss << "VK_NOT_READY";
        case VK_TIMEOUT:                                            ss << "VK_TIMEOUT";
        case VK_EVENT_SET:                                          ss << "VK_EVENT_SET";
        case VK_EVENT_RESET:                                        ss << "VK_EVENT_RESET";
        case VK_INCOMPLETE:                                         ss << "VK_INCOMPLETE";
        case VK_ERROR_OUT_OF_HOST_MEMORY:                           ss << "VK_ERROR_OUT_OF_HOST_MEMORY";
        case VK_ERROR_OUT_OF_DEVICE_MEMORY:                         ss << "VK_ERROR_OUT_OF_DEVICE_MEMORY";
        case VK_ERROR_INITIALIZATION_FAILED:                        ss << "VK_ERROR_INITIALIZATION_FAILED";
        case VK_ERROR_DEVICE_LOST:                                  ss << "VK_ERROR_DEVICE_LOST";
        case VK_ERROR_MEMORY_MAP_FAILED:                            ss << "VK_ERROR_MEMORY_MAP_FAILED";
        case VK_ERROR_LAYER_NOT_PRESENT:                            ss << "VK_ERROR_LAYER_NOT_PRESENT";
        case VK_ERROR_EXTENSION_NOT_PRESENT:                        ss << "VK_ERROR_EXTENSION_NOT_PRESENT";
        case VK_ERROR_FEATURE_NOT_PRESENT:                          ss << "VK_ERROR_FEATURE_NOT_PRESENT";
        case VK_ERROR_INCOMPATIBLE_DRIVER:                          ss << "VK_ERROR_INCOMPATIBLE_DRIVER";
        case VK_ERROR_TOO_MANY_OBJECTS:                             ss << "VK_ERROR_TOO_MANY_OBJECTS";
        case VK_ERROR_FORMAT_NOT_SUPPORTED:                         ss << "VK_ERROR_FORMAT_NOT_SUPPORTED";
        case VK_ERROR_FRAGMENTED_POOL:                              ss << "VK_ERROR_FRAGMENTED_POOL";
        case VK_ERROR_UNKNOWN:                                      ss << "VK_ERROR_UNKNOWN";
        case VK_ERROR_OUT_OF_POOL_MEMORY:                           ss << "VK_ERROR_OUT_OF_POOL_MEMORY";
        case VK_ERROR_INVALID_EXTERNAL_HANDLE:                      ss << "VK_ERROR_INVALID_EXTERNAL_HANDLE";
        case VK_ERROR_FRAGMENTATION:                                ss << "VK_ERROR_FRAGMENTATION";
        case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS:               ss << "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS";
        case VK_PIPELINE_COMPILE_REQUIRED:                          ss << "VK_PIPELINE_COMPILE_REQUIRED";
        case VK_ERROR_SURFACE_LOST_KHR:                             ss << "VK_ERROR_SURFACE_LOST_KHR";
        case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:                     ss << "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
        case VK_SUBOPTIMAL_KHR:                                     ss << "VK_SUBOPTIMAL_KHR";
        case VK_ERROR_OUT_OF_DATE_KHR:                              ss << "VK_ERROR_OUT_OF_DATE_KHR";
        case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:                     ss << "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
        case VK_ERROR_VALIDATION_FAILED_EXT:                        ss << "VK_ERROR_VALIDATION_FAILED_EXT";
        case VK_ERROR_INVALID_SHADER_NV:                            ss << "VK_ERROR_INVALID_SHADER_NV";
        case VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR:                ss << "VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR";
        case VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR:       ss << "VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR";
        case VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR:    ss << "VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR";
        case VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR:       ss << "VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR";
        case VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR:        ss << "VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR";
        case VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR:          ss << "VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR";
        case VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT: ss << "VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT";
        case VK_ERROR_NOT_PERMITTED_KHR:                            ss << "VK_ERROR_NOT_PERMITTED_KHR";
        case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT:          ss << "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT";
        case VK_THREAD_IDLE_KHR:                                    ss << "VK_THREAD_IDLE_KHR";
        case VK_THREAD_DONE_KHR:                                    ss << "VK_THREAD_DONE_KHR";
        case VK_OPERATION_DEFERRED_KHR:                             ss << "VK_OPERATION_DEFERRED_KHR";
        case VK_OPERATION_NOT_DEFERRED_KHR:                         ss << "VK_OPERATION_NOT_DEFERRED_KHR";
        case VK_ERROR_COMPRESSION_EXHAUSTED_EXT:                    ss << "VK_ERROR_COMPRESSION_EXHAUSTED_EXT";
        case VK_ERROR_INCOMPATIBLE_SHADER_BINARY_EXT:               ss << "VK_ERROR_INCOMPATIBLE_SHADER_BINARY_EXT";
        case VK_ERROR_INVALID_VIDEO_STD_PARAMETERS_KHR:             ss << "VK_ERROR_INVALID_VIDEO_STD_PARAMETERS_KHR";
        case VK_RESULT_MAX_ENUM:                                    break;
    }
    return ss.str();
}
} // namespace dusk