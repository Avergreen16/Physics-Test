
add_library(AxiomPhysics3D STATIC
    bounding_box.cpp
    collider.cpp
    collision_constraint.cpp
    position_constraint.cpp
    collision.cpp
    mesh.cpp
    system.cpp
    debugger.cpp
)

target_include_directories(AxiomPhysics3D
    PUBLIC
        ${AXIOM_ROOT}
        ${AXIOM_ROOT}/engine
        ${AXIOM_ROOT}/external/glm
)

target_link_libraries(AxiomPhysics3D
    PUBLIC
        AxiomMath
        AxiomScene
        AxiomECS
        AxiomUtilities

        atomic
)

if(WIN32)
    target_link_libraries(AxiomPhysics3D
        PRIVATE
            dwmapi
            gdi32
            user32
            shell32
            opengl32
            ws2_32
            wsock32
            mingw32
            mingwex 
            atomic
    )
elseif(UNIX)
    target_link_libraries(AxiomPhysics3D
        PRIVATE
            GL
            X11
            dl
            pthread
    )
endif()

target_compile_options(AxiomPhysics3D PRIVATE
    -g
    -O3
    -fpermissive
    -march=native
    -static
    -Wtemplate-body
    -Wchanges-meaning
)