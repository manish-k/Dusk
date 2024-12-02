cmake_minimum_required(VERSION 3.16)

project(dusk)
set(CMAKE_CXX_STANDARD 20)

set(ENGINE_DIR "src")
set(ENGINE_FILES
	# Header files
	"${ENGINE_DIR}/dusk.h"
	"${ENGINE_DIR}/engine.h"
	# Source files
	"${ENGINE_DIR}/engine.cpp"
)

set(VULKAN_BACKEND_FILES
	# Header files
	
	# Source files
	
)

set(CORE_DIR "src/core")
set(CORE_FILES
	# Header files
	"${CORE_DIR}/application.h"
	"${CORE_DIR}/base.h"
	"${CORE_DIR}/log.h"
	"${CORE_DIR}/entrypoint.h"
	"${CORE_DIR}/key_codes.h"
	# Source files
	"${CORE_DIR}/application.cpp"
	"${CORE_DIR}/log.cpp"
)

set (EVENTS_DIR "src/events")
set(EVENTS_FILES
	# Header files
	"${EVENTS_DIR}/event.h"
	"${EVENTS_DIR}/key_event.h"
	"${EVENTS_DIR}/mouse_event.h"
	"${EVENTS_DIR}/app_event.h"
	"${EVENTS_DIR}/event_bus.h"
	# Source files
)

file(
	GLOB_RECURSE 
	DEBUG_FILES 
	CONFIGURE_DEPENDS 
	"${CMAKE_CURRENT_SOURCE_DIR}/src/debug/*"
)

set(LOADERS_DIR "src/loaders")
set(LOADERS_FILES
	# Header files
	"${LOADERS_DIR}/gltf_loader.h"
	# Source files
	"${LOADERS_DIR}/gltf_loader.cpp"
)

set(PLATFORM_DIR "src/platform")
set(PLATFORM_FILES
	# Header files
	"${PLATFORM_DIR}/window.h"
	"${PLATFORM_DIR}/glfw_vulkan_window.h"
	# Source files
	"${PLATFORM_DIR}/window.cpp"
	"${PLATFORM_DIR}/glfw_vulkan_window.cpp"
)

set(RENDERER_DIR "src/renderer")
set(RENDERER_FILES
	# Header files
	"${RENDERER_DIR}/render_api.h"
	# Source files
)

set(SCENE_DIR "src/scene")
set(COMPONENTS_DIR "src/scene/components")
set(SCENE_FILES
	# Header files
	"${COMPONENTS_DIR}/transform.h"
	"${SCENE_DIR}/entity.h"
	"${SCENE_DIR}/game_object.h"
	"${SCENE_DIR}/scene.h"
	# Source files
	"${SCENE_DIR}/entity.cpp"
	"${SCENE_DIR}/game_object.cpp"
	"${SCENE_DIR}/scene.cpp"
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

set(PROJECT_FILES
	${ENGINE_FILES}
	${CORE_FILES}
	${DEBUG_FILES}
	${EVENTS_FILES}
	${LOADERS_FILES}
	${RENDERER_FILES}
	${SCENE_FILES}
	${PLATFORM_FILES}
)

add_library(${PROJECT_NAME} OBJECT ${PROJECT_FILES})
target_include_directories(
	${PROJECT_NAME} PUBLIC 
	${ENGINE_DIR}
	${CORE_DIR}
	${EVENTS_DIR}
	${LOADERS_DIR}
	${RENDERER_DIR}
	${COMPONENTS_DIR}
	${SCENE_DIR}
	${PLATFORM_DIR}
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
)