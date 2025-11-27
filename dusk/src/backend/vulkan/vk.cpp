#include "vk.h"
#include "renderer/gfx_buffer.h"
#include "renderer/texture.h"
#include "renderer/vertex.h"

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

const char* vulkan::getDeviceTypeString(VkPhysicalDeviceType deviceType)
{
    switch (deviceType)
    {
        case VK_PHYSICAL_DEVICE_TYPE_OTHER:          return "other";
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: return "integrated gpu";
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:   return "discrete gpu";
        case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:    return "virtual gpu";
        case VK_PHYSICAL_DEVICE_TYPE_CPU:            return "cpu";
        case VK_PHYSICAL_DEVICE_TYPE_MAX_ENUM:       break;
    }
    return "unknown";
}

const char* vulkan::getVkFormatString(VkFormat format)
{
    switch (format)
    {
        case VK_FORMAT_UNDEFINED:                   return "VK_FORMAT_UNDEFINED";
        case VK_FORMAT_R4G4_UNORM_PACK8:            return "VK_FORMAT_R4G4_UNORM_PACK8";
        case VK_FORMAT_R4G4B4A4_UNORM_PACK16:       return "VK_FORMAT_R4G4B4A4_UNORM_PACK16";
        case VK_FORMAT_B4G4R4A4_UNORM_PACK16:       return "VK_FORMAT_B4G4R4A4_UNORM_PACK16";
        case VK_FORMAT_R5G6B5_UNORM_PACK16:         return "VK_FORMAT_R5G6B5_UNORM_PACK16";
        case VK_FORMAT_B5G6R5_UNORM_PACK16:         return "VK_FORMAT_B5G6R5_UNORM_PACK16";
        case VK_FORMAT_R5G5B5A1_UNORM_PACK16:       return "VK_FORMAT_R5G5B5A1_UNORM_PACK16";
        case VK_FORMAT_B5G5R5A1_UNORM_PACK16:       return "VK_FORMAT_B5G5R5A1_UNORM_PACK16";
        case VK_FORMAT_A1R5G5B5_UNORM_PACK16:       return "VK_FORMAT_A1R5G5B5_UNORM_PACK16";
        case VK_FORMAT_R8_UNORM:                    return "VK_FORMAT_R8_UNORM";
        case VK_FORMAT_R8_SNORM:                    return "VK_FORMAT_R8_SNORM";
        case VK_FORMAT_R8_USCALED:                  return "VK_FORMAT_R8_USCALED";
        case VK_FORMAT_R8_SSCALED:                  return "VK_FORMAT_R8_SSCALED";
        case VK_FORMAT_R8_UINT:                     return "VK_FORMAT_R8_UINT";
        case VK_FORMAT_R8_SINT:                     return "VK_FORMAT_R8_SINT";
        case VK_FORMAT_R8_SRGB:                     return "VK_FORMAT_R8_SRGB";
        case VK_FORMAT_R8G8_UNORM:                  return "VK_FORMAT_R8G8_UNORM";
        case VK_FORMAT_R8G8_SNORM:                  return "VK_FORMAT_R8G8_SNORM";
        case VK_FORMAT_R8G8_USCALED:                return "VK_FORMAT_R8G8_USCALED";
        case VK_FORMAT_R8G8_SSCALED:                return "VK_FORMAT_R8G8_SSCALED";
        case VK_FORMAT_R8G8_UINT:                   return "VK_FORMAT_R8G8_UINT";
        case VK_FORMAT_R8G8_SINT:                   return "VK_FORMAT_R8G8_SINT";
        case VK_FORMAT_R8G8_SRGB:                   return "VK_FORMAT_R8G8_SRGB";
        case VK_FORMAT_R8G8B8_UNORM:                return "VK_FORMAT_R8G8B8_UNORM";
        case VK_FORMAT_R8G8B8_SNORM:                return "VK_FORMAT_R8G8B8_SNORM";
        case VK_FORMAT_R8G8B8_USCALED:              return "VK_FORMAT_R8G8B8_USCALED";
        case VK_FORMAT_R8G8B8_SSCALED:              return "VK_FORMAT_R8G8B8_SSCALED";
        case VK_FORMAT_R8G8B8_UINT:                 return "VK_FORMAT_R8G8B8_UINT";
        case VK_FORMAT_R8G8B8_SINT:                 return "VK_FORMAT_R8G8B8_SINT";
        case VK_FORMAT_R8G8B8_SRGB:                 return "VK_FORMAT_R8G8B8_SRGB";
        case VK_FORMAT_B8G8R8_UNORM:                return "VK_FORMAT_B8G8R8_UNORM";
        case VK_FORMAT_B8G8R8_SNORM:                return "VK_FORMAT_B8G8R8_SNORM";
        case VK_FORMAT_B8G8R8_USCALED:              return "VK_FORMAT_B8G8R8_USCALED";
        case VK_FORMAT_B8G8R8_SSCALED:              return "VK_FORMAT_B8G8R8_SSCALED";
        case VK_FORMAT_B8G8R8_UINT:                 return "VK_FORMAT_B8G8R8_UINT";
        case VK_FORMAT_B8G8R8_SINT:                 return "VK_FORMAT_B8G8R8_SINT";
        case VK_FORMAT_B8G8R8_SRGB:                 return "VK_FORMAT_B8G8R8_SRGB";
        case VK_FORMAT_R8G8B8A8_UNORM:              return "VK_FORMAT_R8G8B8A8_UNORM";
        case VK_FORMAT_R8G8B8A8_SNORM:              return "VK_FORMAT_R8G8B8A8_SNORM";
        case VK_FORMAT_R8G8B8A8_USCALED:            return "VK_FORMAT_R8G8B8A8_USCALED";
        case VK_FORMAT_R8G8B8A8_SSCALED:            return "VK_FORMAT_R8G8B8A8_SSCALED";
        case VK_FORMAT_R8G8B8A8_UINT:               return "VK_FORMAT_R8G8B8A8_UINT";
        case VK_FORMAT_R8G8B8A8_SINT:               return "VK_FORMAT_R8G8B8A8_SINT";
        case VK_FORMAT_R8G8B8A8_SRGB:               return "VK_FORMAT_R8G8B8A8_SRGB";
        case VK_FORMAT_B8G8R8A8_UNORM:              return "VK_FORMAT_B8G8R8A8_UNORM";
        case VK_FORMAT_B8G8R8A8_SNORM:              return "VK_FORMAT_B8G8R8A8_SNORM";
        case VK_FORMAT_B8G8R8A8_USCALED:            return "VK_FORMAT_B8G8R8A8_USCALED";
        case VK_FORMAT_B8G8R8A8_SSCALED:            return "VK_FORMAT_B8G8R8A8_SSCALED";
        case VK_FORMAT_B8G8R8A8_UINT:               return "VK_FORMAT_B8G8R8A8_UINT";
        case VK_FORMAT_B8G8R8A8_SINT:               return "VK_FORMAT_B8G8R8A8_SINT";
        case VK_FORMAT_B8G8R8A8_SRGB:               return "VK_FORMAT_B8G8R8A8_SRGB";
        case VK_FORMAT_A8B8G8R8_UNORM_PACK32:       return "VK_FORMAT_A8B8G8R8_UNORM_PACK32";
        case VK_FORMAT_A8B8G8R8_SNORM_PACK32:       return "VK_FORMAT_A8B8G8R8_SNORM_PACK32";
        case VK_FORMAT_A8B8G8R8_USCALED_PACK32:     return "VK_FORMAT_A8B8G8R8_USCALED_PACK32";
        case VK_FORMAT_A8B8G8R8_SSCALED_PACK32:     return "VK_FORMAT_A8B8G8R8_SSCALED_PACK32";
        case VK_FORMAT_A8B8G8R8_UINT_PACK32:        return "VK_FORMAT_A8B8G8R8_UINT_PACK32";
        case VK_FORMAT_A8B8G8R8_SINT_PACK32:        return "VK_FORMAT_A8B8G8R8_SINT_PACK32";
        case VK_FORMAT_A8B8G8R8_SRGB_PACK32:        return "VK_FORMAT_A8B8G8R8_SRGB_PACK32";
        case VK_FORMAT_A2R10G10B10_UNORM_PACK32:    return "VK_FORMAT_A2R10G10B10_UNORM_PACK32";
        case VK_FORMAT_A2R10G10B10_SNORM_PACK32:    return "VK_FORMAT_A2R10G10B10_SNORM_PACK32";
        case VK_FORMAT_A2R10G10B10_USCALED_PACK32:  return "VK_FORMAT_A2R10G10B10_USCALED_PACK32";
        case VK_FORMAT_A2R10G10B10_SSCALED_PACK32:  return "VK_FORMAT_A2R10G10B10_SSCALED_PACK32";
        case VK_FORMAT_A2R10G10B10_UINT_PACK32:     return "VK_FORMAT_A2R10G10B10_UINT_PACK32";
        case VK_FORMAT_A2R10G10B10_SINT_PACK32:     return "VK_FORMAT_A2R10G10B10_SINT_PACK32";
        case VK_FORMAT_A2B10G10R10_UNORM_PACK32:    return "VK_FORMAT_A2B10G10R10_UNORM_PACK32";
        case VK_FORMAT_A2B10G10R10_SNORM_PACK32:    return "VK_FORMAT_A2B10G10R10_SNORM_PACK32";
        case VK_FORMAT_A2B10G10R10_USCALED_PACK32:  return "VK_FORMAT_A2B10G10R10_USCALED_PACK32";
        case VK_FORMAT_A2B10G10R10_SSCALED_PACK32:  return "VK_FORMAT_A2B10G10R10_SSCALED_PACK32";
        case VK_FORMAT_A2B10G10R10_UINT_PACK32:     return "VK_FORMAT_A2B10G10R10_UINT_PACK32";
        case VK_FORMAT_A2B10G10R10_SINT_PACK32:     return "VK_FORMAT_A2B10G10R10_SINT_PACK32";
        case VK_FORMAT_R16_UNORM:                   return "VK_FORMAT_R16_UNORM";
        case VK_FORMAT_R16_SNORM:                   return "VK_FORMAT_R16_SNORM";
        case VK_FORMAT_R16_USCALED:                 return "VK_FORMAT_R16_USCALED";
        case VK_FORMAT_R16_SSCALED:                 return "VK_FORMAT_R16_SSCALED";
        case VK_FORMAT_R16_UINT:                    return "VK_FORMAT_R16_UINT";
        case VK_FORMAT_R16_SINT:                    return "VK_FORMAT_R16_SINT";
        case VK_FORMAT_R16_SFLOAT:                  return "VK_FORMAT_R16_SFLOAT";
        case VK_FORMAT_R16G16_UNORM:                return "VK_FORMAT_R16G16_UNORM";
        case VK_FORMAT_R16G16_SNORM:                return "VK_FORMAT_R16G16_SNORM";
        case VK_FORMAT_R16G16_USCALED:              return "VK_FORMAT_R16G16_USCALED";
        case VK_FORMAT_R16G16_SSCALED:              return "VK_FORMAT_R16G16_SSCALED";
        case VK_FORMAT_R16G16_UINT:                 return "VK_FORMAT_R16G16_UINT";
        case VK_FORMAT_R16G16_SINT:                 return "VK_FORMAT_R16G16_SINT";
        case VK_FORMAT_R16G16_SFLOAT:               return "VK_FORMAT_R16G16_SFLOAT";
        case VK_FORMAT_R16G16B16_UNORM:             return "VK_FORMAT_R16G16B16_UNORM";
        case VK_FORMAT_R16G16B16_SNORM:             return "VK_FORMAT_R16G16B16_SNORM";
        case VK_FORMAT_R16G16B16_USCALED:           return "VK_FORMAT_R16G16B16_USCALED";
        case VK_FORMAT_R16G16B16_SSCALED:           return "VK_FORMAT_R16G16B16_SSCALED";
        case VK_FORMAT_R16G16B16_UINT:              return "VK_FORMAT_R16G16B16_UINT";
        case VK_FORMAT_R16G16B16_SINT:              return "VK_FORMAT_R16G16B16_SINT";
        case VK_FORMAT_R16G16B16_SFLOAT:            return "VK_FORMAT_R16G16B16_SFLOAT";
        case VK_FORMAT_R16G16B16A16_UNORM:          return "VK_FORMAT_R16G16B16A16_UNORM";
        case VK_FORMAT_R16G16B16A16_SNORM:          return "VK_FORMAT_R16G16B16A16_SNORM";
        case VK_FORMAT_R16G16B16A16_USCALED:        return "VK_FORMAT_R16G16B16A16_USCALED";
        case VK_FORMAT_R16G16B16A16_SSCALED:        return "VK_FORMAT_R16G16B16A16_SSCALED";
        case VK_FORMAT_R16G16B16A16_UINT:           return "VK_FORMAT_R16G16B16A16_UINT";
        case VK_FORMAT_R16G16B16A16_SINT:           return "VK_FORMAT_R16G16B16A16_SINT";
        case VK_FORMAT_R16G16B16A16_SFLOAT:         return "VK_FORMAT_R16G16B16A16_SFLOAT";
        case VK_FORMAT_R32_UINT:                    return "VK_FORMAT_R32_UINT";
        case VK_FORMAT_R32_SINT:                    return "VK_FORMAT_R32_SINT";
        case VK_FORMAT_R32_SFLOAT:                  return "VK_FORMAT_R32_SFLOAT";
        case VK_FORMAT_R32G32_UINT:                 return "VK_FORMAT_R32G32_UINT";
        case VK_FORMAT_R32G32_SINT:                 return "VK_FORMAT_R32G32_SINT";
        case VK_FORMAT_R32G32_SFLOAT:               return "VK_FORMAT_R32G32_SFLOAT";
        case VK_FORMAT_R32G32B32_UINT:              return "VK_FORMAT_R32G32B32_UINT";
        case VK_FORMAT_R32G32B32_SINT:              return "VK_FORMAT_R32G32B32_SINT";
        case VK_FORMAT_R32G32B32_SFLOAT:            return "VK_FORMAT_R32G32B32_SFLOAT";
        case VK_FORMAT_R32G32B32A32_UINT:           return "VK_FORMAT_R32G32B32A32_UINT";
        case VK_FORMAT_R32G32B32A32_SINT:           return "VK_FORMAT_R32G32B32A32_SINT";
        case VK_FORMAT_R32G32B32A32_SFLOAT:         return "VK_FORMAT_R32G32B32A32_SFLOAT";
        case VK_FORMAT_R64_UINT:                    return "VK_FORMAT_R64_UINT";
        case VK_FORMAT_R64_SINT:                    return "VK_FORMAT_R64_SINT";
        case VK_FORMAT_R64_SFLOAT:                  return "VK_FORMAT_R64_SFLOAT";
        case VK_FORMAT_R64G64_UINT:                 return "VK_FORMAT_R64G64_UINT";
        case VK_FORMAT_R64G64_SINT:                 return "VK_FORMAT_R64G64_SINT";
        case VK_FORMAT_R64G64_SFLOAT:               return "VK_FORMAT_R64G64_SFLOAT";
        case VK_FORMAT_R64G64B64_UINT:              return "VK_FORMAT_R64G64B64_UINT";
        case VK_FORMAT_R64G64B64_SINT:              return "VK_FORMAT_R64G64B64_SINT";
        case VK_FORMAT_R64G64B64_SFLOAT:            return "VK_FORMAT_R64G64B64_SFLOAT";
        case VK_FORMAT_R64G64B64A64_UINT:           return "VK_FORMAT_R64G64B64A64_UINT";
        case VK_FORMAT_R64G64B64A64_SINT:           return "VK_FORMAT_R64G64B64A64_SINT";
        case VK_FORMAT_R64G64B64A64_SFLOAT:         return "VK_FORMAT_R64G64B64A64_SFLOAT";
        case VK_FORMAT_B10G11R11_UFLOAT_PACK32:     return "VK_FORMAT_B10G11R11_UFLOAT_PACK32";
        case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32:      return "VK_FORMAT_E5B9G9R9_UFLOAT_PACK32";
        case VK_FORMAT_D16_UNORM:                   return "VK_FORMAT_D16_UNORM";
        case VK_FORMAT_X8_D24_UNORM_PACK32:         return "VK_FORMAT_X8_D24_UNORM_PACK32";
        case VK_FORMAT_D32_SFLOAT:                  return "VK_FORMAT_D32_SFLOAT";
        case VK_FORMAT_S8_UINT:                     return "VK_FORMAT_S8_UINT";
        case VK_FORMAT_D16_UNORM_S8_UINT:           return "VK_FORMAT_D16_UNORM_S8_UINT";
        case VK_FORMAT_D24_UNORM_S8_UINT:           return "VK_FORMAT_D24_UNORM_S8_UINT";
        case VK_FORMAT_D32_SFLOAT_S8_UINT:          return "VK_FORMAT_D32_SFLOAT_S8_UINT";
        case VK_FORMAT_BC1_RGB_UNORM_BLOCK:         return "VK_FORMAT_BC1_RGB_UNORM_BLOCK";
        case VK_FORMAT_BC1_RGB_SRGB_BLOCK:          return "VK_FORMAT_BC1_RGB_SRGB_BLOCK";
        case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:        return "VK_FORMAT_BC1_RGBA_UNORM_BLOCK";
        case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:         return "VK_FORMAT_BC1_RGBA_SRGB_BLOCK";
        case VK_FORMAT_BC2_UNORM_BLOCK:             return "VK_FORMAT_BC2_UNORM_BLOCK";
        case VK_FORMAT_BC2_SRGB_BLOCK:              return "VK_FORMAT_BC2_SRGB_BLOCK";
        case VK_FORMAT_BC3_UNORM_BLOCK:             return "VK_FORMAT_BC3_UNORM_BLOCK";
        case VK_FORMAT_BC3_SRGB_BLOCK:              return "VK_FORMAT_BC3_SRGB_BLOCK";
        case VK_FORMAT_BC4_UNORM_BLOCK:             return "VK_FORMAT_BC4_UNORM_BLOCK";
        case VK_FORMAT_BC4_SNORM_BLOCK:             return "VK_FORMAT_BC4_SNORM_BLOCK";
        case VK_FORMAT_BC5_UNORM_BLOCK:             return "VK_FORMAT_BC5_UNORM_BLOCK";
        case VK_FORMAT_BC5_SNORM_BLOCK:             return "VK_FORMAT_BC5_SNORM_BLOCK";
        case VK_FORMAT_BC6H_UFLOAT_BLOCK:           return "VK_FORMAT_BC6H_UFLOAT_BLOCK";
        case VK_FORMAT_BC6H_SFLOAT_BLOCK:           return "VK_FORMAT_BC6H_SFLOAT_BLOCK";
        case VK_FORMAT_BC7_UNORM_BLOCK:             return "VK_FORMAT_BC7_UNORM_BLOCK";
        case VK_FORMAT_BC7_SRGB_BLOCK:              return "VK_FORMAT_BC7_SRGB_BLOCK";
        case VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK:     return "VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK";
        case VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK:      return "VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK";
        case VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK:   return "VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK";
        case VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK:    return "VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK";
        case VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK:   return "VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK";
        case VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK:    return "VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK";
        case VK_FORMAT_EAC_R11_UNORM_BLOCK:         return "VK_FORMAT_EAC_R11_UNORM_BLOCK";
        case VK_FORMAT_EAC_R11_SNORM_BLOCK:         return "VK_FORMAT_EAC_R11_SNORM_BLOCK";
        case VK_FORMAT_EAC_R11G11_UNORM_BLOCK:      return "VK_FORMAT_EAC_R11G11_UNORM_BLOCK";
        case VK_FORMAT_EAC_R11G11_SNORM_BLOCK:      return "VK_FORMAT_EAC_R11G11_SNORM_BLOCK";
        case VK_FORMAT_ASTC_4x4_UNORM_BLOCK:        return "VK_FORMAT_ASTC_4x4_UNORM_BLOCK";
        case VK_FORMAT_ASTC_4x4_SRGB_BLOCK:         return "VK_FORMAT_ASTC_4x4_SRGB_BLOCK";
        case VK_FORMAT_ASTC_5x4_UNORM_BLOCK:        return "VK_FORMAT_ASTC_5x4_UNORM_BLOCK";
        case VK_FORMAT_ASTC_5x4_SRGB_BLOCK:         return "VK_FORMAT_ASTC_5x4_SRGB_BLOCK";
        case VK_FORMAT_ASTC_5x5_UNORM_BLOCK:        return "VK_FORMAT_ASTC_5x5_UNORM_BLOCK";
        case VK_FORMAT_ASTC_5x5_SRGB_BLOCK:         return "VK_FORMAT_ASTC_5x5_SRGB_BLOCK";
        case VK_FORMAT_ASTC_6x5_UNORM_BLOCK:        return "VK_FORMAT_ASTC_6x5_UNORM_BLOCK";
        case VK_FORMAT_ASTC_6x5_SRGB_BLOCK:         return "VK_FORMAT_ASTC_6x5_SRGB_BLOCK";
        case VK_FORMAT_ASTC_6x6_UNORM_BLOCK:        return "VK_FORMAT_ASTC_6x6_UNORM_BLOCK";
        case VK_FORMAT_ASTC_6x6_SRGB_BLOCK:         return "VK_FORMAT_ASTC_6x6_SRGB_BLOCK";
        case VK_FORMAT_ASTC_8x5_UNORM_BLOCK:        return "VK_FORMAT_ASTC_8x5_UNORM_BLOCK";
        case VK_FORMAT_ASTC_8x5_SRGB_BLOCK:         return "VK_FORMAT_ASTC_8x5_SRGB_BLOCK";
        case VK_FORMAT_ASTC_8x6_UNORM_BLOCK:        return "VK_FORMAT_ASTC_8x6_UNORM_BLOCK";
        case VK_FORMAT_ASTC_8x6_SRGB_BLOCK:         return "VK_FORMAT_ASTC_8x6_SRGB_BLOCK";
        case VK_FORMAT_ASTC_8x8_UNORM_BLOCK:        return "VK_FORMAT_ASTC_8x8_UNORM_BLOCK";
        case VK_FORMAT_ASTC_8x8_SRGB_BLOCK:         return "VK_FORMAT_ASTC_8x8_SRGB_BLOCK";
        case VK_FORMAT_ASTC_10x5_UNORM_BLOCK:       return "VK_FORMAT_ASTC_10x5_UNORM_BLOCK";
        case VK_FORMAT_ASTC_10x5_SRGB_BLOCK:        return "VK_FORMAT_ASTC_10x5_SRGB_BLOCK";
        case VK_FORMAT_ASTC_10x6_UNORM_BLOCK:       return "VK_FORMAT_ASTC_10x6_UNORM_BLOCK";
        case VK_FORMAT_ASTC_10x6_SRGB_BLOCK:        return "VK_FORMAT_ASTC_10x6_SRGB_BLOCK";
        case VK_FORMAT_ASTC_10x8_UNORM_BLOCK:       return "VK_FORMAT_ASTC_10x8_UNORM_BLOCK";
        case VK_FORMAT_ASTC_10x8_SRGB_BLOCK:        return "VK_FORMAT_ASTC_10x8_SRGB_BLOCK";
        case VK_FORMAT_ASTC_10x10_UNORM_BLOCK:      return "VK_FORMAT_ASTC_10x10_UNORM_BLOCK";
        case VK_FORMAT_ASTC_10x10_SRGB_BLOCK:       return "VK_FORMAT_ASTC_10x10_SRGB_BLOCK";
        case VK_FORMAT_ASTC_12x10_UNORM_BLOCK:      return "VK_FORMAT_ASTC_12x10_UNORM_BLOCK";
        case VK_FORMAT_ASTC_12x10_SRGB_BLOCK:       return "VK_FORMAT_ASTC_12x10_SRGB_BLOCK";
        case VK_FORMAT_ASTC_12x12_UNORM_BLOCK:      return "VK_FORMAT_ASTC_12x12_UNORM_BLOCK";
        case VK_FORMAT_ASTC_12x12_SRGB_BLOCK:       return "VK_FORMAT_ASTC_12x12_SRGB_BLOCK";
        case VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG: return "VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG";
        case VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG: return "VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG";
        case VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG: return "VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG";
        case VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG: return "VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG";
        case VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG:  return "VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG";
        case VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG:  return "VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG";
        case VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG:  return "VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG";
        case VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG:  return "VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG";
        default:                                    return "unknown";
    }
}

