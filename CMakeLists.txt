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

add_executable(Game 
    src/main.cpp
    src/chat.cpp
    src/world_gen.cpp
)

# icon.rc

#axiom_set_icon(Game)

target_include_directories(Game PUBLIC 
    ${CMAKE_SOURCE_DIR}/external/axiom/include
    ${CMAKE_SOURCE_DIR}/external/glm
    ${CMAKE_SOURCE_DIR}/external/glad
    ${CMAKE_SOURCE_DIR}/external/stb
    ${CMAKE_SOURCE_DIR}/external/lua

    ${CMAKE_SOURCE_DIR}/src
)

set_target_properties(Game PROPERTIES OUTPUT_NAME "Physics-Test")

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/external/assimp)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/external/glad)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/external/glfw)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/external/glm)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/external/xsimd)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/external/wrappers/asio)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/external/wrappers/lua)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/external/wrappers/stb)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/external/json)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/external/axiom)

target_link_libraries(Game 
    PRIVATE
        AxiomWindow
        AxiomMath
        AxiomECS
        AxiomRender
        AxiomUI
        AxiomScene
        AxiomPhysics2D
        AxiomPhysics3D
        
        glm
        nlohmann_json
)

target_compile_options(Game PRIVATE
    -O3
    -fpermissive
    -march=native
    -static
    -Wtemplate-body
    -g
)