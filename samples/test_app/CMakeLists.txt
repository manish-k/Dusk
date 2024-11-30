cmake_minimum_required(VERSION 3.16)

project(test_app)
set(CMAKE_CXX_STANDARD 20)

file(
	GLOB_RECURSE 
	SOURCE_FILES 
	CONFIGURE_DEPENDS 
	"${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

target_include_directories(
	${PROJECT_NAME} 
	PUBLIC 
	${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(${PROJECT_NAME} PUBLIC dusk)

## TODO: Try with symlink approach
set (ASSETS_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/assets")
set (ASSETS_DEST_DIR "${CMAKE_CURRENT_BINARY_DIR}")
file(COPY ${ASSETS_SOURCE_DIR} DESTINATION ${ASSETS_DEST_DIR})