const char* vulkan::getVkColorSpaceString(VkColorSpaceKHR colorSpace)
{
    switch (colorSpace)
    {
        case VK_COLOR_SPACE_SRGB_NONLINEAR_KHR:       return "VK_COLOR_SPACE_SRGB_NONLINEAR_KHR";
        case VK_COLOR_SPACE_DISPLAY_P3_NONLINEAR_EXT: return "VK_COLOR_SPACE_DISPLAY_P3_NONLINEAR_EXT";
        case VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT: return "VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT";
        case VK_COLOR_SPACE_DCI_P3_LINEAR_EXT:        return "VK_COLOR_SPACE_DCI_P3_LINEAR_EXT";
        case VK_COLOR_SPACE_DCI_P3_NONLINEAR_EXT:     return "VK_COLOR_SPACE_DCI_P3_NONLINEAR_EXT";
        case VK_COLOR_SPACE_BT709_LINEAR_EXT:         return "VK_COLOR_SPACE_BT709_LINEAR_EXT";
        case VK_COLOR_SPACE_BT709_NONLINEAR_EXT:      return "VK_COLOR_SPACE_BT709_NONLINEAR_EXT";
        case VK_COLOR_SPACE_BT2020_LINEAR_EXT:        return "VK_COLOR_SPACE_BT2020_LINEAR_EXT";
        case VK_COLOR_SPACE_HDR10_ST2084_EXT:         return "VK_COLOR_SPACE_HDR10_ST2084_EXT";
        case VK_COLOR_SPACE_DOLBYVISION_EXT:          return "VK_COLOR_SPACE_DOLBYVISION_EXT";
        case VK_COLOR_SPACE_HDR10_HLG_EXT:            return "VK_COLOR_SPACE_HDR10_HLG_EXT";
        case VK_COLOR_SPACE_ADOBERGB_LINEAR_EXT:      return "VK_COLOR_SPACE_ADOBERGB_LINEAR_EXT";
        case VK_COLOR_SPACE_ADOBERGB_NONLINEAR_EXT:   return "VK_COLOR_SPACE_ADOBERGB_NONLINEAR_EXT";
        default:                                      return "unknown";
    }
}

