cmake_minimum_required(VERSION 3.16)

project(dusk)
set(CMAKE_CXX_STANDARD 20)

file(
	GLOB_RECURSE 
	CORE_FILES 
	CONFIGURE_DEPENDS 
	"${CMAKE_CURRENT_SOURCE_DIR}/src/core/*"
)

set(DEBUG_FILES
	# Header files
	# Source files
)

set(RENDERER_FILES
	# Header files
	# Source files
)

set(SCENE_FILES
	# Header files
	# Source files
)

set(SYSTEM_FILES
	# Header files
	# Source files
)

source_group("core\\" FILES ${CORE_FILES})
source_group("debug\\" FILES ${DEBUG_FILES})
source_group("renderer\\" FILES ${RENDERER_FILES})
source_group("scene\\" FILES ${SCENE_FILES})
source_group("system\\" FILES ${SYSTEM_FILES})

set(PROJECT_FILES
	"src/dusk.h"
	${CORE_FILES}
	${DEBUG_FILES}
	${RENDERER_FILES}
	${SCENE_FILES}
	${SYSTEM_FILES}
)

add_subdirectory(vendor/spdlog)
add_subdirectory(vendor/entt)
add_subdirectory(vendor/glfw)
add_subdirectory(vendor/glm)

add_library(${PROJECT_NAME} OBJECT ${PROJECT_FILES})
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/)

# TODO make links private, does not work currently
# because Apps are using engine references. eg. spdlog
target_link_libraries(${PROJECT_NAME} PUBLIC
	spdlog
	#entt
	#glfw
	#glm
	#imgui
)