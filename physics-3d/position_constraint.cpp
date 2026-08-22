#include <physics-3d/position_constraint.hpp>
#include <include/ecs.hpp>

namespace axiom {

void position_constraint::before() {
    ca = &axiom::global_core.ecs->get_component<collider3d>(a);
    ta = &axiom::global_core.ecs->get_component<transform3d>(a);
    if(b != NULL_ENTITY) {
        cb = &axiom::global_core.ecs->get_component<collider3d>(b);
        tb = &axiom::global_core.ecs->get_component<transform3d>(b);
    }

    //

    baumgarte.resize(vs.size());
    inertia_a.resize(vs.size());
    inertia_b.resize(vs.size());
    lambda.resize(vs.size(), 0.0f);

    ra = ta->orientation * va;
    wa = ra + ta->position;
    

    for(int i = 0; i < vs.size(); ++i) {
        vec3 v = vs[i];
        vec3 d = cross(ra, v); 

        inertia_a[i] = 1.0f / ca->mass + dot(d, (ta->orientation * ca->inverse_inertia_tensor * transpose(ta->orientation)) * d);
    }

    if(b != NULL_ENTITY) {
        rb = tb->orientation * vec3(vb);
        wb = rb + tb->position;

        for(int i = 0; i < vs.size(); ++i) {
            vec3 v = vs[i];
            vec3 d = cross(ra, v); 
            
            inertia_b[i] = 1.0f / cb->mass + dot(d, (tb->orientation * cb->inverse_inertia_tensor * transpose(tb->orientation)) * d);
        }
    } else {
        wb = vb;
    }

    for(int i = 0; i < vs.size(); ++i) {
        vec3 diff = wa - wb;
        float dd = dot(diff, vs[i]);
        baumgarte[i] = dd;
    }

    //
    
    uint i = 0;
    for(vec3 v : vs) {
        vec3 impulse = v * lambda[i];

        ca->apply_impulse(impulse, ra);
        if(b != NULL_ENTITY) {
            cb->apply_impulse(-impulse, rb);
        }

        ++i;
    }
}

void position_constraint::solve(float delta_time) {
    if(b == NULL_ENTITY) {
        //std::cout << "CALLED!\n";
        uint32_t i = 0;
        for(vec3 v : vs) {
            vec3 velocity = ca->get_velocity(ra);

            float bg = -baumgarte[i] * spring / delta_time;
            
            float L = bg - dot(v, velocity);
            L /= inertia_a[i];
            //if(do_dampening) L -= softness * lambda[i];
            float new_lambda = lambda[i] + L;
            //new_lambda = clamp(new_lambda, -max_impulse, max_impulse);
            
            L = new_lambda - lambda[i];
            lambda[i] = new_lambda;
            
            vec3 impulse = v * L;
            //std::cout << impulse << " " << a << "\n";
            
            ca->apply_impulse(impulse, ra);

            ++i;
        }
    } else {
        uint32_t i = 0;
        for(vec3 v : vs) {
            vec3 velocity = ca->get_velocity(ra);

            velocity -= cb->get_velocity(rb);

            float bg = -baumgarte[i] * spring / delta_time;
            
            float L = bg - dot(v, velocity);

            L /= inertia_a[i] + inertia_b[i];

            //if(do_dampening) L -= softness * lambda[i];
            float new_lambda = lambda[i] + L;
            //new_lambda = clamp(new_lambda, -max_impulse, max_impulse);

            L = new_lambda - lambda[i];
            lambda[i] = new_lambda;
            
            vec3 impulse = v * L;
            
            ca->apply_impulse(impulse, ra);
            ca->apply_impulse(-impulse, rb);

            ++i;
        }
    }
}

void position_constraint::after() {
    
}

}