const char* vulkan::getVkPresentModeString(VkPresentModeKHR presentMode)
{
    switch (presentMode)
    {
        case VK_PRESENT_MODE_IMMEDIATE_KHR:    return "VK_PRESENT_MODE_IMMEDIATE_KHR";
        case VK_PRESENT_MODE_MAILBOX_KHR:      return "VK_PRESENT_MODE_MAILBOX_KHR";
        case VK_PRESENT_MODE_FIFO_KHR:         return "VK_PRESENT_MODE_FIFO_KHR";
        case VK_PRESENT_MODE_FIFO_RELAXED_KHR: return "VK_PRESENT_MODE_FIFO_RELAXED_KHR";
        default:                               return "unknown";
    }
}

VkFormat vulkan::getVkVertexAttributeFormat(VertexAttributeFormat format)
{
    switch (format)
    {
        case VertexAttributeFormat::X32Y32_FLOAT:       return VK_FORMAT_R32G32_SFLOAT;
        case VertexAttributeFormat::X32Y32Z32_FLOAT:    return VK_FORMAT_R32G32B32_SFLOAT;
        case VertexAttributeFormat::X32Y32Z32W32_FLOAT: return VK_FORMAT_R32G32B32A32_SFLOAT;
        default:                                        break;
    }

    return VK_FORMAT_UNDEFINED;
}

