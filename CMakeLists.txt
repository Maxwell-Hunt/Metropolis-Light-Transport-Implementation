cmake_minimum_required(VERSION 3.25)
project(MLT)

set(CMAKE_CXX_STANDARD 23)

set(TRACY_ENABLE OFF CACHE BOOL "Enable profiling in Tracy")

add_subdirectory(external/tracy)
add_subdirectory(external/argparse)
add_subdirectory(external/glew)
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
        external/tracy/public/TracyClient.cpp
)

add_executable(MLT ${SRC_FILES})

if(ENABLE_TRACY)
    target_compile_definitions(MLT PUBLIC TRACY_ENABLE)
endif()

# Enable SIMD (SSE/AVX) for x86/x64
if (MSVC)
        target_compile_options(MLT PRIVATE /arch:AVX2)
else()
        target_compile_options(MLT PRIVATE -mavx2 -mfma -msse4.2)
endif()
target_compile_definitions(MLT PUBLIC
        GLM_FORCE_INTRINSICS
        GLM_ENABLE_EXPERIMENTAL
        GLM_FORCE_SWIZZLE)

target_include_directories(MLT PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/external/glfw/include)
target_include_directories(MLT PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/external/glew/include)
target_include_directories(MLT PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/external/fastgltf/include)
target_include_directories(MLT PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/external/args)
target_include_directories(MLT PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/external/stb_image)
target_link_libraries(MLT PRIVATE TracyClient PUBLIC argparse glfw libglew_static fastgltf glm::glm)
