#pragma once

#include <vector>
#include <physics-3d/collider.hpp>
#include <physics-3d/debugger.hpp>

namespace axiom {

struct hash_s {
    std::size_t operator()(const std::array<uint, 2>& a) const {
        ulong pa = (ulong)a[0];
        ulong pb = (ulong)a[1];

        return pa ^ pb;
    }
};

struct contact_point {
    vec3 a;
    vec3 b;
};

struct collision_data3d {
    bool priority = false;

    contact_point point;

    glm::vec3 normal;
    vec3 tangent = vec3(0.0f, 0.0f, 0.0f);
    vec3 bitangent = vec3(0.0f, 0.0f, 0.0f);

    uint frames = 0;

    float lambdaN = 0.0f;
    float lambdaT = 0.0f;
    float lambdaB = 0.0f;

    float deltaT = 0.0f;
    float deltaB = 0.0f;
    float deltaN = 0.0f;

    vec2 lambda_tolerance = vec2(0.0f);
};

struct manifold {
    vec3 normal;
    uint a = 0xFFFFFFFF;
    uint b = 0xFFFFFFFF;

    std::vector<collision_data3d> points;
};

struct return_point {
    vec3 a;
    vec3 b;
    vec3 normal;
};

enum class collision_type{FACE, EDGE, VERTEX};
struct return_tag {
    collision_type type;
    ivec3 vid_a[3];
    ivec3 vid_b[3];
};

using collision_table = std::unordered_map<std::array<uint, 2>, std::vector<manifold>, hash_s>;

//

// GJK

struct simplex_vertex {
    vec3 m;
    vec3 a;
    vec3 b;
};

struct simplex {
    std::vector<simplex_vertex> vertices;

    uint find_closest_face(vec3& weights, vec3& dir);

    int contains(vec3 point, bool output = false);
};

struct polytope_return {
    std::vector<simplex_vertex> vertices;
    glm::vec3 normal;
    glm::vec3 weights = vec3(-1.0f);
    bool f = false;
};

struct polytope_face {
    std::vector<uint> vertices;
    glm::vec3 normal;
};

struct polytope {
    std::vector<simplex_vertex> vertices;
    std::vector<polytope_face> faces;
    vec3 center = vec3(0.0f);

    polytope_return find_closest_face();
    void insert_face(std::vector<uint> v);
    void from_simplex(simplex s);
    std::vector<uint> expand(simplex_vertex vertex);
};

extern std::vector<std::vector<vec3>> debug_vertices;

vec3 transform_vertices(std::vector<vertex_element3d>& elements, transform3d& transform, vec3 origin);

bool contains(std::vector<vertex_element3d>& elements, vec3 point);
std::vector<return_point> collide(transform3d& ta, collision_shape3d& ca, transform3d& tb, collision_shape3d& cb, return_tag& tag, collision_event& c_event);
std::vector<return_point> collide(transform3d& ta, collider3d& ca, transform3d& tb, collider3d& cb, std::vector<collision_event>& c_event);

//static bool GJK(Temporary_collider3d& a, Temporary_collider3d& b);
bool gjk(collision_shape3d& ca, transform3d& ta, collision_shape3d& cb, transform3d& tb);
std::vector<uint> gjk_bvh(collider3d& ca, transform3d& ta, collider3d& cb, collision_shape3d& ccb, transform3d& tb);

extern collision_event c_event;
extern std::vector<collision_event> c_events;

}