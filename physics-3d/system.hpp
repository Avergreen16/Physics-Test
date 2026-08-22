#pragma once

#include <include/ecs.hpp>
#include <physics-3d/collision.hpp>
#include <physics-3d/constraint.hpp>
#include <physics-3d/debugger.hpp>

namespace axiom {

struct physics_system3d : system {
    int step = 0;
    
    bool sim_active = false;

    // parameters
    float fps = 60.0f;
    float physics_step = 1.0f / fps;
    int max_frames = 1;
    float physics_time = 0.0f;

    float contact_sep = 0.0625f;
    uint iterations = 4;
    uint substeps = 4;
    float sub_dt = physics_step / substeps;

    //

    vec3 gravity_aspect = vec3(1, 1, 1);
    vec3 gravity_center = vec3(0.0f);
    mat3 gravity_orientation = glm::identity<mat3>();
    bool do_DOF = true;
    bool do_dampening = true;

    //

    collision_table collision_table;

    uint temp_constraints = 0;
    std::vector<std::unique_ptr<constraint>> constraints;

    std::vector<ulong> broad_collisions;
    std::vector<manifold> narrow_collisions;
    //std::vector<Constraint> constraints;
    //std::vector<DOF_constraint> dof_constraints;

    //std::vector<Debug_point> debug_points;

    std::vector<vec3> debug_points_a;
    std::vector<vec3> debug_points_b;

    physics_debugger debugger;
    bool capture_events = false;
    debugger_frame current_frame;
    ulong frame_count = 0;

    std::function<vec3(vec3)> gravity = [](vec3 pos) {
        return vec3(0.0f, 0.0f, -30.0f);
    };

    //

    physics_system3d();

    void call();

    void physics_loop();

    void broad_phase();

    void narrow_phase();

    void prune_manifolds();

    void build_constraints();

    void solver();

    //

    contact_point get_points(collision_data3d& data, uint a, uint b);

    void merge_manifolds(manifold& a, manifold& b);

    void velocity_solve();

    void insert_collision(manifold& data);
    
    void integrate();

    //
    
    bool raycast(vec3 start, vec3 direction, float step, float dist, float inflate, std::unordered_set<uint>& mask, uint* hit, uint* shape_hit, vec3* normal, vec3* point);

    vec3 get_gravity(vec3 position);
    /*
    std::vector<ulong> broad_phase();

    static std::vector<shape_face> triangulate_merge(std::vector<vec3> vertices);

    //

    static manifold create_manifold(std::vector<return_point> contacts, uint a, uint b, collider3d& ca, transform3d& ta, collider3d& cb, transform3d& tb);

    static bool collision(transform3d& ta, bounding_box3d& a, transform3d& tb, bounding_box3d& b);

    void insert_collision(manifold& data);

    void prune_manifolds();
    
    void erase_collisions(uint shape);

    void merge_manifolds(manifold& a, manifold& b);
    contact_point get_points(collision_data& data, uint a, uint b);

    static bool contains(std::vector<vec3> points, vec3 radius, vec3 point);
    static float contains_dist(std::vector<vec3> points, vec3 radius, vec3 point);
    
    static std::pair<mat3, vec3> calculate_inertia_tensor_flat(std::vector<vec3> points, float mass);
    static std::pair<mat3, vec3> calculate_inertia_tensor_flat(std::vector<vec3> points, float thickness, float& mass, float density);
    static std::pair<mat3, vec3> calculate_inertia_tensor_flat_volume(std::vector<vec3> points, float thickness, float& volume);
    
    static std::pair<mat3, vec3> calculate_M(std::vector<vec3> points, vec3 radius, float& volume);
    static std::pair<mat3, vec3> calculate_M_flat(std::vector<vec3> points, float thickness, float& volume);
    static mat3 inertia_tensor(mat3 M);
    
    static mat3 translate_inertia_tensor(vec3 delta, mat3 inertia_tensor, float mass);

    static mat3 translate_inertia_tensor_inverse(vec3 delta, mat3 inertia_tensor, float mass);

    static mat3 add_inertia_tensor(mat3 a, mat3 b);

    static void initialize_collision_shape3d(collision_shape3d& shape);
    static vec3 initialize_collider3d(collider3d& collider3d);
    
    static void initialize_collision_shape3d(collision_shape3d& shape, float density);
    static vec3 initialize_collider3d(collider3d& collider3d, std::vector<float> density);

    void integrate();
    void compute_velocities();
    void velocity_solve(std::vector<collision_constraint>& collision_constraints);
    //void position_solve(std::vector<collision_constraint>& collision_constraints);
    
    void apply_position(collider3d* ca, transform3d* ta, vec3 delta_pos, vec3 rel_pos);
    void apply_rotation(collider3d* c, transform3d* t, vec3 delta);

    void physics_loop();

    void call();

    float shape_cast(collider3d& shape, mat3 orientation, vec3 start, vec3 direction, float step, uint* hit = nullptr, vec3* normal = nullptr);

    //static bool raycast(vec3 start, vec3 direction, float step, float dist, std::vector<temporary_collider3d>& collider3ds, uint* hit = nullptr, vec3* normal = nullptr, vec3* point = nullptr);
    */
};

}