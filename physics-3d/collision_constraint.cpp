#include <physics-3d/collision_constraint.hpp>
#include <include/ecs.hpp>

namespace axiom {

static collision_constraint create(collision_data3d* data) {
    collision_constraint constraint;
    constraint.data = data;

    //
}

void collision_constraint::before() {
    ca = &axiom::global_core.ecs->get_component<collider3d>(a);
    ta = &axiom::global_core.ecs->get_component<transform3d>(a);
    if(b != NULL_ENTITY) {
        cb = &axiom::global_core.ecs->get_component<collider3d>(b);
        tb = &axiom::global_core.ecs->get_component<transform3d>(b);
    }
    
    if(b == NULL_ENTITY) {
        mat3 inverse_tensor_a = ta->orientation * ca->inverse_inertia_tensor * transpose(ta->orientation);
        ca->iit_rot = inverse_tensor_a;

        if(data->tangent.x == 0.0f && data->tangent.y == 0.0f && data->tangent.z == 0.0f) {
            vec3 normal = data->normal;

            vec3 tangent = vec3(0.0f, -normal.z, normal.y);
            float t_len = length(tangent);
            if(t_len < 0.001f) {
                tangent = vec3(-normal.y, normal.x, 0.0f);
                t_len = length(tangent);
            }
            tangent /= t_len;

            vec3 bitangent = normalize(cross(tangent, normal));
            
            data->tangent = tangent;
            data->bitangent = bitangent;
        }

        //

        vec3 pa = ta->orientation * vec3(data->point.a);
        vec3 pb = data->point.b - ta->position;
        constraint_point.a = pa;
        constraint_point.b = pb;
        
        pos_a = constraint_point.a;
        
        normal = data->normal;
        tangent = data->tangent;
        bitangent = data->bitangent;
        
        float inv_mass = 1.0f / ca->mass;
        vec3 d;

        d = cross(pos_a, normal);
        inertiaNa = inv_mass + dot(d, inverse_tensor_a * d);

        d = cross(pos_a, tangent);
        inertiaTa = inv_mass + dot(d, inverse_tensor_a * d);

        d = cross(pos_a, bitangent);
        inertiaBa = inv_mass + dot(d, inverse_tensor_a * d);

        //
        
        inertiaNb = 0.0f;
        inertiaTb = 0.0f;
        inertiaBb = 0.0f;
        
        vec3 rel_a = constraint_point.a;
        vec3 rel_b = constraint_point.b;

        //

        inertiaN = inertiaNa + inertiaNb;
        inertiaT = inertiaTa + inertiaTb;
        inertiaB = inertiaBa + inertiaBb;

        vec3 diff = rel_b - rel_a;
        baumgarteN = dot(diff, normal);
    } else {
        mat3 inverse_tensor_a = ta->orientation * ca->inverse_inertia_tensor * transpose(ta->orientation);
        mat3 inverse_tensor_b = tb->orientation * cb->inverse_inertia_tensor * transpose(tb->orientation);
        ca->iit_rot = inverse_tensor_a;
        cb->iit_rot = inverse_tensor_b;
        
        if(data->tangent.x == 0.0f && data->tangent.y == 0.0f && data->tangent.z == 0.0f) {
            vec3 normal = data->normal;

            vec3 tangent = vec3(0.0f, -normal.z, normal.y);
            float t_len = length(tangent);
            if(t_len < 0.001f) {
                tangent = vec3(-normal.y, normal.x, 0.0f);
                t_len = length(tangent);
            }
            tangent /= t_len;

            vec3 bitangent = normalize(cross(tangent, normal));
            
            data->tangent = tangent;
            data->bitangent = bitangent;
        }

        //

        vec3 pa = ta->orientation * vec3(data->point.a);
        vec3 pb = tb->orientation * vec3(data->point.b); // contact point b is relative to b
        constraint_point.b = pb;
        constraint_point.a = pa;
        // both constraint points are relative to A's position in global space
        
        pos_a = constraint_point.a;
        pos_b = constraint_point.b;
        
        normal = data->normal;
        tangent = data->tangent;
        bitangent = data->bitangent;

        //
        
        float inv_mass = 1.0f / ca->mass;
        vec3 d;

        d = cross(pos_a, normal);
        inertiaNa = inv_mass + dot(d, inverse_tensor_a * d);

        d = cross(pos_a, tangent);
        inertiaTa = inv_mass + dot(d, inverse_tensor_a * d);

        d = cross(pos_a, bitangent);
        inertiaBa = inv_mass + dot(d, inverse_tensor_a * d);
        
        inv_mass = 1.0f / cb->mass;

        d = cross(pos_b, -normal);
        inertiaNb = inv_mass + dot(d, inverse_tensor_b * d);
        
        d = cross(pos_b, -tangent);
        inertiaTb = inv_mass + dot(d, inverse_tensor_b * d);
        
        d = cross(pos_b, -tangent);
        inertiaBb = inv_mass + dot(d, inverse_tensor_b * d);

        vec3 rel_a = constraint_point.a;
        vec3 rel_b = constraint_point.b + (tb->position - ta->position);

        inertiaN = inertiaNa + inertiaNb;
        inertiaT = inertiaTa + inertiaTb;
        inertiaB = inertiaBa + inertiaBb;

        vec3 diff = rel_b - rel_a;
        baumgarteN = dot(diff, normal);
    }

    //

    lambdaN = data->lambdaN;
    lambdaT = data->lambdaT;
    lambdaB = data->lambdaB;

    vec3 normal_impulse = normal * lambdaN;
    vec3 tangent_impulse = tangent * lambdaT;
    vec3 bitangent_impulse = bitangent * lambdaB;

    ca->apply_impulse(normal_impulse, pos_a);
    ca->apply_impulse(tangent_impulse, pos_a);
    ca->apply_impulse(bitangent_impulse, pos_a);

    if(b != NULL_ENTITY) {
        cb->apply_impulse(-normal_impulse, pos_b);
        cb->apply_impulse(-tangent_impulse, pos_b);
        cb->apply_impulse(-bitangent_impulse, pos_b);
    }
}

void collision_constraint::solve(float delta_time) {
    float spring = 0.35f;

    float baumgarte = baumgarteN * spring / delta_time;
    vec3 velocity;

    if(b != NULL_ENTITY) {
        velocity = ca->get_velocity(pos_a) - cb->get_velocity(pos_b);

        // normal force

        float L = baumgarte - dot(normal, velocity);
        L /= inertiaN;
        L -= softness * lambdaN;
        float new_lambda = lambdaN + L;
        new_lambda = glm::clamp(new_lambda, 0.0f, axiom::max_float);
        L = new_lambda - lambdaN;
        lambdaN = new_lambda;
        
        vec3 impulse = normal * L;

        ca->apply_impulse(impulse, pos_a);
        cb->apply_impulse(-impulse, pos_b);

        float friction_max = mu * abs(lambdaN);

        // tangent

        velocity = ca->get_velocity(pos_a) - cb->get_velocity(pos_b);

        L = -dot(tangent, velocity);
        L /= inertiaT;
        new_lambda = lambdaT + L;
        new_lambda = glm::clamp(new_lambda, -friction_max, friction_max);
        L = new_lambda - lambdaT;
        lambdaT = new_lambda;

        impulse = tangent * L;

        ca->apply_impulse(impulse, pos_a);
        cb->apply_impulse(-impulse, pos_b);
        
        // bitangent

        velocity = ca->get_velocity(pos_a) - cb->get_velocity(pos_b);

        L = -dot(bitangent, velocity);
        L /= inertiaB;
        new_lambda = lambdaB + L;
        new_lambda = glm::clamp(new_lambda, -friction_max, friction_max);
        L = new_lambda - lambdaB;
        lambdaB = new_lambda;

        impulse = bitangent * L;

        ca->apply_impulse(impulse, pos_a);
        cb->apply_impulse(-impulse, pos_b);
    } else {
        velocity = ca->get_velocity(pos_a);

        // normal force

        float L = baumgarte - dot(normal, velocity);
        L /= inertiaN;
        L -= softness * lambdaN;
        float new_lambda = lambdaN + L;
        new_lambda = glm::clamp(new_lambda, 0.0f, __FLT_MAX__);
        L = new_lambda - lambdaN;
        lambdaN = new_lambda;
        
        vec3 impulse = normal * L;
        
        ca->apply_impulse(impulse, pos_a);

        float friction_max = mu * abs(lambdaN);

        // tangent

        velocity = ca->get_velocity(pos_a);

        L = -dot(tangent, velocity);
        L /= inertiaT;
        new_lambda = lambdaT + L;
        new_lambda = glm::clamp(new_lambda, -friction_max, friction_max);
        L = new_lambda - lambdaT;
        lambdaT = new_lambda;

        impulse = tangent * L;

        ca->apply_impulse(impulse, pos_a);
        
        // tangent

        velocity = ca->get_velocity(pos_a);

        L = -dot(bitangent, velocity);
        L /= inertiaB;
        new_lambda = lambdaB + L;
        new_lambda = glm::clamp(new_lambda, -friction_max, friction_max);
        L = new_lambda - lambdaB;
        lambdaB = new_lambda;

        impulse = bitangent * L;

        ca->apply_impulse(impulse, pos_a);
    }
}

void collision_constraint::after() {
    data->lambdaN = lambdaN;
    data->lambdaT = lambdaT;
    data->lambdaB = lambdaB;
}

}