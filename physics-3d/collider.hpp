#pragma once

#include <include/math.hpp>
#include <include/scene.hpp>
#include <physics-3d/bounding_box.hpp>

#include <set>

namespace axiom {

/*
struct clipping_plane2d {
    vec2 origin;
    vec2 normal;
};
*/

struct vertex_element3d {
    vec3 center;
    vec3 radii = vec3(0.0f);
    mat3 orientation = glm::identity<mat3>();

    //std::vector<clipping_plane2d> planes;
};

vec3 support(vec3 direction, vec3 center, mat3 orientation, vec3 radii);
vec3 support(vec3 direction, std::vector<vertex_element3d> ellipsoids);
vec3 support(vec3 direction, std::vector<vertex_element3d> ellipsoids, uint& element);

//

struct shape_face {
    std::vector<uint> vertices;
    vec3 normal;
};

struct collision_shape3d {
    std::vector<vertex_element3d> elements;
    std::vector<shape_face> faces;
    
    float mass = 0.0f;
    float radius = 0.0f;
    vec3 split_radius = vec3(0.0f);
    mat3 inertia_tensor = {
        {0, 0, 0},
        {0, 0, 0},
        {0, 0, 0}
    };
    mat3 inverse_inertia_tensor = {
        {0, 0, 0},
        {0, 0, 0},
        {0, 0, 0}
    };

    vec3 center_of_mass;
    
    vec3 position = {0, 0, 0};
    mat3 orientation = glm::identity<mat3>();
    
    bounding_box3d bounding_box = {glm::vec3(FLT_MAX), glm::vec3(-FLT_MAX)};
};

struct collider3d {
    std::vector<collision_shape3d> shapes;
    float mass = 0;
    glm::mat3 inertia_tensor = {
        {0, 0, 0},
        {0, 0, 0},
        {0, 0, 0}
    };
    glm::mat3 inverse_inertia_tensor = {
        {0, 0, 0},
        {0, 0, 0},
        {0, 0, 0}
    };

    mat3 iit_rot;

    glm::vec3 velocity = glm::vec3(0.0f);
    glm::vec3 angular_momentum = glm::vec3(0.0f);

    float friction_coefficient = 0.5;
    float bounce_coefficient = 0.0f;
    bool allow_rotation = false;
    bool allow_gravity = true; 
    bool sleeping = false; 
    bool locked = false;

    bool collect = false;
    std::vector<vec3> colliding_normal;
    std::vector<uint> colliding_with;

    bool is_static = false;

    bounding_box3d bounding_box = {glm::vec3(FLT_MAX), glm::vec3(-FLT_MAX)};
    bool init_bb = false;

    vec3 am_delta = vec3(0.0f);
    vec3 pos_delta = vec3(0.0f);

    bvh3d bvh;

    std::unordered_set<uint> collision_mask;

    void apply_impulse(vec3 impulse, vec3 position);

    vec3 get_velocity(vec3 position);
    
    vec3 get_angular_velocity();
};

mat3 calculate_inertia_tensor(std::vector<vertex_element3d>& elements, vec3& center, float mass);

mat3 translate_inertia_tensor(vec3 delta, mat3 inertia_tensor, float mass);
mat3 inv_translate_inertia_tensor(vec3 delta, mat3 inertia_tensor, float mass);
mat3 add_inertia_tensor(mat3 a, mat3 b);
//mat3 cuboid_inertia_tensor(vec3 size);

mat3 translate_M(vec3 d, mat3 M, float mass);
mat3 inv_translate_M(vec3 d, mat3 M, float mass);
mat3 cuboid_M(vec3 size);
mat3 calculate_M(std::vector<vertex_element3d>& elements, vec3& center);
    
mat3 to_inertia_tensor(mat3 M);

void initialize_shape(collision_shape3d& shape, float mass);
vec3 initialize_collider(collider3d& collider, std::vector<float> mass);
void create_mesh_collider(collider3d& collider, std::vector<vec3> triangles);

}