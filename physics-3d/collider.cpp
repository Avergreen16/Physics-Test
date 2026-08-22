#include <physics-3d/collider.hpp>
#include <physics-3d/collision.hpp>
#include <include/utilities.hpp>

#include <iostream>

namespace axiom {

vec3 support(vec3 direction, vec3 center, mat3 orientation, vec3 radii) {
    vec3 local = glm::transpose(orientation) * direction;

    vec3 q = {
        radii.x * radii.x * local.x,
        radii.y * radii.y * local.y,
        radii.z * radii.z * local.z
    };

    float denom = sqrt(q.x * local.x + q.y * local.y + q.z * local.z);
    if(denom == 0) denom = 1.0f;

    vec3 point = orientation * (q / denom);

    return center + point;
}

vec3 support(vec3 direction, std::vector<vertex_element3d> ellipsoids) {
    float max_dot = -FLT_MAX;
    vec3 point = vec3(0.0f);

    for(vertex_element3d& e : ellipsoids) {
        vec3 new_point;
        //if(e.planes.size()) new_point = support(direction, e.center, e.orientation, e.radii, e.planes);
        new_point = support(direction, e.center, e.orientation, e.radii);

        float new_dot = dot(new_point, direction);

        if(new_dot > max_dot) {
            max_dot = new_dot;
            point = new_point;
        }
    }

    return point;
}

vec3 support(vec3 direction, std::vector<vertex_element3d> ellipsoids, uint& element) {
    float max_dot = -FLT_MAX;
    vec3 point = vec3(0.0f);

    uint ee = 0;
    for(vertex_element3d& e : ellipsoids) {
        vec3 new_point;
        //if(e.planes.size()) new_point = support(direction, e.center, e.orientation, e.radii, e.planes);
        new_point = support(direction, e.center, e.orientation, e.radii);

        float new_dot = dot(new_point, direction);

        if(new_dot > max_dot) {
            max_dot = new_dot;
            point = new_point;

            element = ee;
        }

        ++ee;
    }

    return point;
}

//

void collider3d::apply_impulse(vec3 impulse, vec3 position) {
    bool b0 = glm::isnan(velocity.x) || glm::isnan(velocity.y) || glm::isnan(velocity.z) || glm::isnan(angular_momentum.x) || glm::isnan(angular_momentum.y) || glm::isnan(angular_momentum.z);

    if(allow_rotation) {
        velocity += impulse / mass;

        vec3 torque = cross(position, impulse);
        angular_momentum += torque;
    } else {
        velocity += impulse / mass;
    }
    
    bool b1 = glm::isnan(velocity.x) || glm::isnan(velocity.y) || glm::isnan(velocity.z) || glm::isnan(angular_momentum.x) || glm::isnan(angular_momentum.y) || glm::isnan(angular_momentum.z);
}

bool ff = false;
vec3 collider3d::get_velocity(vec3 position) {
    vec3 linear_velocity = velocity;
    
    if(allow_rotation) {
        vec3 angular_velocity = get_angular_velocity();
        vec3 vel = cross(angular_velocity, position);
        
        linear_velocity += vel;
    }

    ff = false;

    return linear_velocity;
}

vec3 collider3d::get_angular_velocity() {
    vec3 angular_velocity = (iit_rot) * angular_momentum;
    return angular_velocity;
}

// inertia tensor

mat3 calculate_inertia_tensor(std::vector<vertex_element3d>& elements, vec3& center, float mass) {
    center = vec3(0, 0, 0);

    float center_mass = 0;

    bounding_box3d bb = create_bounding_box(elements);

    //

    vec3 area = bb.maximum - bb.minimum;

    ivec3 sample_points = ivec3(8);

    vec3 size = area / vec3(sample_points);

    mat3 inertia_tensor;
    inertia_tensor[0] = vec3(0);
    inertia_tensor[1] = vec3(0);
    inertia_tensor[2] = vec3(0);

    int num = 0;

    for(int z = 0; z < sample_points.z; ++z) {
        for(int y = 0; y < sample_points.y; ++y) {
            for(int x = 0; x < sample_points.x; ++x) {
                vec3 p = {x, y, z};
                p = bb.minimum + area * ((p + 0.5f) / (vec3)sample_points);

                bool contains = axiom::contains(elements, p);

                if(contains) {
                    mat3 cuboid_it = {
                        vec3((1.0f / 12) * (size.y * size.y + size.z * size.z), 0, 0),
                        vec3(0, (1.0f / 12) * (size.x * size.x + size.z * size.z), 0),
                        vec3(0, 0, (1.0f / 12) * (size.x * size.x + size.y * size.y)),
                    };

                    cuboid_it = translate_inertia_tensor(p, cuboid_it, 1.0f);

                    inertia_tensor += cuboid_it;

                    center += p;

                    center_mass += 1;

                    ++num;
                }
            }
        }
    }

    center /= center_mass;

    if(num != 0) {
        float multiplier = mass / float(num);

        inertia_tensor[0] *= multiplier;
        inertia_tensor[1] *= multiplier;
        inertia_tensor[2] *= multiplier;
    }

    return inertia_tensor;
}


mat3 translate_inertia_tensor(vec3 delta, mat3 inertia_tensor, float mass) {
    mat3 tensor = inertia_tensor;

    float squared_delta = dot(delta, delta);

    for(int i = 0; i < 3; ++i) {
        for(int j = 0; j < 3; ++j) {
            tensor[i][j] = inertia_tensor[i][j] + mass * (((i == j) ? squared_delta : 0) - delta[i] * delta[j]);
        }
    }

    return tensor;
}

mat3 inv_translate_inertia_tensor(vec3 delta, mat3 inertia_tensor, float mass) {
    mat3 tensor = inertia_tensor;

    float squared_delta = dot(delta, delta);

    for(int i = 0; i < 3; ++i) {
        for(int j = 0; j < 3; ++j) {
            tensor[i][j] = inertia_tensor[i][j] - mass * (((i == j) ? squared_delta : 0) - delta[i] * delta[j]);
        }
    }

    return tensor;
}

mat3 add_inertia_tensor(mat3 a, mat3 b) {
    mat3 ret = {
        {0, 0, 0},
        {0, 0, 0},
        {0, 0, 0}
    };

    for(int i = 0; i < 3; ++i) {
        for(int j = 0; j < 3; ++j) {
            ret[i][j] = a[i][j] + b[i][j];
        }
    }

    return ret;
}

mat3 translate_M(vec3 d, mat3 M, float mass) {
    mat3 t = {
        vec3(d.x * d.x, d.x * d.y, d.x * d.z),
        vec3(d.y * d.x, d.y * d.y, d.y * d.z),
        vec3(d.z * d.x, d.z * d.y, d.z * d.z)
    };

    return M + mass * t;
}

mat3 inv_translate_M(vec3 d, mat3 M, float mass) {
    mat3 t = {
        vec3(d.x * d.x, d.x * d.y, d.x * d.z),
        vec3(d.y * d.x, d.y * d.y, d.y * d.z),
        vec3(d.z * d.x, d.z * d.y, d.z * d.z)
    };

    return M - mass * t;
}


mat3 cuboid_M(vec3 size) {
    return mat3{
        size.x * size.x / 12.0f, 0.0f, 0.0f,
        0.0f, size.y * size.y / 12.0f, 0.0f,
        0.0f, 0.0f, size.z * size.z / 12.0f
    };
}

mat3 calculate_M(std::vector<vertex_element3d>& elements, vec3& center) {
    center = vec3(0, 0, 0);
    float center_mass = 0;

    bounding_box3d bb = create_bounding_box(elements);

    vec3 area = bb.maximum - bb.minimum;

    ivec3 sample_points = ivec3(8);

    vec3 size = area / vec3(sample_points);

    float cell_volume = size.x * size.y * size.z;

    mat3 M;
    M[0] = vec3(0);
    M[1] = vec3(0);
    M[2] = vec3(0);

    int num = 0;

    mat3 M0 = cuboid_M(size);

    for(int z = 0; z < sample_points.z; ++z) {
        for(int y = 0; y < sample_points.y; ++y) {
            for(int x = 0; x < sample_points.x; ++x) {
                vec3 p = {x, y, z};
                p = bb.minimum + area * ((p + 0.5f) / (vec3)sample_points);

                bool contains = axiom::contains(elements, p);

                if(contains) {
                    mat3 M1 = translate_M(p, M0, 1.0f);

                    M += M1;

                    center_mass += 1.0f;
                    center += p;

                    ++num;
                }
            }
        }
    }

    center /= center_mass;

    if(num != 0) {
        float multiplier = 1.0f / float(num);

        M[0] *= multiplier;
        M[1] *= multiplier;
        M[2] *= multiplier;
    }

    return M;
}

mat3 to_inertia_tensor(mat3 M) {
    float trace = M[0][0] + M[1][1] + M[2][2];

    return glm::identity<mat3>() * trace - M;
}

void initialize_shape(collision_shape3d& shape, float mass) {
    shape.inertia_tensor = calculate_inertia_tensor(shape.elements, shape.center_of_mass, mass);
    shape.mass = mass;
}

vec3 initialize_collider(collider3d& collider, std::vector<float> mass) {
    mat3 total_tensor = {
        {0, 0, 0},
        {0, 0, 0},
        {0, 0, 0}
    };

    float total_mass = 0;

    vec3 center_pos = {0, 0, 0};
    
    uint i = 0;
    for(collision_shape3d& shape : collider.shapes) {
        initialize_shape(shape, mass[i]);

        mat3 shape_tensor = shape.inertia_tensor;
        shape_tensor = inv_translate_inertia_tensor(shape.center_of_mass, shape_tensor, shape.mass);

        //

        shape_tensor = shape.orientation * shape_tensor * transpose(shape.orientation);
        vec3 center = shape.orientation * shape.center_of_mass + shape.position;
        shape_tensor = translate_inertia_tensor(center, shape_tensor, shape.mass);

        total_tensor = add_inertia_tensor(total_tensor, shape_tensor);

        center_pos += center * shape.mass;
        total_mass += shape.mass;

        ++i;
    }

    center_pos /= total_mass;

    //

    total_tensor = inv_translate_inertia_tensor(center_pos, total_tensor, total_mass);
    if(collider.allow_rotation) {
        for(collision_shape3d& shape : collider.shapes) {
            shape.position -= center_pos;
        }
        
        collider.inertia_tensor = total_tensor;
        
        collider.inverse_inertia_tensor = inverse(total_tensor);
    }

    collider.mass = total_mass;
    
    return center_pos;
}

void create_mesh_collider(collider3d& collider, std::vector<vec3> triangles) {
    for(int i = 0; i < triangles.size(); i += 3) {
        vec3 va = triangles[i];
        vec3 vb = triangles[i + 1];
        vec3 vc = triangles[i + 2];

        collision_shape3d shape;
        shape.elements = {vertex_element3d(va), vertex_element3d(vb), vertex_element3d(vc)};
        shape.faces = {axiom::shape_face({0, 1, 2}, glm::normalize(glm::cross(va - vc, vb - vc)))};

        collider.shapes.push_back(shape);
    }

    create_bounding_box(collider);

    create_bvh(collider);
}

}