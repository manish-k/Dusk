cmake_minimum_required(VERSION 3.29)

project(dusk)
set(CMAKE_CXX_STANDARD 20)

find_program(CLANG_FORMAT_EXE NAMES "clang-format" DOC "Path to clang-format executable")

set(ENGINE_DIR "${PROJECT_SOURCE_DIR}/src")
set(ENGINE_FILES
	# Header files
	"${ENGINE_DIR}/dusk.h"
	"${ENGINE_DIR}/engine.h"
	# Source files
	"${ENGINE_DIR}/engine.cpp"
)

set(BACKEND_DIR "${PROJECT_SOURCE_DIR}/src/backend")
set(VULKAN_BACKEND_DIR "${BACKEND_DIR}/vulkan")
set(VULKAN_BACKEND_FILES
	# Header files
	"${VULKAN_BACKEND_DIR}/vk.h"
	"${VULKAN_BACKEND_DIR}/vk_base.h"
	"${VULKAN_BACKEND_DIR}/vk_types.h"
	"${VULKAN_BACKEND_DIR}/vk_renderer.h"
	"${VULKAN_BACKEND_DIR}/vk_device.h"
	"${VULKAN_BACKEND_DIR}/vk_swapchain.h"
	"${VULKAN_BACKEND_DIR}/vk_pipeline_layout.h"
	"${VULKAN_BACKEND_DIR}/vk_renderpass.h"
	"${VULKAN_BACKEND_DIR}/vk_pipeline.h"
	"${VULKAN_BACKEND_DIR}/vk_debug.h"
	"${VULKAN_BACKEND_DIR}/vk_allocator.h"
	"${VULKAN_BACKEND_DIR}/vk_descriptors.h"
	"${VULKAN_BACKEND_DIR}/vk_pass.h"
	# Source files
	"${VULKAN_BACKEND_DIR}/vk.cpp"
	"${VULKAN_BACKEND_DIR}/vk_renderer.cpp"
	"${VULKAN_BACKEND_DIR}/vk_device.cpp"
	"${VULKAN_BACKEND_DIR}/vk_swapchain.cpp"
	"${VULKAN_BACKEND_DIR}/vk_pipeline_layout.cpp"
	"${VULKAN_BACKEND_DIR}/vk_renderpass.cpp"
	"${VULKAN_BACKEND_DIR}/vk_pipeline.cpp"
	"${VULKAN_BACKEND_DIR}/vk_debug.cpp"
	"${VULKAN_BACKEND_DIR}/vk_allocator.cpp"
	"${VULKAN_BACKEND_DIR}/vk_descriptors.cpp"
)

set(CORE_DIR "${PROJECT_SOURCE_DIR}/src/core")
set(CORE_FILES
	# Header files
	"${CORE_DIR}/application.h"
	"${CORE_DIR}/base.h"
	"${CORE_DIR}/error.h"
	"${CORE_DIR}/log.h"
	"${CORE_DIR}/entrypoint.h"
	"${CORE_DIR}/key_codes.h"
	"${CORE_DIR}/mouse_codes.h"
	"${CORE_DIR}/dtime.h"
	"${CORE_DIR}/buffer.h"
	# Source files
	"${CORE_DIR}/application.cpp"
	"${CORE_DIR}/log.cpp"
)

set(DEBUG_DIR "${PROJECT_SOURCE_DIR}/src/debug")
set(DEBUG_FILES
	# Header files
	"${DEBUG_DIR}/renderdoc.h"
	"${DEBUG_DIR}/profiler.h"
	# Source files
	"${DEBUG_DIR}/debug.cpp"
)

set(LOADERS_DIR "${PROJECT_SOURCE_DIR}/src/loaders")
set(LOADERS_FILES
	# Header files
	"${LOADERS_DIR}/assimp_loader.h"
	"${LOADERS_DIR}/stb_image_loader.h"
	# Source files
	"${LOADERS_DIR}/assimp_loader.cpp"
	"${LOADERS_DIR}/stb_image_loader.cpp"
)

