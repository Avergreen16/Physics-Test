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

add_executable(Example
    src/main.cpp
)

# icon.rc

#axiom_set_icon(Example)

target_include_directories(Example PUBLIC 
    ${CMAKE_SOURCE_DIR}/external/axiom

    ${CMAKE_SOURCE_DIR}/external/lua
    ${CMAKE_SOURCE_DIR}/src
)

set_target_properties(Example PROPERTIES OUTPUT_NAME "axiom-example")

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/external/wrappers/lua)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/external/glfw)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/external/glad)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/external/glm)

set(AXIOM_INCLUDE_RENDER ON)
set(AXIOM_INCLUDE_UI ON)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/external/axiom)

target_link_libraries(Example PRIVATE
    Axiom
    lua
)

target_compile_options(Example PRIVATE
    -O3
    -fpermissive
    -march=native
    -static
    -Wtemplate-body
    -g
)