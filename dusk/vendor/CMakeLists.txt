  cmake_minimum_required(VERSION 3.29)

project(vendor LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)

# spdlog
add_subdirectory(spdlog)

# tracy
set(TRACY_ENABLE ON)
set(TRACY_TIMER_FALLBACK ON)
add_subdirectory(tracy)

# glfw
option(GLFW_BUILD_DOCS OFF)
option(GLFW_BUILD_TESTS OFF)
option(GLFW_BUILD_EXAMPLES OFF)
option(GLFW_BUILD_TESTS OFF)
option(GLFW_INSTALL OFF)
add_subdirectory(glfw)

# imgui
set(IMGUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/imgui-docking")
set(IMGUI_FILES
    "${IMGUI_DIR}/imgui.cpp"
    "${IMGUI_DIR}/imgui_demo.cpp"
    "${IMGUI_DIR}/imgui_draw.cpp"
    "${IMGUI_DIR}/imgui_tables.cpp"
    "${IMGUI_DIR}/imgui_widgets.cpp"
    "${IMGUI_DIR}/imconfig.h"
    "${IMGUI_DIR}/imgui.h"
    "${IMGUI_DIR}/imgui_internal.h"
    "${IMGUI_DIR}/imstb_rectpack.h"
    "${IMGUI_DIR}/imstb_textedit.h"
    "${IMGUI_DIR}/imstb_truetype.h")
add_library(imgui STATIC ${IMGUI_FILES})
target_include_directories(imgui SYSTEM PUBLIC ${IMGUI_DIR} ${IMGUI_DIR}/backends)

# entt
add_library(entt INTERFACE)
set(ENTT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/entt)
target_sources(entt INTERFACE ${ENTT_DIR}/single_include/entt/entt.hpp)
target_include_directories(entt INTERFACE ${ENTT_DIR}/single_include/entt/)

# glm
add_library(glm INTERFACE)
set(GLM_DIR ${CMAKE_CURRENT_SOURCE_DIR}/glm)
target_include_directories(glm SYSTEM INTERFACE ${GLM_DIR})
target_compile_definitions(glm INTERFACE
    GLM_FORCE_SWIZZLE
    GLM_FORCE_RADIANS
    GLM_FORCE_CTOR_INIT
    GLM_ENABLE_EXPERIMENTAL
    GLM_EXT_INCLUDED
)

# tinygltf
set(TINYGLTF_HEADER_ONLY ON CACHE INTERNAL "" FORCE)
set(TINYGLTF_INSTALL OFF CACHE INTERNAL "" FORCE)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/tinygltf)

# vulkan
add_library(vulkan INTERFACE)
set(VULKAN_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vulkan-headers/include)
target_sources(vulkan INTERFACE ${VULKAN_INCLUDE_DIR}/vulkan/vulkan.h)
target_include_directories(vulkan SYSTEM INTERFACE ${VULKAN_INCLUDE_DIR})

target_compile_definitions(vulkan INTERFACE VK_NO_PROTOTYPES)

# volk
if (WIN32)
   set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_WIN32_KHR)
endif()
add_subdirectory(volk)

# vma
add_library(vma INTERFACE)
set(VMA_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vma/include)
target_sources(vma INTERFACE ${VMA_DIR}/vk_mem_alloc.h)
target_include_directories(vma SYSTEM INTERFACE ${VMA_DIR})
target_link_libraries(vma INTERFACE vulkan)

# excalibur hash
add_library(excalibur_hash INTERFACE)
set(EXCALIBUR_HASH_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/excalibur-hash/ExcaliburHash)
target_sources(excalibur_hash INTERFACE ${EXCALIBUR_HASH_INCLUDE_DIR}/ExcaliburHash.h)
target_include_directories(excalibur_hash SYSTEM INTERFACE ${EXCALIBUR_HASH_INCLUDE_DIR})

# renderdoc
add_library(renderdoc INTERFACE)
set(RENDERDOC_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/renderdoc)
target_sources(renderdoc INTERFACE ${RENDERDOC_INCLUDE_DIR}/renderdoc_app.h)
target_include_directories(renderdoc SYSTEM INTERFACE ${RENDERDOC_INCLUDE_DIR})

# assimp
add_subdirectory(assimp)

# stb
add_library(stb INTERFACE)
set(STB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/stb)
target_sources(stb INTERFACE ${STB_DIR}/stb_image.h)
target_include_directories(stb INTERFACE ${STB_DIR})

# taskflow
add_library(taskflow INTERFACE)
set(TASKFLOW_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/taskflow/)
target_include_directories(taskflow INTERFACE ${TASKFLOW_INCLUDE_DIR})

# ktx
option(KTX_FEATURE_LOADTEST_APPS "Build KTX load test apps" OFF)
option(KTX_FEATURE_TOOLS "Build KTX tools like ktx2check" OFF)
option(KTX_FEATURE_DOC "Build documentation" OFF)
option(KTX_FEATURE_TESTS "Build tests" OFF)
option(KTX_FEATURE_WRITE "Enable writing KTX files" OFF)
add_subdirectory(ktx)