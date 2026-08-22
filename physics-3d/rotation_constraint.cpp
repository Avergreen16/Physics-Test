#include <physics-3d/rotation_constraint.hpp>
#include <include/ecs.hpp>

namespace axiom {

void rotation_constraint::before() {
    ca = &axiom::global_core.ecs->get_component<collider3d>(a);
    ta = &axiom::global_core.ecs->get_component<transform3d>(a);
    if(b != NULL_ENTITY) {
        cb = &axiom::global_core.ecs->get_component<collider3d>(b);
        tb = &axiom::global_core.ecs->get_component<transform3d>(b);
    }
    
    int i = 0;
    for(vec3 v : wvs) {
        vec3 twirl = lambda[i] * v;
        
        ca->angular_momentum += twirl;
        if(b != NULL_ENTITY) cb->angular_momentum -= twirl;

        ++i;
    }
}

void rotation_constraint::solve() {

    i = 0;
    for(rot_constraint& rc : data.rot) {
        uint32_t j = 0;
        for(vec3 v : rc.wvs) {
            vec3 vel_a = data.ca->iit_rot * data.ca->angular_momentum;
            vec3 vel_b = data.cb->iit_rot * data.cb->angular_momentum;

            vec3 rel_velocity = vel_a - vel_b;

            float baumgarte = -rc.baumgarte[j] * rc.spring / physics_step;

            float L = baumgarte - dot(rel_velocity, v);
            L /= rc.inertia_a[j] + rc.inertia_b[j];
            if(do_dampening) L -= rc.softness * rc.lambda[j];
            float new_lambda = rc.lambda[j] + L;
            new_lambda = clamp(new_lambda, -rc.max_impulse, rc.max_impulse);

            L = new_lambda - rc.lambda[j];
            rc.lambda[j] = new_lambda;
            
            vec3 impulse = v * L;
            
            rot_apply(data.ca, impulse);
            rot_apply(data.cb, -impulse);

            ++j;
        }
        ++i;
    }
}

void rotation_constraint::after() {

}
    
}