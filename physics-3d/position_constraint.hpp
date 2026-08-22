#pragma once

#include <physics-3d/collider.hpp>
#include <physics-3d/collision.hpp>
#include <physics-3d/constraint.hpp>
#include <include/ecs.hpp>

namespace axiom {

struct position_constraint : constraint {
    uint32_t a = NULL_ENTITY;
    uint32_t b = NULL_ENTITY;

    collider3d* ca;
    collider3d* cb;
    transform3d* ta;
    transform3d* tb;

    // object space
    vec3 va;
    vec3 vb;
    
    // relative space
    vec3 ra;
    vec3 rb;

    // world space
    vec3 wa;
    vec3 wb;
    
    // velocities;
    vec3 vel_a;
    vec3 vel_b;

    std::vector<vec3> vs;

    std::vector<float> baumgarte;
    std::vector<float> inertia_a;
    std::vector<float> inertia_b;
    std::vector<float> lambda;
    
    float spring = 0.35f;
    float softness = 0.005f;
    float max_impulse = FLT_MAX;
    bool is_grab = false;
    
    void before();
    void solve(float delta_time);
    void after();
};
    
}