DynamicArray<VkVertexInputBindingDescription> vulkan::getVertexBindingDescription()
{
    DynamicArray<VkVertexInputBindingDescription> bindingDescriptions(1);
    bindingDescriptions[0].binding   = 0;
    bindingDescriptions[0].stride    = sizeof(Vertex);
    bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return bindingDescriptions;
}

DynamicArray<VkVertexInputAttributeDescription> vulkan::getVertexAtrributeDescription()
{
    DynamicArray<VkVertexInputAttributeDescription> attributeDescriptions {};

    // get vertex struct info
    auto attribuitesInfo = Vertex::getVertexAttributesDescription();
    for (uint32_t attribIndex = 0u; attribIndex < attribuitesInfo.size(); ++attribIndex)
    {
        auto& attribute = attribuitesInfo[attribIndex];
        attributeDescriptions.push_back(
            { attribute.location, 0, getVkVertexAttributeFormat(attribute.format), attribute.offset });
    }

    return attributeDescriptions;
}

VkBufferUsageFlags vulkan::getBufferUsageFlagBits(uint32_t usage)
{
    VkBufferUsageFlags flags = 0u;
    if (usage & GfxBufferUsageFlags::TransferSource) flags |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    if (usage & GfxBufferUsageFlags::TransferTarget) flags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    if (usage & GfxBufferUsageFlags::UniformBuffer) flags |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    if (usage & GfxBufferUsageFlags::VertexBuffer) flags |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    if (usage & GfxBufferUsageFlags::IndexBuffer) flags |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    if (usage & GfxBufferUsageFlags::StorageBuffer) flags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

    return flags;
}

