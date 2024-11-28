cmake_minimum_required(VERSION 3.16)

project(dusk)
set(CMAKE_CXX_STANDARD 20)

set(CORE_FILES
	# Header files
	"src/core/application.h"
	"src/core/base.h"
	"src/core/log.h"
	"src/core/entrypoint.h"
	"src/core/key_codes.h"
	# Source files
	"src/core/application.cpp"
	"src/core/log.cpp"
)

file(
	GLOB_RECURSE 
	DEBUG_FILES 
	CONFIGURE_DEPENDS 
	"${CMAKE_CURRENT_SOURCE_DIR}/src/debug/*"
)

set(RENDERER_FILES
	# Header files
	# Source files
)

set(SCENE_FILES
	# Header files
	"src/scene/components/transform.h"
	"src/scene/entity.h"
	"src/scene/game_object.h"
	"src/scene/scene.h"
	# Source files
	"src/scene/entity.cpp"
	"src/scene/game_object.cpp"
	"src/scene/scene.cpp"
)

set(LOADERS_FILES
	# Header files
	"src/loaders/gltf_loader.h"
	# Source files
	"src/loaders/gltf_loader.cpp"
)

set(EVENTS_FILE
	# Header files
	"src/events/event.h"
	"src/events/key_event.h"
	"src/events/mouse_event.h"
	"src/events/app_event.h"
	"src/events/event_bus.h"
	# Source files
)

source_group("core\\" FILES ${CORE_FILES})
source_group("debug\\" FILES ${DEBUG_FILES})
source_group("renderer\\" FILES ${RENDERER_FILES})
source_group("scene\\" FILES ${SCENE_FILES})
source_group("loaders\\" FILES ${LOADERS_FILES})
source_group("system\\" FILES ${EVENTS_FILE})

set(PROJECT_FILES
	"src/dusk.h"
	${CORE_FILES}
	${DEBUG_FILES}
	${RENDERER_FILES}
	${SCENE_FILES}
	${LOADERS_FILES}
	${SYSTEM_FILES}
)

add_library(${PROJECT_NAME} OBJECT ${PROJECT_FILES})
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/)

add_subdirectory(vendor)

# TODO make links private, does not work currently
# because Apps are using engine references. eg. spdlog
target_link_libraries(${PROJECT_NAME} PUBLIC
	spdlog
	entt
	imgui
	glm
	glfw
	tinygltf
)