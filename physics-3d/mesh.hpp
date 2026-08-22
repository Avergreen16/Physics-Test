#pragma once
#include <include/math.hpp>
#include <physics-3d/collider.hpp>

namespace axiom {

struct output_vertex {
    vec3 position;
    uint element;
};
    
void create_mesh(std::vector<vertex_element3d> elements, std::vector<output_vertex>* vertices, std::vector<uint>* indices);
//void create_mesh(axiom::collider3d& shape, std::vector<vec3>* surface);

}