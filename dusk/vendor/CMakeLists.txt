cmake_minimum_required(VERSION 3.16)

project(vendor)

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

add_library(entt STATIC ${IMGUI_FILES})

target_include_directories(imgui SYSTEM PUBLIC ${IMGUI_DIR})

# entt
set(ENTT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/entt")
set(IMGUI_FILES
    "${ENTT_DIR}/single_include/entt/entt.hpp")

add_library(entt STATIC ${ENTT_DIR})

target_include_directories(entt SYSTEM PUBLIC ${ENTT_DIR})