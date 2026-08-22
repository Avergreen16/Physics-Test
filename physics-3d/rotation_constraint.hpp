#pragma once

#include <physics-3d/collider.hpp>
#include <physics-3d/collision.hpp>
#include <physics-3d/constraint.hpp>
#include <include/ecs.hpp>

namespace axiom {

struct rotation_constraint {
    uint32_t a = NULL_ENTITY;
    uint32_t b = NULL_ENTITY;

    collider3d* ca;
    collider3d* cb;
    transform3d* ta;
    transform3d* tb;

    // vectors to be aligned in the space of their object
    vec3 va;
    vec3 vb;

    // vectors in world space
    vec3 wa;
    vec3 wb;

    // vectors to rotate along
    uint32_t c = NULL_ENTITY;
    std::vector<vec3> vs;
    std::vector<vec3> wvs;
    
    std::vector<float> baumgarte;
    std::vector<float> inertia_a;
    std::vector<float> inertia_b;
    std::vector<float> lambda;
    
    float spring = 0.35f;
    float softness = 0.005f;
    float max_impulse = FLT_MAX;

    void before();
    void solve(float delta_time);
    void after();
};
    
}