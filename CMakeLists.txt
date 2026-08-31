cmake_minimum_required(VERSION 3.25)
include(FetchContent)
include(CMakeDependentOption)

project(MLT)

set(CMAKE_CXX_STANDARD 23)

# User configurable options
option(MLT_ENABLE_TRACY "Enable profiling in Tracy" OFF)
cmake_dependent_option(MLT_ENABLE_WAYLAND "Enable Wayland/EGL support" ON "UNIX;NOT APPLE" OFF)

# Library options
set(BUILD_SHARED_LIBS OFF)
set(TRACY_ENABLE ${MLT_ENABLE_TRACY} CACHE BOOL "Enable profiling in Tracy" FORCE)
set(GLM_BUILD_LIBRARY OFF CACHE BOOL "Build GLM dynamic library" FORCE)

if(MLT_ENABLE_WAYLAND)
    message(STATUS "MLT: Wayland support enabled")
endif()
set(GLFW_BUILD_WAYLAND ${MLT_ENABLE_WAYLAND} CACHE BOOL "Build support for Wayland" FORCE)
set(GLEW_EGL ${MLT_ENABLE_WAYLAND} CACHE BOOL "Enable EGL support" FORCE)
set(BUILD_UTILS OFF CACHE BOOL "Build GLEW utilities" FORCE)

# GLEW repository does not contain generated sources. Only releases include
# generated sources.
FetchContent_Declare(glew
        URL https://github.com/nigels-com/glew/releases/download/glew-2.3.0/glew-2.3.0.tgz
        URL_HASH SHA256=b261a06dfc8b970e0a1974488530e58dd2390acf68acb05b45235cd6fb17a086
        SOURCE_SUBDIR build/cmake
)
FetchContent_MakeAvailable(glew)

add_subdirectory(external/tracy)
add_subdirectory(external/argparse)
add_subdirectory(external/glfw)
add_subdirectory(external/glm)
add_subdirectory(external/fastgltf)

set(SRC_FILES
        src/application.cpp
        src/aabb.cpp
        src/aabb4.cpp
        src/bvh.cpp
        src/image.cpp
        src/main.cpp
        src/material.cpp
        src/mesh.cpp
        src/path.cpp
        src/path_tracer.cpp
        src/random.cpp
        src/scene.cpp
        src/threadpool.cpp
        src/mlt.cpp
)

add_executable(MLT ${SRC_FILES})

if(MLT_ENABLE_TRACY)
        target_compile_definitions(MLT PUBLIC TRACY_ENABLE)
        message(STATUS "MLT: Tracy profiling enabled")
endif()

# Enable SIMD (SSE/AVX) for x86/x64
if (MSVC)
        target_compile_options(MLT PRIVATE /arch:AVX2)
else()
        target_compile_options(MLT PRIVATE -mavx2 -mfma -msse4.2)
endif()

target_compile_definitions(MLT PUBLIC
        GLM_FORCE_INTRINSICS
        GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
        GLM_ENABLE_EXPERIMENTAL
        GLM_FORCE_SWIZZLE)

target_include_directories(MLT PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/external/glfw/include)
target_include_directories(MLT PUBLIC ${glew_SOURCE_DIR}/include)
target_include_directories(MLT PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/external/fastgltf/include)
target_include_directories(MLT PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/external/args)
target_include_directories(MLT PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/external/stb_image)
target_link_libraries(MLT PUBLIC TracyClient argparse glfw glew_s fastgltf glm::glm)
