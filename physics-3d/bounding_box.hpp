#pragma once

#include <vector>
#include <include/math.hpp>
#include <include/scene.hpp>

namespace axiom {

struct bounding_box3d {
    vec3 minimum = vec3(FLT_MAX);
    vec3 maximum = vec3(-FLT_MAX);

    float volume;
};

bounding_box3d transform_bounding_box(bounding_box3d b, vec3 pos, mat3 ori);
bool collide(transform3d& ta, bounding_box3d& a, transform3d& tb, bounding_box3d& b);
bool collide(bounding_box3d& a, bounding_box3d& b);

struct vertex_element3d;
struct collider3d;

bounding_box3d create_bounding_box(std::vector<vertex_element3d>& elements);
bounding_box3d create_bounding_box(std::vector<vertex_element3d> elements, vec3 offset_pos, mat3 offset_ori);
void create_bounding_box(collider3d& collider);

// bvh

struct bvh_node3d {
    bounding_box3d bounding_box;
    bool split = true;
    std::vector<uint> children;
};

struct bvh3d {
    std::vector<bvh_node3d> nodes;
};

void create_bvh(collider3d& collider, ivec3 v = ivec3(0.0));
std::vector<uint> traverse_bvh(transform3d& ta, bvh3d& ba, transform3d& tb, bounding_box3d& bb);
std::vector<ulong> traverse_bvh(transform3d& ta, bvh3d& ba, transform3d& tb, bvh3d& bb);

}