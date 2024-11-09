cmake_minimum_required(VERSION 3.16)

project(dusk_engine)

# search for Vulkan SDK
find_package(Vulkan)

if(Vulkan_FOUND)
    if(NOT Vulkan_dxc_exe_FOUND)
        find_program(Vulkan_dxc_EXECUTABLE
                    NAMES dxc
                    HINTS
                    "$ENV{VULKAN_SDK}/Bin"
                    "$ENV{VULKAN_SDK}/bin"
                    )
    endif()
    if(Vulkan_dxc_EXECUTABLE)
        message(STATUS "Found DirectX Shader Compiler under ${Vulkan_dxc_EXECUTABLE}")
    else()
        message(STATUS "Couldn't find DirectX Shader Compiler executable, make sure it is present in Vulkan SDK or add it manually via Vulkan_dxc_EXECUTABLE cmake variable. HLSL shaders won't be compiled.")
    endif()
endif()

add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

add_subdirectory(dusk)

add_subdirectory(samples)