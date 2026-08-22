#include <physics-3d/system.hpp>
#include <physics-3d/collider.hpp>
#include <physics-3d/collision.hpp>
#include <physics-3d/collision_constraint.hpp>
#include <physics-3d/bounding_box.hpp>
#include <include/ecs.hpp>

namespace axiom {

void physics_system3d::call() {
    if(sim_active) {
        physics_time += axiom::global_core.ecs->delta_time;

        uint count = 0;
        while(physics_time >= physics_step) {
            physics_time -= physics_step;
            physics_loop();
            count += 1;

            if(count >= max_frames) {
                physics_time = 0;
                break;
            }
        }
    }
}

void physics_system3d::physics_loop() {
    broad_phase();

    narrow_phase();

    prune_manifolds();

    build_constraints();

    solver();

    debugger.insert_frame(frame_count, current_frame);
    current_frame = debugger_frame();

    uint count = 0;
    for(auto& frame : debugger.frames) {
        count += frame.second.collision_events.size();
    }

    ++frame_count;

    //std::cout << count << " " << debugger.frames.size() << "\n";
}

struct spacial_data {
    uint i;
    bool is_static = false;
    bounding_box3d bb;
    transform3d* t;
};

struct input_data {
    bounding_box3d bounding_box;
    transform3d *transform;
    uint id;
};

void physics_system3d::broad_phase() {
    std::vector<input_data> input;

    for(uint entity : collectors[0].entities) {
        input_data ii;
        ii.id = entity;
        
        collider3d& collider = axiom::global_core.ecs->get_component<collider3d>(entity);
        transform3d& transform = axiom::global_core.ecs->get_component<transform3d>(entity);

        ii.transform = &transform;
        ii.bounding_box = collider.bounding_box;

        input.push_back(ii);
    }

    //

    const std::size_t max_i = 4;
    const float ratio = 4.0f;
    const float start_size = 2.0f;

    std::unordered_set<ulong> set;

    std::array<std::unordered_map<ivec3, std::vector<spacial_data>, hash_coord>, max_i> spacial;

    auto insert_into = [&](input_data& ii) {
        bounding_box3d bb = transform_bounding_box(ii.bounding_box, ii.transform->position, ii.transform->orientation);

        vec3 size = bb.maximum - bb.minimum;

        float max_size = glm::max(glm::max(size.x, size.y), size.z);

        std::unordered_map<ivec3, std::vector<spacial_data>, hash_coord>* spacial_scale;

        float bucket_size = start_size;
        for(int i = 0; i < max_i; ++i) {
            if(max_size <= bucket_size || i == max_i - 1) {
                spacial_scale = &spacial[i];
                break;
            }
            bucket_size *= ratio;
        }

        vec3 min = bb.minimum / bucket_size;
        vec3 max = bb.maximum / bucket_size;

        ivec3 mmin = ivec3(floor(min));
        ivec3 mmax = ivec3(floor(max));

        spacial_data sd;
        sd.i = ii.id;
        sd.bb = bb;

        for(int z = mmin.z; z <= mmax.z; ++z) {
            for(int y = mmin.y; y <= mmax.y; ++y) {
                for(int x = mmin.x; x <= mmax.x; ++x) {
                    ivec3 i = {x, y, z};
                    auto& bucket = spacial_scale->operator[](i);

                    bucket.push_back(sd);
                }
            }
        }
    };

    for(auto& ii : input) {
        insert_into(ii);
    }

    float bucket_size = start_size;
    for(int k = 0; k < max_i; ++k) {
        auto& s = spacial[k];
        for(auto& [key, v] : s) {
            for(int i = 0; i < v.size(); ++i) {
                auto vi = v[i];

                // loop through all shapes in same level
                
                for(int j = i + 1; j < v.size(); ++j) {
                    auto vj = v[j];

                    if(collide(vi.bb, vj.bb)) {
                        ulong kk;
                        if(vi.i < vj.i) kk = (ulong)vi.i | (ulong(vj.i) << 32);
                        else kk = (ulong)vj.i | (ulong(vi.i) << 32);

                        set.emplace(kk);
                    }
                }

                // loop through all shapes in levels above

                vec3 key2 = key;
                for(int m = k + 1; m < max_i; ++m) {
                    key2 /= ratio;
                    ivec3 k2 = floor(key2);
                    if(spacial[m].find(k2) != spacial[m].end()) {
                        auto& v2 = spacial[m].at(k2);

                        for(int j = 0; j < v2.size(); ++j) {
                            auto vj = v2[j];

                            if(collide(vi.bb, vj.bb)) {
                                ulong kk;
                                if(vi.i < vj.i) kk = (ulong)vi.i | (ulong(vj.i) << 32);
                                else kk = (ulong)vj.i | (ulong(vi.i) << 32);

                                set.emplace(kk);
                            }
                        }
                    }
                }
            }
        }
        bucket_size *= ratio;
    }

    broad_collisions = std::vector<ulong>(set.begin(), set.end());
}


manifold create_manifold(std::vector<return_point> contacts, uint32_t a, uint32_t b, collider3d& ca, transform3d& ta, collider3d& cb, transform3d& tb) {
    manifold manifold;
    
    if(ca.mass <= 0.0f || ca.is_static) {
        manifold.a = b;
        manifold.b = NULL_ENTITY;
    } else if(cb.mass <= 0.0f || cb.is_static) {
        manifold.a = a;
        manifold.b = NULL_ENTITY;
    } else {
        manifold.a = a;
        manifold.b = b;
    }

    for(return_point& rp : contacts) {
        collision_data3d d;

        if(ca.mass <= 0.0f || ca.is_static) {
            rp.normal = -rp.normal;
            vec3 p = rp.a;
            rp.a = transpose(tb.orientation) * vec3(rp.b);
            rp.b = p + ta.position;
        } else if(cb.mass <= 0.0f || cb.is_static) {
            rp.a = transpose(ta.orientation) * vec3(rp.a);
            rp.b = rp.b + tb.position;
        } else {
            rp.a = transpose(ta.orientation) * vec3(rp.a);
            rp.b = transpose(tb.orientation) * vec3(rp.b);
        }

        d.point.a = rp.a;
        d.point.b = rp.b;
        d.normal = normalize(rp.normal);
        manifold.normal = d.normal;

        manifold.points.push_back(d);
    }
    
    return manifold;
}

void physics_system3d::narrow_phase() {
    const uint num_threads = 6;
    std::vector<std::thread> threads(num_threads);
    std::vector<std::vector<manifold>> cdata(num_threads);
    std::vector<std::vector<ulong>> threads_collisions(num_threads);
    std::vector<std::vector<collision_event>> thread_events(num_threads);

    uint num_collisions = 0;
    float num_per_thread = float(broad_collisions.size()) / num_threads;
    for(ulong i : broad_collisions) {
        uint fi = num_collisions % num_threads;
        threads_collisions[fi].push_back(i);
        ++num_collisions;
    }

    auto thread_GJK = [&](uint j) {
        for(ulong i : threads_collisions[j]) {
            uint a = i & NULL_ENTITY;
            uint b = i >> 32;
            
            collider3d& ai = axiom::global_core.ecs->get_component<collider3d>(a);
            collider3d& bi = axiom::global_core.ecs->get_component<collider3d>(b);

            uint ii = 0;
            uint jj = 0;

            if(!(ai.is_static && bi.is_static) && !(ai.collision_mask.contains(b) || bi.collision_mask.contains(a))) {
                collider3d& ac = axiom::global_core.ecs->get_component<collider3d>(a);
                transform3d& at = axiom::global_core.ecs->get_component<transform3d>(a);

                collider3d& bc = axiom::global_core.ecs->get_component<collider3d>(b);
                transform3d& bt = axiom::global_core.ecs->get_component<transform3d>(b);

                std::vector<collision_event> events;

                std::vector<return_point> data = collide(at, ac, bt, bc, events);

                for(collision_event& e : events) {
                    e.collider_a = a;
                    e.collider_b = b;
                }

                thread_events[j].insert(thread_events[j].end(), events.begin(), events.end());

                if(data.size()) {
                    manifold m = create_manifold(data, a, b, ac, at, bc, bt);
                    cdata[j].push_back(m);
                }
            }
        }
    };

    for(int i = 0; i < num_threads; ++i) {
        threads[i] = std::thread(thread_GJK, i);
    }
    
    for(int i = 0; i < num_threads; ++i) {
        threads[i].join();
    }
    
    for(int i = 0; i < num_threads; ++i) {
        current_frame.collision_events.insert(current_frame.collision_events.end(), thread_events[i].begin(), thread_events[i].end());
    }

    narrow_collisions.clear();
    for(auto& nc : cdata) narrow_collisions.insert(narrow_collisions.end(), nc.begin(), nc.end());
}

void physics_system3d::insert_collision(manifold& data) {
    std::array<uint32_t, 2> key;
    if(data.a > data.b) {
        key = {data.b, data.a};
    } else key = {data.a, data.b};

    if(collision_table.contains(key)) {
        collision_table[key].push_back(data);
    } else {
        collision_table.emplace(key, std::vector<manifold>{data});
    }
};

void physics_system3d::prune_manifolds() {
    for(auto& manifold : narrow_collisions) {
        insert_collision(manifold);
    }

    std::vector<std::array<uint, 2>> delete_keys;

    for(auto& [key, data] : collision_table) {
        std::vector<manifold> new_manifolds = {data[0]};
        
        for(int i = 1; i < data.size(); ++i) {
            bool insert = true;
            for(manifold& m : new_manifolds) {
                if(dot(m.normal, data[i].normal) > 0.9f) {
                    merge_manifolds(m, data[i]);
                    insert = false;
                    break;
                }
            }
            
            if(insert) new_manifolds.push_back(data[i]);
        }

        std::vector<uint> delete_manifolds;

        debug_vertices[0].clear();

        uint d = 0;
        uint vv = 0;
        for(axiom::manifold& manifold : new_manifolds) {
            uint max_id;
            float max_pen = FLT_MAX;
            
            for(int i = 0; i < manifold.points.size(); ++i) {
                auto& v = manifold.points[i]; 
        
                v.normal = manifold.normal;
                ++vv;

                contact_point p = get_points(v, manifold.a, manifold.b);

                debug_vertices[0].push_back(p.a);
                debug_vertices[0].push_back(p.b);

                vec3 diff = p.a - p.b;
                
                float dot_normal = dot(v.normal, diff);
                float tangent = length(diff - v.normal * dot_normal);

                if((dot_normal > contact_sep) || tangent > contact_sep) {
                    manifold.points.erase(manifold.points.begin() + i);

                    --i;
                } else if(dot_normal < max_pen) {
                    max_id = i;
                    max_pen = dot_normal;
                }
                
                if(manifold.points.size() == 0) {
                    delete_manifolds.push_back(d);
                }
            }
                
            std::vector<collision_data3d>& cdata = manifold.points;

            if(cdata.size() > 4) {
                uint priority_points = 0;
                for(int i = 0; i < 4; ++i) {
                    if(!cdata[i].priority) break;
                    ++priority_points;
                }

                std::vector<uint> ids(4);

                ids[0] = max_id;

                float m = -FLT_MAX;
                uint mi;

                uint i = 0;
                for(auto& v : cdata) {
                    if(i != ids[0]) {
                        float dist = length(vec3(v.point.a - cdata[ids[0]].point.a));

                        if(dist > m) {
                            m = dist;
                            mi = i;
                        }
                    }
                    ++i;
                }
                ids[1] = mi; 
                
                m = -FLT_MAX;
                vec3 n = normalize(vec3(cdata[ids[1]].point.a - cdata[ids[0]].point.a));

                i = 0;
                for(auto& v : cdata) {
                    if(i != ids[0] && i != ids[1]) {
                        vec3 diff = vec3(v.point.a - cdata[ids[0]].point.a);
                        float dist = length(diff - n * dot(n, diff));

                        if(dist > m) {
                            m = dist;
                            mi = i;
                        }
                    }
                    ++i;
                }
                ids[2] = mi; 

                vec3 center = vec3(cdata[ids[1]].point.a - cdata[ids[0]].point.a) + vec3(cdata[ids[2]].point.a - cdata[ids[0]].point.a);
                center /= 3;

                m = -FLT_MAX;

                i = 0;
                for(auto& v : cdata) {
                    if(i != ids[0] && i != ids[1] && i != ids[2]) {
                        vec3 diff = vec3(v.point.a - cdata[ids[0]].point.a);
                        float dist = length(diff - center);

                        if(dist > m) {
                            m = dist;
                            mi = i;
                        }
                    }
                    ++i;
                }
                ids[3] = mi; 
                
                std::vector<collision_data3d> cd(4);

                for(int i = 0; i < 4; ++i) {
                    cd[i] = cdata[ids[i]];
                }

                cdata = cd;
            }

            ++d;
        }
        
        data = new_manifolds;
        
        int num_deleted = 0;
        for(int dd : delete_manifolds) {
            data.erase(data.begin() + dd - num_deleted);
            ++num_deleted;
        }

        if(data.size() == 0) {
            delete_keys.push_back(key);
        }
    }

    for(auto key : delete_keys) {
        collision_table.erase(key);
    }
}

void physics_system3d::build_constraints() {
    temp_constraints = 0;
    for(auto& [k, d] : collision_table) {
        for(auto& j : d) temp_constraints += j.points.size();
    }

    constraints.reserve(constraints.size() + temp_constraints);

    debug_vertices[1].clear();

    uint i = 0;
    for(auto& [k, d] : collision_table) {
        for(manifold& manifold : d) {
            for(int j = 0; j < manifold.points.size(); ++j) {  
                collision_constraint cc;

                //

                cc.a = manifold.a;
                cc.b = manifold.b;

                cc.ca = &axiom::global_core.ecs->get_component<collider3d>(cc.a);
                cc.ta = &axiom::global_core.ecs->get_component<transform3d>(cc.a);
                
                if(cc.ca->collect) cc.ca->colliding_with.push_back(cc.b);

                if(cc.b != NULL_ENTITY) {
                    cc.cb = &axiom::global_core.ecs->get_component<collider3d>(cc.b);
                    cc.tb = &axiom::global_core.ecs->get_component<transform3d>(cc.b);
                    
                    if(cc.cb->collect) cc.cb->colliding_with.push_back(cc.a);
                }
                
                collision_data3d& c = manifold.points[j];
                contact_point p = get_points(c, cc.a, cc.b);

                debug_vertices[1].push_back(p.a);
                debug_vertices[1].push_back(p.b);
                
                cc.normal = c.normal;
                cc.data = &c;

                if(cc.ca->collect) cc.ca->colliding_normal.push_back(c.normal);

                if(manifold.b != NULL_ENTITY) {
                    if(cc.cb->collect) cc.cb->colliding_normal.push_back(-c.normal);
                }

                constraints.push_back(std::make_unique<collision_constraint>(cc));

                ++i;
            }
        }
    }
}

void physics_system3d::solver() {
    for(int i = 0; i < substeps; ++i) {
        integrate();

        velocity_solve();   
    }

    constraints.resize(constraints.size() - temp_constraints);
    temp_constraints = 0;
}

//


contact_point physics_system3d::get_points(collision_data3d& data, uint a, uint b) {
    transform3d& at = axiom::global_core.ecs->get_component<transform3d>(a);
    collider3d& ac = axiom::global_core.ecs->get_component<collider3d>(a);
    
    vec3 pa = at.orientation * data.point.a + at.position;
    vec3 pb;
    if(b != NULL_ENTITY) {
        transform3d& bt = axiom::global_core.ecs->get_component<transform3d>(b);
        collider3d& bc = axiom::global_core.ecs->get_component<collider3d>(b);
        pb = bt.orientation * data.point.b + bt.position;//(bt.position - at.position);
    } else pb = data.point.b;// - at.position;

    return contact_point{pa, pb};
}

void physics_system3d::merge_manifolds(manifold& a, manifold& b) {
    manifold ret = a;

    ret.normal = a.normal * (float)a.points.size() + b.normal * (float)b.points.size();
    ret.normal = normalize(ret.normal);

    for(collision_data3d& c : b.points) {
        bool insert = true;
        for(int i = 0; i < ret.points.size(); ++i) {
            collision_data3d& data_b = ret.points[i];

            vec3 diff_a = data_b.point.a - c.point.a;
            vec3 diff_b = data_b.point.b - c.point.b;

            if(length(diff_a) < contact_sep && length(diff_b) < contact_sep) {
                insert = false;
                break;
            }
        }

        if(insert) ret.points.push_back(c);
    }

    a = ret;
}


physics_system3d::physics_system3d() {
    signature s = axiom::global_core.ecs->update_signature<collider3d>();
    axiom::global_core.ecs->update_signature<transform3d>(s);
    collectors.push_back(collector(s));
}
    
bool physics_system3d::raycast(vec3 start, vec3 direction, float step, float dist, float inflate, std::unordered_set<uint>& mask, uint* hit, uint* shape_hit, vec3* normal, vec3* point) {
    collision_shape3d shape;

    collider3d collider;
    collider.shapes.resize(1);
    collider.shapes[0] = shape;

    *hit = NULL_ENTITY;

    vec3 norm = normalize(direction);
    bool decrease = false;

    float pos = 0.0;
    float length = step;

    float prev_pos = 0.0f;
    float prev_length = 0.0f;
    
    bool is_collide = false;

    float precision = 0.001f;
    bool collide_this_step;
    
    while(abs(length) > precision && pos + length <= dist) {
        collide_this_step = false;

        transform3d t;
        t.position = start + vec3(direction * pos);
        t.orientation = glm::identity<mat3>();

        std::vector<vertex_element3d> vs2 = {
            vertex_element3d(vec3(0.0f), vec3(inflate, inflate, 0.0f)),
            vertex_element3d(direction * length, vec3(inflate, inflate, 0.0f)),
        };
        collider.shapes[0].elements = vs2;
        
        create_bounding_box(collider);

        for(uint entity : collectors[0].entities) {
            if(!mask.contains(entity)) {
                collider3d& ec = axiom::global_core.ecs->get_component<collider3d>(entity);
                transform3d& et = axiom::global_core.ecs->get_component<transform3d>(entity);
                //create_bounding_box(ec);
            
                if(collide(et, ec.bounding_box, t, collider.bounding_box)) {
                    if(ec.bvh.nodes.size()) {
                        std::vector<uint> shapes = gjk_bvh(ec, et, collider, collider.shapes[0], t);

                        if(shapes.size()) {
                            *hit = entity;
                            *shape_hit = shapes[0];
                            
                            collide_this_step = true;
                            goto end_loop;
                        }
                    } else {
                        uint c = 0;
                        for(collision_shape3d& ce : ec.shapes) {
                            for(collision_shape3d& convex : collider.shapes) {
                                if(collide(et, ce.bounding_box, t, convex.bounding_box)) {
                                    bool b = gjk(ce, et, convex, t);

                                    if(b) {
                                        *hit = entity;
                                        *shape_hit = c;
                                        
                                        collide_this_step = true;
                                        goto end_loop;
                                    }
                                }
                            }
                            ++c;
                        }
                    }
                }
            }
        }

        end_loop:

        if(collide_this_step) {
            is_collide = true;
            decrease = true;
        }

        if(decrease) {
            if(collide_this_step) {
                prev_pos = pos;
                prev_length = length;

                length *= 0.5f;
            } else {
                pos += length;
                
                length *= 0.5f;
            }
        } else {
            pos += length;
        }
    }

    // get normal
    
    transform3d t;
    t.position = start + vec3(direction * prev_pos);
    t.orientation = glm::identity<mat3>();
    
    std::vector<vertex_element3d> vs2 = {
        vertex_element3d(vec3(0.0f), vec3(inflate, inflate, 0.0f)),
        vertex_element3d(direction * length, vec3(inflate, inflate, 0.0f)),
    };
    collider.shapes[0].elements = vs2;

    //

    *point = start + vec3(direction * pos);

    create_bounding_box(collider);

    for(uint entity : collectors[0].entities) {
        if(!mask.contains(entity)) {
            collider3d& ec = axiom::global_core.ecs->get_component<collider3d>(entity);
            transform3d& et = axiom::global_core.ecs->get_component<transform3d>(entity);

            if(collide(et, ec.bounding_box, t, collider.bounding_box)) {
                std::vector<collision_event> events;

                auto rp = collide(et, ec, t, collider, events);

                if(rp.size()) {
                    *normal = -rp[0].normal;
                }
            }
        }
    }

    if(dot(*normal, direction) > 0.0f) *normal = -*normal;

    return is_collide;
}

vec3 physics_system3d::get_gravity(vec3 position) {
    return gravity(position);
}

void physics_system3d::integrate() {
    for(uint32_t c : collectors[0].entities) {
        transform3d& c_transform = axiom::global_core.ecs->get_component<transform3d>(c);
        collider3d& c_collider = axiom::global_core.ecs->get_component<collider3d>(c);

        if(!c_collider.is_static) {
            c_transform.position += c_collider.velocity * sub_dt;

            if(c_collider.allow_rotation && glm::length(c_collider.angular_momentum)) {
                glm::vec3 angular_velocity = (c_transform.orientation * c_collider.inverse_inertia_tensor * transpose(c_transform.orientation)) * c_collider.angular_momentum;
                
                float len_av = length(angular_velocity);
                vec3 norm_av = angular_velocity / len_av;
                if(len_av * sub_dt != 0.0f) {
                    glm::mat3 rotation = glm::rotate(len_av * sub_dt, norm_av);
        
                    c_transform.orientation = rotation * c_transform.orientation;
                }
            }
            
            if(c_collider.allow_gravity) {
                vec3 gravity_acceleration = get_gravity(c_transform.position);

                c_collider.velocity += gravity_acceleration * sub_dt;
            }
        }
    }
}

void physics_system3d::velocity_solve() {
    for(auto& constraint : constraints) {
        constraint->before();
    }

    /*
    
    */

    for(int i = 0; i < iterations; ++i) {
        /*
        for(Constraint& data : constraints) {
            for(pos_constraint& pc : data.pos) {
                if(data.b == NULL_ENTITY) {
                    pc.vel_a = data.ca->get_velocity(pc.ra);
                } else {
                    pc.vel_a = data.ca->get_velocity(pc.ra);
                    pc.vel_b = data.cb->get_velocity(pc.rb);
                }
            }
        }
        */
        
        /*
        for(int j = 0; j < constraints.size(); ++j) { 
        //for(collision_constraint& data : collision_constraints) {
            int start = 0;//core.random.next() % collision_constraints.size();
            int dir = 1;

            if(i % 2 == 1) {
                start = constraints.size() - 1;
                dir = -1;
            }

            Constraint& data = constraints[start + j * dir];

            if(data.b == NULL_ENTITY) {
                uint i = 0;
                for(pos_constraint& pc : data.pos) {
                    uint j = 0;
                    for(vec3 v : pc.vs) {
                        vec3 velocity = data.ca->get_velocity(pc.ra);

                        float baumgarte = -pc.baumgarte[j] * pc.spring / physics_step;
                        
                        float L = baumgarte - dot(v, velocity);
                        L /= pc.inertia_a[j];
                        if(do_dampening) L -= pc.softness * pc.lambda[j];
                        float new_lambda = pc.lambda[j] + L;
                        //new_lambda = clamp(new_lambda, -pc.max_impulse, pc.max_impulse);
                        
                        L = new_lambda - pc.lambda[j];
                        pc.lambda[j] = new_lambda;
                        
                        vec3 impulse = v * L;
                        
                        lambda_apply(data.ca, impulse, pc.ra);

                        ++j;
                    }
                    ++i;
                }
            } else {
                uint i = 0;
                for(pos_constraint& pc : data.pos) {
                    uint j = 0;
                    for(vec3 v : pc.vs) {
                        if(data.ca->mass == 1.0f) ff = true;
                        vec3 velocity = data.ca->get_velocity(pc.ra);

                        if(data.cb->mass == 1.0f) ff = true;
                        velocity -= data.cb->get_velocity(pc.rb);

                        float baumgarte = -pc.baumgarte[j] * pc.spring / physics_step;
                        
                        float L = baumgarte - dot(v, velocity);

                        std::cout << baumgarte << " " << (L - baumgarte) << "\n";
                        L /= pc.inertia_a[j] + pc.inertia_b[j];

                        if(do_dampening) L -= pc.softness * pc.lambda[j];
                        float new_lambda = pc.lambda[j] + L;
                        //new_lambda = clamp(new_lambda, -pc.max_impulse, pc.max_impulse);

                        L = new_lambda - pc.lambda[j];
                        pc.lambda[j] = new_lambda;
                        
                        vec3 impulse = v * L;
                        
                        lambda_apply(data.ca, impulse, pc.ra);
                        lambda_apply(data.cb, -impulse, pc.rb);

                        ++j;
                    }
                    ++i;
                } 
                
                i = 0;
                for(rot_constraint& rc : data.rot) {
                    uint j = 0;
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
        }
        */

        for(auto& constraint : constraints) {
            constraint->solve(physics_step);
        }

        /*
        if(do_DOF) {
            for(DOF_constraint& c : dof_constraints) {
                c.apply(sub_dt);
            }
        }
        */
    }

    for(auto& constraint : constraints) {
        constraint->after();
    }
}

}