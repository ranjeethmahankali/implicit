cmake_minimum_required(VERSION 3.16.0)
set(CMAKE_CXX_STANDARD 17)
if(WIN32)
set(VCPKG_TARGET_TRIPLET x64-windows-static)
else()
set(VCPKG_TARGET_TRIPLET x64-linux)
endif(WIN32)
set(CMAKE_PREFIX_PATH $ENV{VCPKG_PATH}/installed/${VCPKG_TARGET_TRIPLET})
set(CMAKE_TOOLCHAIN_FILE $ENV{VCPKG_PATH}/scripts/buildsystems/vcpkg.cmake)

project(implicitkernel)
include_directories(include)

if(WIN32)
add_compile_definitions(NOMINMAX GLEW_STATIC)
endif(WIN32)

find_package(Boost REQUIRED) # No components used at the moment
if (Boost_FOUND)
include_directories(${Boost_INCLUDE_DIRS})
endif()

find_package(GLEW REQUIRED)
if (GLEW_FOUND)
    include_directories(${GLEW_INCLUDE_DIRS})
    # link_libraries(${GLEW_LIBRARIES})
endif()

find_package(Lua51 REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(OpenGL REQUIRED)

# Implicit kernel - Library
file(GLOB IMPLICITKERNEL_SRC "src/implicitkernel/*.cpp")
add_library(implicitkernel ${IMPLICITKERNEL_SRC})

if (WIN32)
target_include_directories(implicitkernel PUBLIC
dependencies/lightOCLSDK/include)
set(OPENCL_LIB ${CMAKE_SOURCE_DIR}/dependencies/lightOCLSDK/lib/x86_64/OpenCL.lib)
else()
find_package(OpenCL REQUIRED)
set(OPENCL_LIB OpenCL::OpenCL)
endif(WIN32)

if(WIN32)
set(SHLWAPI_LIB shlwapi)
endif(WIN32)

# message(WARNING ${GLEW_LIBRARIES})
target_link_libraries(implicitkernel PUBLIC
    GLEW::GLEW
    OpenGL::GL
    glfw
    ${OPENCL_LIB}
    ${SHLWAPI_LIB})

# Implicit lua bindings - library
file(GLOB IMPLICITLUA_SRC "src/implicitlua/*.cpp")
add_library(implicitlua ${IMPLICITLUA_SRC})

target_link_libraries(implicitlua PUBLIC
    implicitkernel
    ${LUA_LIBRARIES})

#Implicit shell - application
file(GLOB IMPLICITSHELL_SRC "src/implicitshell/*.cpp")
add_executable(implicitshell ${IMPLICITSHELL_SRC})
target_link_libraries(implicitshell PRIVATE
    implicitkernel
    implicitlua)

# Delete NVIDIA compiled opencl cache
add_custom_command(TARGET implicitshell PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E remove_directory
    $ENV{APPDATA}/NVIDIA/ComputeCache)

# Copy all the kernel sources to be compiled at runtime.
list(APPEND KERNEL_FILE_DIRS
    ${CMAKE_SOURCE_DIR}/src/kernels
    ${CMAKE_SOURCE_DIR}/include/kernels)

foreach(KERNEL_DIR IN LISTS KERNEL_FILE_DIRS)
    add_custom_command(TARGET implicitshell POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${KERNEL_DIR} $<TARGET_FILE_DIR:implicitshell>)
endforeach()