VkImageUsageFlags vulkan::getTextureUsageFlagBits(uint32_t usage)
{
    VkImageUsageFlags flags = 0u;
    if (usage & TextureUsageFlags::TransferSrcTexture) flags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    if (usage & TextureUsageFlags::TransferDstTexture) flags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    if (usage & TextureUsageFlags::SampledTexture) flags |= VK_IMAGE_USAGE_SAMPLED_BIT;
    if (usage & TextureUsageFlags::StorageTexture) flags |= VK_IMAGE_USAGE_STORAGE_BIT;
    if (usage & TextureUsageFlags::ColorTexture) flags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    if (usage & TextureUsageFlags::DepthStencilTexture) flags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

    return flags;
}

VkImageType vulkan::getImageType(TextureType type)
{
    switch (type)
    {
        case TextureType::Texture1D:      return VK_IMAGE_TYPE_1D;
        case TextureType::Texture2D:      return VK_IMAGE_TYPE_2D;
        case TextureType::Texture2DArray: return VK_IMAGE_TYPE_2D;
        case TextureType::Texture3D:      return VK_IMAGE_TYPE_3D;
        case TextureType::Cube:           return VK_IMAGE_TYPE_2D;
        case TextureType::CubeArray:      return VK_IMAGE_TYPE_2D;
    }

    return VK_IMAGE_TYPE_MAX_ENUM;
}

