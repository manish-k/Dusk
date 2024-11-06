cmake_minimum_required(VERSION 3.16)

project(dusk)
set(CMAKE_CXX_STANDARD 20)

set(CORE_FILES
	# Header files
	src/core/application.h
	src/core/entrypoint.h
	# Source files
	src/core/application.cpp
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
	${CORE_FILES}
	${DEBUG_FILES}
	${RENDERER_FILES}
	${SCENE_FILES}
	${SYSTEM_FILES}
)

# add_subdirectory(vendor)

add_library(${PROJECT_NAME} OBJECT ${PROJECT_FILES})
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/)