set(PLATFORM_DIR "${PROJECT_SOURCE_DIR}/src/platform")
set(PLATFORM_FILES
	# Header files
	"${PLATFORM_DIR}/window.h"
	"${PLATFORM_DIR}/glfw_vulkan_window.h"
	"${PLATFORM_DIR}/file_system.h"
	"${PLATFORM_DIR}/platform.h"
	# Source files
	"${PLATFORM_DIR}/window.cpp"
	"${PLATFORM_DIR}/glfw_vulkan_window.cpp"
	"${PLATFORM_DIR}/file_system.cpp"
)

set(RENDERER_DIR "${PROJECT_SOURCE_DIR}/src/renderer")
set(RENDERER_SYSTEMS_DIR "${RENDERER_DIR}/systems")
set(RENDERER_SHADERS_DIR "${RENDERER_SYSTEMS_DIR}/shaders")
set(RENDERER_PASSES_DIR "${RENDERER_DIR}/passes")
set(RENDERER_FILES
	# Header files
	"${RENDERER_DIR}/render_api.h"
	"${RENDERER_DIR}/renderer.h"
	"${RENDERER_DIR}/vertex.h"
	"${RENDERER_DIR}/sub_mesh.h"
	"${RENDERER_DIR}/material.h"
	"${RENDERER_DIR}/gfx_buffer.h"
	"${RENDERER_DIR}/frame_data.h"
	"${RENDERER_DIR}/image.h"
	"${RENDERER_DIR}/texture.h"
	"${RENDERER_DIR}/material.h"
	"${RENDERER_DIR}/render_graph.h"
	"${RENDERER_DIR}/texture_db.h"
	"${RENDERER_DIR}/gfx_types.h"
	"${RENDERER_SYSTEMS_DIR}/basic_render_system.h"
	"${RENDERER_SYSTEMS_DIR}/grid_render_system.h"
	"${RENDERER_SYSTEMS_DIR}/lights_system.h"
	"${RENDERER_PASSES_DIR}/render_passes.h"
	# Source files
	"${RENDERER_DIR}/sub_mesh.cpp"
	"${RENDERER_DIR}/texture.cpp"
	"${RENDERER_DIR}/gfx_buffer.cpp"
	"${RENDERER_DIR}/render_graph.cpp"
	"${RENDERER_DIR}/texture_db.cpp"
	"${RENDERER_SYSTEMS_DIR}/basic_render_system.cpp"
	"${RENDERER_SYSTEMS_DIR}/grid_render_system.cpp"
	"${RENDERER_SYSTEMS_DIR}/lights_system.cpp"
	"${RENDERER_PASSES_DIR}/g_buffer_pass.cpp"
	"${RENDERER_PASSES_DIR}/presentation_pass.cpp"
	"${RENDERER_PASSES_DIR}/lighting_pass.cpp"
	"${RENDERER_PASSES_DIR}/skybox_pass.cpp"
)

set(SCENE_DIR "${PROJECT_SOURCE_DIR}/src/scene")
set(COMPONENTS_DIR "${SCENE_DIR}/components")
set(SCENE_FILES
	# Header files
	"${COMPONENTS_DIR}/transform.h"
	"${COMPONENTS_DIR}/camera.h"
	"${COMPONENTS_DIR}/mesh.h"
	"${COMPONENTS_DIR}/lights.h"
	"${SCENE_DIR}/entity.h"
	"${SCENE_DIR}/game_object.h"
	"${SCENE_DIR}/scene.h"
	"${SCENE_DIR}/registry.h"
	"${SCENE_DIR}/camera_controller.h"
	# Source files
	"${SCENE_DIR}/entity.cpp"
	"${SCENE_DIR}/game_object.cpp"
	"${SCENE_DIR}/scene.cpp"
	"${SCENE_DIR}/registry.cpp"
	"${SCENE_DIR}/camera_controller.cpp"
)

