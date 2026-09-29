#pragma once

#include <include/core.hpp>
#include <include/render.hpp>

struct world_params {
    vec3 radii;
    vec3 position;

    uint num_chunks;
    uint num_tiles;
};

struct crater_population {
    float min_size;
    float max_size;
    float distribution;
    int num_craters;
    int num_ejecta;
};

struct crater {
    vec3 position;
    float radius = 0.1f;
    float ejecta = 0.0f;
    float age = 0.0f;
    float height = 0.0f;
};

mat4 get_matrix(vec3 y, vec3 z, vec3 origin);
void make_terrain(world_params params);
void make_world(world_params params);
void create_planet(uint seed, vec3 position, mat3 orientation, vec3 dimensions, std::vector<crater_population> populations, std::vector<vec3> colors, float amplitude, float noise_freq, float noise_offset, float age_value, float ejecta_value, float blend_value);

//
mat3 random_orientation(axiom::random32& rand);

std::vector<axiom::shape_face> get_faces(std::vector<axiom::vertex_element3d>& elements);
void create_collider(axiom::collider3d& collider, axiom::transform3d& transform, std::vector<axiom::vertex_element3d>& elements, float mass = 1.0f, bool is_static = false);
void create_mesh(axiom::color_mesh3d& mesh, std::vector<axiom::vertex_element3d>& elements, vec3 color, bool blend_normals = false);
void create_mesh(axiom::color_mesh3d& mesh, std::vector<std::vector<axiom::vertex_element3d>>& elements, vec3 color);
void create_collider(axiom::collider3d& collider, axiom::transform3d& transform, std::vector<std::vector<axiom::vertex_element3d>>& elements, std::vector<float> masses, bool is_static = false);

void create_cuboid(vec3 position, mat3 orientation, vec3 axes, vec3 color, float mass = 1.0f);
void create_cube(vec3 position, mat3 orientation, float diameter, vec3 color, float mass = 1.0f);
void create_tetrahedron(vec3 position, mat3 orientation, float diameter, vec3 color, float mass = 1.0f);
void create_bipyramid(vec3 position, mat3 orientation, float diameter, int num_vertices, vec3 color, float mass = 1.0f);
void create_dodecahedron(vec3 position, mat3 orientation, float diameter, vec3 color, float mass = 1.0f);
void create_icosahedron(vec3 position, mat3 orientation, float diameter, vec3 color, float mass = 1.0f);
uint create_capsule(vec3 position, mat3 orientation, vec2 dimensions, ivec2 vertex_density, vec3 color, float mass = 2.0f);