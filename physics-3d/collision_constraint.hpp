#pragma once

#include <physics-3d/collider.hpp>
#include <physics-3d/collision.hpp>
#include <physics-3d/constraint.hpp>
#include <include/ecs.hpp>

namespace axiom {

struct collision_constraint : constraint {
    uint a = NULL_ENTITY;
    uint b = NULL_ENTITY;
    
    collider3d* ca;
    collider3d* cb;
    transform3d* ta;
    transform3d* tb;
    
    collision_data3d* data;
    contact_point constraint_point;

    //

    vec3 va;
    vec3 vb;

    vec3 pos_a;
    vec3 pos_b;

    vec3 normal;
    vec3 tangent;
    vec3 bitangent;

    float lambdaN = 0.0f;
    float lambdaT = 0.0f;
    float lambdaB = 0.0f;

    float inertiaNa;
    float inertiaTa;
    float inertiaBa;
    float inertiaNb;
    float inertiaTb;
    float inertiaBb;
    float inertiaN;
    float inertiaT;
    float inertiaB;

    float baumgarteN = 0.0f;
    float baumgarteT = 0.0f;
    float baumgarteB = 0.0f;

    float prev_lambdaT;
    float prev_lambdaB;
    float normal_force;

    bool apply_friction = false;

    float spring = 0.35f;
    float softness = 0.005f;
    float mu = 0.8f;

    static collision_constraint create(collision_data3d* data);

    void before();
    void solve(float delta_time);
    void after();
};

}