set(UI_DIR "${PROJECT_SOURCE_DIR}/src/ui")
set(UI_FILES
	# Header files
	"${UI_DIR}/editor_ui.h"
	"${UI_DIR}/ui_states.h"
	"${UI_DIR}/scene_widgets.h"
	"${UI_DIR}/renderer_widgets.h"
	"${UI_DIR}/stats_widgets.h"
	# Source files
	"${UI_DIR}/editor_ui.cpp"
	"${UI_DIR}/../../vendor/imgui-docking/backends/imgui_impl_vulkan.cpp"
	"${UI_DIR}/../../vendor/imgui-docking/backends/imgui_impl_glfw.cpp"
)

set(UTILS_DIR "${PROJECT_SOURCE_DIR}/src/utils/")
set(UTILS_FILES
	# Header files
	"${UTILS_DIR}/hash.h"
	"${UTILS_DIR}/utils.h"
	# Source files
)

source_group("\\" FILES ${ENGINE_FILES})
source_group("backend/vulkan\\" FILES ${VULKAN_BACKEND_FILES})
source_group("core\\" FILES ${CORE_FILES})
source_group("debug\\" FILES ${DEBUG_FILES})
source_group("events\\" FILES ${EVENTS_FILES})
source_group("renderer\\" FILES ${RENDERER_FILES})
source_group("renderer/systems\\" FILES ${RENDERER_SYSTEMS_DIR})
source_group("renderer/passes\\" FILES ${RENDERER_PASSES_DIR})
source_group("renderer/systems/shaders\\" FILES ${RENDERER_SHADERS_DIR})
source_group("scene\\" FILES ${SCENE_FILES})
source_group("loaders\\" FILES ${LOADERS_FILES})
source_group("platform\\" FILES ${PLATFORM_FILES})
source_group("ui\\" FILES ${UI_FILES})
source_group("utils\\" FILES ${UTILS_FILES})

set(PROJECT_FILES
	${ENGINE_FILES}
	${VULKAN_BACKEND_FILES}
	${CORE_FILES}
	${DEBUG_FILES}
	${EVENTS_FILES}
	${LOADERS_FILES}
	${RENDERER_FILES}
	${SCENE_FILES}
	${PLATFORM_FILES}
	${UI_FILES}
	${UTILS_FILES}
)

add_library(${PROJECT_NAME} OBJECT ${PROJECT_FILES})
target_include_directories(
	${PROJECT_NAME} PUBLIC 
	${ENGINE_DIR}
	${CORE_DIR}
	${DEBUG_DIR}
	${VULKAN_BACKEND_DIR}
	${EVENTS_DIR}
	${LOADERS_DIR}
	${RENDERER_DIR}
	${COMPONENTS_DIR}
	${SCENE_DIR}
	${PLATFORM_DIR}
	${UI_DIR}
	${UTILS_DIR}
)

##target_compile_definitions(${PROJECT_NAME} PUBLIC ENABLE_RENDERDOC)

target_compile_definitions(${PROJECT_NAME} PUBLIC 
	DUSK_ENABLE_PROFILING
	TRACY_ENABLE
)

target_compile_definitions(${PROJECT_NAME} PUBLIC 
	IMGUI_IMPL_VULKAN_USE_VOLK
)

add_subdirectory(vendor)

# TODO: making links private, does not work currently
# because Apps are using engine references. eg. spdlog
target_link_libraries(${PROJECT_NAME} PUBLIC
	stb
	spdlog
	entt
	imgui
	glm
	glfw
	tinygltf
	vulkan
	volk
	vma
	excalibur_hash
	renderdoc
	assimp
	TracyClient
	taskflow
)

if(CLANG_FORMAT_EXE)
    add_custom_target(
		Code-Format COMMAND ${CLANG_FORMAT_EXE} -i -style=file ${PROJECT_FILES})

	add_dependencies(${PROJECT_NAME} Code-Format)
endif()

if(ANDROID)
    target_compile_definitions(${PROJECT_NAME} PUBLIC 
		DUSK_PLATFORM_ANDROID
		VK_USE_PLATFORM_ANDROID_KHR
	)
elseif(WIN32)
    target_compile_definitions(${PROJECT_NAME} PUBLIC 
		DUSK_PLATFORM_WIN32
		VK_USE_PLATFORM_WIN32_KHR
	)
