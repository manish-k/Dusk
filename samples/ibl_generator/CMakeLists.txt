cmake_minimum_required(VERSION 3.29)

project(ibl_generator)
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

set (ASSETS_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/assets")
set (ASSETS_DEST_DIR "${CMAKE_CURRENT_BINARY_DIR}/assets")

# get all .vert and .frag files in shaders directory
file(GLOB_RECURSE GLSL_SOURCE_FILES
  "${PROJECT_SOURCE_DIR}/assets/shaders/*.frag"
  "${PROJECT_SOURCE_DIR}/assets/shaders/*.vert"
)

foreach(GLSL ${GLSL_SOURCE_FILES})
  get_filename_component(FILE_NAME ${GLSL} NAME)
  set(SPIRV "${PROJECT_SOURCE_DIR}/assets//shaders/${FILE_NAME}.spv")
  add_custom_command(
    OUTPUT ${SPIRV}
	COMMAND ${CMAKE_COMMAND} -E echo "Compiling ${FILE_NAME}"
    COMMAND ${GLSL_VALIDATOR} -g -V ${GLSL} -o ${SPIRV}
    DEPENDS ${GLSL})
  list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(GLSL)

add_custom_target(
    IBL_Generator_Shaders ALL
    DEPENDS ${SPIRV_BINARY_FILES}
)

add_custom_target(
    IBL_Generator_Assets ALL
	COMMAND ${CMAKE_COMMAND} -E echo "Copying assets"
	COMMAND ${CMAKE_COMMAND} -E remove_directory ${ASSETS_DEST_DIR}
	COMMAND ${CMAKE_COMMAND} -E copy_directory ${ASSETS_SOURCE_DIR} ${ASSETS_DEST_DIR}

	DEPENDS IBL_Generator_Shaders
)

add_dependencies(${PROJECT_NAME} IBL_Generator_Assets)