VkImageViewType vulkan::getImageViewType(TextureType type)
{
    switch (type)
    {
        case TextureType::Texture1D:      return VK_IMAGE_VIEW_TYPE_1D;
        case TextureType::Texture2D:      return VK_IMAGE_VIEW_TYPE_2D;
        case TextureType::Texture2DArray: return VK_IMAGE_VIEW_TYPE_2D_ARRAY;
        case TextureType::Texture3D:      return VK_IMAGE_VIEW_TYPE_3D;
        case TextureType::Cube:           return VK_IMAGE_VIEW_TYPE_CUBE;
        case TextureType::CubeArray:      return VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
    }

    return VK_IMAGE_VIEW_TYPE_MAX_ENUM;
}

uint32_t vulkan::getVmaAllocationCreateFlagBits(uint32_t flags)
{
    uint32_t creationFlags = 0u;

    if (flags & GfxBufferMemoryTypeFlags::PersistentlyMapped)
        creationFlags |= VMA_ALLOCATION_CREATE_MAPPED_BIT;
    if (flags & GfxBufferMemoryTypeFlags::HostSequentialWrite)
        creationFlags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    if (flags & GfxBufferMemoryTypeFlags::HostRandomAccess)
        creationFlags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
    if (flags & GfxBufferMemoryTypeFlags::DedicatedDeviceMemory)
        creationFlags |= VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;

    return creationFlags;
}

VkAttachmentLoadOp vulkan::getLoadOp(GfxLoadOperation loadOp)
{
    switch (loadOp)
    {
        case GfxLoadOperation::DontCare: return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        case GfxLoadOperation::Load:     return VK_ATTACHMENT_LOAD_OP_LOAD;
        case GfxLoadOperation::Clear:    return VK_ATTACHMENT_LOAD_OP_CLEAR;
    }
    return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
}

VkAttachmentStoreOp vulkan::getStoreOp(GfxStoreOperation storeOp)
{
    switch (storeOp)
    {
        case GfxStoreOperation::DontCare: return VK_ATTACHMENT_STORE_OP_DONT_CARE;
        case GfxStoreOperation::Store:    return VK_ATTACHMENT_STORE_OP_STORE;
        case GfxStoreOperation::None:     return VK_ATTACHMENT_STORE_OP_NONE_KHR;
    }
    return VK_ATTACHMENT_STORE_OP_DONT_CARE;
}

VkFormat vulkan::getPixelVkFormat(PixelFormat format)
{
    switch (format)
    {
        // R8
        case PixelFormat::R8_unorm:             return VK_FORMAT_R8_UNORM;
        case PixelFormat::R8_snorm:             return VK_FORMAT_R8_SNORM;
        case PixelFormat::R8_uscaled:           return VK_FORMAT_R8_USCALED;
        case PixelFormat::R8_sscaled:           return VK_FORMAT_R8_SSCALED;
        case PixelFormat::R8_uint:              return VK_FORMAT_R8_UINT;
        case PixelFormat::R8_sint:              return VK_FORMAT_R8_SINT;
        case PixelFormat::R8_srgb:              return VK_FORMAT_R8_SRGB;

        // R8G8
        case PixelFormat::R8G8_unorm:           return VK_FORMAT_R8G8_UNORM;
        case PixelFormat::R8G8_snorm:           return VK_FORMAT_R8G8_SNORM;
        case PixelFormat::R8G8_uscaled:         return VK_FORMAT_R8G8_USCALED;
        case PixelFormat::R8G8_sscaled:         return VK_FORMAT_R8G8_SSCALED;
        case PixelFormat::R8G8_uint:            return VK_FORMAT_R8G8_UINT;
        case PixelFormat::R8G8_sint:            return VK_FORMAT_R8G8_SINT;
        case PixelFormat::R8G8_srgb:            return VK_FORMAT_R8G8_SRGB;

        // R8G8B8
        case PixelFormat::R8G8B8_unorm:         return VK_FORMAT_R8G8B8_UNORM;
        case PixelFormat::R8G8B8_snorm:         return VK_FORMAT_R8G8B8_SNORM;
        case PixelFormat::R8G8B8_uscaled:       return VK_FORMAT_R8G8B8_USCALED;
        case PixelFormat::R8G8B8_sscaled:       return VK_FORMAT_R8G8B8_SSCALED;
        case PixelFormat::R8G8B8_uint:          return VK_FORMAT_R8G8B8_UINT;
        case PixelFormat::R8G8B8_sint:          return VK_FORMAT_R8G8B8_SINT;
        case PixelFormat::R8G8B8_srgb:          return VK_FORMAT_R8G8B8_SRGB;

        // R8G8B8A8
        case PixelFormat::R8G8B8A8_unorm:       return VK_FORMAT_R8G8B8A8_UNORM;
        case PixelFormat::R8G8B8A8_snorm:       return VK_FORMAT_R8G8B8A8_SNORM;
        case PixelFormat::R8G8B8A8_uscaled:     return VK_FORMAT_R8G8B8A8_USCALED;
        case PixelFormat::R8G8B8A8_sscaled:     return VK_FORMAT_R8G8B8A8_SSCALED;
        case PixelFormat::R8G8B8A8_uint:        return VK_FORMAT_R8G8B8A8_UINT;
        case PixelFormat::R8G8B8A8_sint:        return VK_FORMAT_R8G8B8A8_SINT;
        case PixelFormat::R8G8B8A8_srgb:        return VK_FORMAT_R8G8B8A8_SRGB;

        // R16
        case PixelFormat::R16_unorm:            return VK_FORMAT_R16_UNORM;
        case PixelFormat::R16_snorm:            return VK_FORMAT_R16_SNORM;
        case PixelFormat::R16_uscaled:          return VK_FORMAT_R16_USCALED;
        case PixelFormat::R16_sscaled:          return VK_FORMAT_R16_SSCALED;
        case PixelFormat::R16_uint:             return VK_FORMAT_R16_UINT;
        case PixelFormat::R16_sint:             return VK_FORMAT_R16_SINT;
        case PixelFormat::R16_sfloat:           return VK_FORMAT_R16_SFLOAT;

        // R16G16
        case PixelFormat::R16G16_unorm:         return VK_FORMAT_R16G16_UNORM;
        case PixelFormat::R16G16_snorm:         return VK_FORMAT_R16G16_SNORM;
        case PixelFormat::R16G16_uscaled:       return VK_FORMAT_R16G16_USCALED;
        case PixelFormat::R16G16_sscaled:       return VK_FORMAT_R16G16_SSCALED;
        case PixelFormat::R16G16_uint:          return VK_FORMAT_R16G16_UINT;
        case PixelFormat::R16G16_sint:          return VK_FORMAT_R16G16_SINT;
        case PixelFormat::R16G16_sfloat:        return VK_FORMAT_R16G16_SFLOAT;

        // R16G16B16
        case PixelFormat::R16G16B16_unorm:      return VK_FORMAT_R16G16B16_UNORM;
        case PixelFormat::R16G16B16_snorm:      return VK_FORMAT_R16G16B16_SNORM;
        case PixelFormat::R16G16B16_uscaled:    return VK_FORMAT_R16G16B16_USCALED;
        case PixelFormat::R16G16B16_sscaled:    return VK_FORMAT_R16G16B16_SSCALED;
        case PixelFormat::R16G16B16_uint:       return VK_FORMAT_R16G16B16_UINT;
        case PixelFormat::R16G16B16_sint:       return VK_FORMAT_R16G16B16_SINT;
        case PixelFormat::R16G16B16_sfloat:     return VK_FORMAT_R16G16B16_SFLOAT;

        // R16G16B16A16
        case PixelFormat::R16G16B16A16_unorm:   return VK_FORMAT_R16G16B16A16_UNORM;
        case PixelFormat::R16G16B16A16_snorm:   return VK_FORMAT_R16G16B16A16_SNORM;
        case PixelFormat::R16G16B16A16_uscaled: return VK_FORMAT_R16G16B16A16_USCALED;
        case PixelFormat::R16G16B16A16_sscaled: return VK_FORMAT_R16G16B16A16_SSCALED;
        case PixelFormat::R16G16B16A16_uint:    return VK_FORMAT_R16G16B16A16_UINT;
        case PixelFormat::R16G16B16A16_sint:    return VK_FORMAT_R16G16B16A16_SINT;
        case PixelFormat::R16G16B16A16_sfloat:  return VK_FORMAT_R16G16B16A16_SFLOAT;

        // R32
        case PixelFormat::R32_uint:             return VK_FORMAT_R32_UINT;
        case PixelFormat::R32_sint:             return VK_FORMAT_R32_SINT;
        case PixelFormat::R32_sfloat:           return VK_FORMAT_R32_SFLOAT;

        // R32G32
        case PixelFormat::R32G32_uint:          return VK_FORMAT_R32G32_UINT;
        case PixelFormat::R32G32_sint:          return VK_FORMAT_R32G32_SINT;
        case PixelFormat::R32G32_sfloat:        return VK_FORMAT_R32G32_SFLOAT;

        // R32G32B32
        case PixelFormat::R32G32B32_uint:       return VK_FORMAT_R32G32B32_UINT;
        case PixelFormat::R32G32B32_sint:       return VK_FORMAT_R32G32B32_SINT;
        case PixelFormat::R32G32B32_sfloat:     return VK_FORMAT_R32G32B32_SFLOAT;

        // R32G32B32A32
        case PixelFormat::R32G32B32A32_uint:    return VK_FORMAT_R32G32B32A32_UINT;
        case PixelFormat::R32G32B32A32_sint:    return VK_FORMAT_R32G32B32A32_SINT;
        case PixelFormat::R32G32B32A32_sfloat:  return VK_FORMAT_R32G32B32A32_SFLOAT;

        default:
            return VK_FORMAT_UNDEFINED;
    }
}