elseif(APPLE)
    target_compile_definitions(${PROJECT_NAME} PUBLIC 
		DUSK_PLATFORM_APPLE
		VK_USE_PLATFORM_METAL_EXT
	)
elseif(UNIX)
    target_compile_definitions(${PROJECT_NAME} PUBLIC 
		DUSK_PLATFORM_UNIX
	)
endif()

set (ENGINE_TEXTURES_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/renderer/systems/textures")
set (ENGINE_TEXTURES_DEST_DIR "${CMAKE_CURRENT_BINARY_DIR}/textures")

set (ENGINE_SHADERS_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/renderer/systems/shaders")
set (ENGINE_SHADERS_DEST_DIR "${CMAKE_CURRENT_BINARY_DIR}/shaders")

set(IMGUI_SRC_CONFIG_FILE "${UI_DIR}/dusk_imgui.ini")
set(IMGUI_DST_CONFIG_FILE "${CMAKE_BINARY_DIR}/dusk_imgui.ini")

target_compile_definitions(${PROJECT_NAME} PUBLIC DUSK_BUILD_PATH=${CMAKE_CURRENT_BINARY_DIR})

## https://github.com/vblanco20-1/vulkan-guide/blob/all-chapters/CMakeLists.txt
find_program(GLSL_VALIDATOR glslang HINTS 
  ${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE} 
  /usr/bin 
  /usr/local/bin 
  ${VULKAN_SDK_PATH}/Bin
  ${VULKAN_SDK_PATH}/Bin32
  $ENV{VULKAN_SDK}/Bin/ 
  $ENV{VULKAN_SDK}/Bin32/
)

# get all .vert and .frag files in shaders directory
file(GLOB_RECURSE DUSK_GLSL_SOURCE_FILES
  "${PROJECT_SOURCE_DIR}/src/renderer/systems/shaders/*.frag"
  "${PROJECT_SOURCE_DIR}/src/renderer/systems/shaders/*.vert"
)

foreach(GLSL ${DUSK_GLSL_SOURCE_FILES})
  get_filename_component(FILE_NAME ${GLSL} NAME)
  set(SPIRV "${PROJECT_SOURCE_DIR}/src/renderer/systems/shaders/${FILE_NAME}.spv")
  add_custom_command(
    OUTPUT ${SPIRV}
	COMMAND ${CMAKE_COMMAND} -E echo "Compiling ${FILE_NAME}"
    COMMAND ${GLSL_VALIDATOR} -g -V ${GLSL} -o ${SPIRV}
    DEPENDS ${GLSL})
  list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(GLSL)

add_custom_target(
    DUSK_SHADERS_COMPILE ALL
    DEPENDS ${SPIRV_BINARY_FILES}
)

add_custom_target(
    DUSK_SHADERS_COPY ALL
	COMMAND ${CMAKE_COMMAND} -E echo "Copying compiled shaders"
	COMMAND ${CMAKE_COMMAND} -E remove_directory ${ENGINE_SHADERS_DEST_DIR}
	COMMAND ${CMAKE_COMMAND} -E copy_directory ${ENGINE_SHADERS_SOURCE_DIR} ${ENGINE_SHADERS_DEST_DIR}
    DEPENDS DUSK_SHADERS_COMPILE
)

add_custom_target(
    DUSK_ASSETS_COPY ALL
	COMMAND ${CMAKE_COMMAND} -E echo "Copying assets"
	COMMAND ${CMAKE_COMMAND} -E remove_directory ${ENGINE_TEXTURES_DEST_DIR}
	COMMAND ${CMAKE_COMMAND} -E copy_directory ${ENGINE_TEXTURES_SOURCE_DIR} ${ENGINE_TEXTURES_DEST_DIR}
	COMMAND ${CMAKE_COMMAND} -E copy ${IMGUI_SRC_CONFIG_FILE} ${IMGUI_DST_CONFIG_FILE}
	DEPENDS DUSK_SHADERS_COPY
)

add_dependencies(${PROJECT_NAME} DUSK_ASSETS_COPY)