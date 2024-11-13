cmake_minimum_required(VERSION 3.16)

project(vendor LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)

# spdlog
add_subdirectory(spdlog)

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

target_include_directories(imgui SYSTEM PUBLIC ${IMGUI_DIR})

# entt
set(ENTT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/entt/single_include/entt")
set(ENTT_FILES
    "${ENTT_DIR}/single_include/entt/entt.hpp")

add_library(entt INTERFACE)

target_include_directories(entt INTERFACE ${ENTT_DIR})