PixelFormat vulkan::getPixelFormat(VkFormat format)
{
    switch (format)
    {
        // R8
        case VK_FORMAT_R8_UNORM:             return PixelFormat::R8_unorm;
        case VK_FORMAT_R8_SNORM:             return PixelFormat::R8_snorm;
        case VK_FORMAT_R8_USCALED:           return PixelFormat::R8_uscaled;
        case VK_FORMAT_R8_SSCALED:           return PixelFormat::R8_sscaled;
        case VK_FORMAT_R8_UINT:              return PixelFormat::R8_uint;
        case VK_FORMAT_R8_SINT:              return PixelFormat::R8_sint;
        case VK_FORMAT_R8_SRGB:              return PixelFormat::R8_srgb;

        // R8G8
        case VK_FORMAT_R8G8_UNORM:           return PixelFormat::R8G8_unorm;
        case VK_FORMAT_R8G8_SNORM:           return PixelFormat::R8G8_snorm;
        case VK_FORMAT_R8G8_USCALED:         return PixelFormat::R8G8_uscaled;
        case VK_FORMAT_R8G8_SSCALED:         return PixelFormat::R8G8_sscaled;
        case VK_FORMAT_R8G8_UINT:            return PixelFormat::R8G8_uint;
        case VK_FORMAT_R8G8_SINT:            return PixelFormat::R8G8_sint;
        case VK_FORMAT_R8G8_SRGB:            return PixelFormat::R8G8_srgb;

        // R8G8B8
        case VK_FORMAT_R8G8B8_UNORM:         return PixelFormat::R8G8B8_unorm;
        case VK_FORMAT_R8G8B8_SNORM:         return PixelFormat::R8G8B8_snorm;
        case VK_FORMAT_R8G8B8_USCALED:       return PixelFormat::R8G8B8_uscaled;
        case VK_FORMAT_R8G8B8_SSCALED:       return PixelFormat::R8G8B8_sscaled;
        case VK_FORMAT_R8G8B8_UINT:          return PixelFormat::R8G8B8_uint;
        case VK_FORMAT_R8G8B8_SINT:          return PixelFormat::R8G8B8_sint;
        case VK_FORMAT_R8G8B8_SRGB:          return PixelFormat::R8G8B8_srgb;

        // R8G8B8A8
        case VK_FORMAT_R8G8B8A8_UNORM:       return PixelFormat::R8G8B8A8_unorm;
        case VK_FORMAT_R8G8B8A8_SNORM:       return PixelFormat::R8G8B8A8_snorm;
        case VK_FORMAT_R8G8B8A8_USCALED:     return PixelFormat::R8G8B8A8_uscaled;
        case VK_FORMAT_R8G8B8A8_SSCALED:     return PixelFormat::R8G8B8A8_sscaled;
        case VK_FORMAT_R8G8B8A8_UINT:        return PixelFormat::R8G8B8A8_uint;
        case VK_FORMAT_R8G8B8A8_SINT:        return PixelFormat::R8G8B8A8_sint;
        case VK_FORMAT_R8G8B8A8_SRGB:        return PixelFormat::R8G8B8A8_srgb;

        // R16
        case VK_FORMAT_R16_UNORM:            return PixelFormat::R16_unorm;
        case VK_FORMAT_R16_SNORM:            return PixelFormat::R16_snorm;
        case VK_FORMAT_R16_USCALED:          return PixelFormat::R16_uscaled;
        case VK_FORMAT_R16_SSCALED:          return PixelFormat::R16_sscaled;
        case VK_FORMAT_R16_UINT:             return PixelFormat::R16_uint;
        case VK_FORMAT_R16_SINT:             return PixelFormat::R16_sint;
        case VK_FORMAT_R16_SFLOAT:           return PixelFormat::R16_sfloat;

        // R16G16
        case VK_FORMAT_R16G16_UNORM:         return PixelFormat::R16G16_unorm;
        case VK_FORMAT_R16G16_SNORM:         return PixelFormat::R16G16_snorm;
        case VK_FORMAT_R16G16_USCALED:       return PixelFormat::R16G16_uscaled;
        case VK_FORMAT_R16G16_SSCALED:       return PixelFormat::R16G16_sscaled;
        case VK_FORMAT_R16G16_UINT:          return PixelFormat::R16G16_uint;
        case VK_FORMAT_R16G16_SINT:          return PixelFormat::R16G16_sint;
        case VK_FORMAT_R16G16_SFLOAT:        return PixelFormat::R16G16_sfloat;

        // R16G16B16
        case VK_FORMAT_R16G16B16_UNORM:      return PixelFormat::R16G16B16_unorm;
        case VK_FORMAT_R16G16B16_SNORM:      return PixelFormat::R16G16B16_snorm;
        case VK_FORMAT_R16G16B16_USCALED:    return PixelFormat::R16G16B16_uscaled;
        case VK_FORMAT_R16G16B16_SSCALED:    return PixelFormat::R16G16B16_sscaled;
        case VK_FORMAT_R16G16B16_UINT:       return PixelFormat::R16G16B16_uint;
        case VK_FORMAT_R16G16B16_SINT:       return PixelFormat::R16G16B16_sint;
        case VK_FORMAT_R16G16B16_SFLOAT:     return PixelFormat::R16G16B16_sfloat;

        // R16G16B16A16
        case VK_FORMAT_R16G16B16A16_UNORM:   return PixelFormat::R16G16B16A16_unorm;
        case VK_FORMAT_R16G16B16A16_SNORM:   return PixelFormat::R16G16B16A16_snorm;
        case VK_FORMAT_R16G16B16A16_USCALED: return PixelFormat::R16G16B16A16_uscaled;
        case VK_FORMAT_R16G16B16A16_SSCALED: return PixelFormat::R16G16B16A16_sscaled;
        case VK_FORMAT_R16G16B16A16_UINT:    return PixelFormat::R16G16B16A16_uint;
        case VK_FORMAT_R16G16B16A16_SINT:    return PixelFormat::R16G16B16A16_sint;
        case VK_FORMAT_R16G16B16A16_SFLOAT:  return PixelFormat::R16G16B16A16_sfloat;

        // R32
        case VK_FORMAT_R32_UINT:             return PixelFormat::R32_uint;
        case VK_FORMAT_R32_SINT:             return PixelFormat::R32_sint;
        case VK_FORMAT_R32_SFLOAT:           return PixelFormat::R32_sfloat;

        // R32G32
        case VK_FORMAT_R32G32_UINT:          return PixelFormat::R32G32_uint;
        case VK_FORMAT_R32G32_SINT:          return PixelFormat::R32G32_sint;
        case VK_FORMAT_R32G32_SFLOAT:        return PixelFormat::R32G32_sfloat;

        // R32G32B32
        case VK_FORMAT_R32G32B32_UINT:       return PixelFormat::R32G32B32_uint;
        case VK_FORMAT_R32G32B32_SINT:       return PixelFormat::R32G32B32_sint;
        case VK_FORMAT_R32G32B32_SFLOAT:     return PixelFormat::R32G32B32_sfloat;

        // R32G32B32A32
        case VK_FORMAT_R32G32B32A32_UINT:    return PixelFormat::R32G32B32A32_uint;
        case VK_FORMAT_R32G32B32A32_SINT:    return PixelFormat::R32G32B32A32_sint;
        case VK_FORMAT_R32G32B32A32_SFLOAT:  return PixelFormat::R32G32B32A32_sfloat;

        default:
            return PixelFormat::None;
    }
}

size_t vulkan::getBytesPerPixel(VkFormat format)
{
    switch (format)
    {
        case VK_FORMAT_R8_UNORM:
        case VK_FORMAT_R8_SRGB:
            return 1;

        case VK_FORMAT_R8G8_UNORM:
        case VK_FORMAT_R8G8_SRGB:
        case VK_FORMAT_R16_SFLOAT:
            return 2;

        case VK_FORMAT_R8G8B8_UNORM:
        case VK_FORMAT_R8G8B8_SRGB:
            return 3;

        case VK_FORMAT_R8G8B8A8_UNORM:
        case VK_FORMAT_R8G8B8A8_SRGB:
        case VK_FORMAT_B8G8R8A8_UNORM:
        case VK_FORMAT_B8G8R8A8_SRGB:
        case VK_FORMAT_R16G16_SFLOAT:
        case VK_FORMAT_R32_SFLOAT:
            return 4;

        case VK_FORMAT_R16G16B16A16_SFLOAT:
        case VK_FORMAT_R32G32_SFLOAT:
            return 8;

        case VK_FORMAT_R32G32B32A32_SFLOAT:
            return 16;

        // Add more formats as needed
        default:
            return 4; // Default to 4 bytes
    }
}

} // namespace dusk