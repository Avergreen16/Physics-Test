cmake_minimum_required(VERSION 3.25)

project(Physics-Test
    VERSION 0
    DESCRIPTION "Physics Test"
    LANGUAGES C CXX
)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(BUILD_SHARED_LIBS OFF)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/external/wrappers/lua)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/external/glfw)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/external/glad)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/external/glm)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/external/freetype)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/external/harfbuzz)

set(AXIOM_INCLUDE_RENDER ON)
set(AXIOM_INCLUDE_UI ON)
set(AXIOM_INCLUDE_PHYSICS3D ON)
set(AXIOM_INCLUDE_PHYSICS2D ON)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/external/axiom)

# icon.rc

#axiom_set_icon(Example)

####

add_executable(example-3d
    src/3d/main.cpp
    src/3d/physics-objects.cpp
)

target_include_directories(example-3d PUBLIC 
    ${CMAKE_SOURCE_DIR}/external/axiom
    ${CMAKE_SOURCE_DIR}/external/lua
    ${CMAKE_SOURCE_DIR}/external/freetype/include
    ${CMAKE_SOURCE_DIR}/external/harfbuzz/src

    ${CMAKE_SOURCE_DIR}/src
)

set_target_properties(example-3d PROPERTIES OUTPUT_NAME "example-3d")

target_link_libraries(example-3d PRIVATE
    Axiom
    lua
    freetype
    harfbuzz
)

target_compile_options(example-3d PRIVATE
    -O3
    -fpermissive
    -march=native
    -static
    -Wtemplate-body
    -g
)

#####


add_executable(example-2d
    src/2d/main.cpp
)

target_include_directories(example-2d PUBLIC 
    ${CMAKE_SOURCE_DIR}/external/axiom

    ${CMAKE_SOURCE_DIR}/external/lua
    ${CMAKE_SOURCE_DIR}/src
)

set_target_properties(example-2d PROPERTIES OUTPUT_NAME "example-2d")

target_link_libraries(example-2d PRIVATE
    Axiom
    lua
)

target_compile_options(example-2d PRIVATE
    -O3
    -fpermissive
    -march=native
    -static
    -Wtemplate-body
    -g
)