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
	# Source files
	"${VULKAN_BACKEND_DIR}/vk.cpp"
	"${VULKAN_BACKEND_DIR}/vk_renderer.cpp"
	"${VULKAN_BACKEND_DIR}/vk_device.cpp"
	"${VULKAN_BACKEND_DIR}/vk_swapchain.cpp"
	"${VULKAN_BACKEND_DIR}/vk_pipeline_layout.cpp"
	"${VULKAN_BACKEND_DIR}/vk_renderpass.cpp"
	"${VULKAN_BACKEND_DIR}/vk_pipeline.cpp"
	"${VULKAN_BACKEND_DIR}/vk_debug.cpp"
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
	# Source files
)

set(LOADERS_DIR "${PROJECT_SOURCE_DIR}/src/loaders")
set(LOADERS_FILES
	# Header files
	"${LOADERS_DIR}/gltf_loader.h"
	"${LOADERS_DIR}/obj_loader.h"
	"${LOADERS_DIR}/stb_image_loader.h"
	# Source files
	"${LOADERS_DIR}/gltf_loader.cpp"
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
set(RENDERER_FILES
	# Header files
	"${RENDERER_DIR}/render_api.h"
	"${RENDERER_DIR}/renderer.h"
	"${RENDERER_DIR}/vertex.h"
	# Source files
)

set(SCENE_DIR "${PROJECT_SOURCE_DIR}/src/scene")
set(COMPONENTS_DIR "${SCENE_DIR}/components")
set(SCENE_FILES
	# Header files
	"${COMPONENTS_DIR}/transform.h"
	"${SCENE_DIR}/entity.h"
	"${SCENE_DIR}/game_object.h"
	"${SCENE_DIR}/scene.h"
	"${SCENE_DIR}/registry.h"
	# Source files
	"${SCENE_DIR}/entity.cpp"
	"${SCENE_DIR}/game_object.cpp"
	"${SCENE_DIR}/scene.cpp"
	"${SCENE_DIR}/registry.cpp"
)

set(UTILS_DIR "${PROJECT_SOURCE_DIR}/src/utils/")
set(UTILS_FILES
	# Header files
	"${UTILS_DIR}/hash.h"
	# Source files
)

source_group("\\" FILES ${ENGINE_FILES})
source_group("backend/vulkan\\" FILES ${VULKAN_BACKEND_FILES})
source_group("core\\" FILES ${CORE_FILES})
source_group("debug\\" FILES ${DEBUG_FILES})
source_group("events\\" FILES ${EVENTS_FILES})
source_group("renderer\\" FILES ${RENDERER_FILES})
source_group("scene\\" FILES ${SCENE_FILES})
source_group("loaders\\" FILES ${LOADERS_FILES})
source_group("platform\\" FILES ${PLATFORM_FILES})
source_group("utilss\\" FILES ${UTILS_FILES})

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
	${UTILS_FILES}
)

add_library(${PROJECT_NAME} OBJECT ${PROJECT_FILES} "src/backend/vulkan/vk_renderer.cpp")
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
	${UTILS_DIR}
)

add_subdirectory(vendor)

# TODO: making links private, does not work currently
# because Apps are using engine references. eg. spdlog
target_link_libraries(${PROJECT_NAME} PUBLIC
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

target_compile_definitions(${PROJECT_NAME} PUBLIC 
		ENABLE_RENDERDOC
	)