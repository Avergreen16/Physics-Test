#include <include/core.hpp>
#include <include/physics3d.hpp>

#include "physics-objects.hpp"

mat3 random_orientation(axiom::random32& rand) {
    glm::quat q = {rand() * 2.0f - 1.0f, rand() * 2.0f - 1.0f, rand() * 2.0f - 1.0f, rand() * 2.0f - 1.0f};

    q = glm::normalize(q);

    return mat3(q);
}

mat4 get_matrix(vec3 y, vec3 z, vec3 origin) {
    mat3 m = mat3(glm::cross(y, z), y, z);

    return glm::translate(origin) * mat4(m);
}

void make_terrain(world_params params) {
    float terrain_scale = 200.0f;
    float terrain_period = 1000.0f;
    float terrain_size = 1000.0f;
    uint seed = 0xFFF;

    axiom::random32 rand(seed);
    mat3 rot0 = random_orientation(rand);
    mat3 rot1 = random_orientation(rand);
    
    auto get_noise = [&](vec3 position) {
        float a = axiom::noise_gen::ridged_perlin_noise(rot0 * (position + vec3(22.91f, 28.19f, -66.53f)), terrain_period, 5, seed);
        float b = axiom::noise_gen::ridged_perlin_noise(rot1 * (position + vec3(10.91f, -23.19f, 16.53f)), terrain_period, 5, seed);

        float x = a * b;
        x *= x;

        x += axiom::noise_gen::perlin_noise(rot0 * (position + vec3(6.91f, -43.19f, 13.53f)), terrain_period * 0.1f, 5, seed) * 0.1f;

        return x;
    };
    
    /*
    */
    
    for(int yc = 0; yc < params.num_chunks; ++yc) {  
        for(int xc = 0; xc < params.num_chunks; ++xc) {
            std::vector<vec3> collision_triangles;
            std::vector<axiom::color_vertex3d> mesh_vertices;
            
            std::vector<vec3> vs;
            std::vector<vec3> ns;

            for(int y = 0; y < params.num_tiles + 1; ++y) {  
                for(int x = 0; x < params.num_tiles + 1; ++x) {
                    vec3 pos = vec3(float(x + xc * params.num_tiles) - params.num_chunks * params.num_tiles * 0.5f, float(y + yc * params.num_tiles) - params.num_chunks * params.num_tiles * 0.5f, 0);
                    pos *= terrain_size / (params.num_tiles * params.num_chunks);

                    vec3 vz = vec3(0.0f, 0.0f, 1.0f);
                    vec3 vx = vec3(1.0f, 0.0f, 0.0f);
                    vec3 vy = vec3(0.0f, 1.0f, 0.0f);

                    vec3 pos_x = pos + vx * 0.1f;
                    vec3 pos_y = pos + vy * 0.1f;
                    
                    float noise = get_noise(pos);
                    float noise_x = get_noise(pos_x);
                    float noise_y = get_noise(pos_y);

                    pos_x = pos_x + vz * noise_x * terrain_scale;
                    pos_y = pos_y + vz * noise_y * terrain_scale;
                    pos = pos + vz * noise * terrain_scale;

                    vs.push_back(pos + vec3(0.0f, 0.0f, terrain_scale));
                    ns.push_back(normalize(cross(pos_x - pos, pos_y - pos)));
                }
            }

            vec3 up_color = axiom::hsv_color(2.0f, 0.9f, 0.45f);
            vec3 down_color = axiom::hsv_color(0.35f, 0.5f, 0.65f);

            for(int y = 0; y < params.num_tiles; ++y) {  
                for(int x = 0; x < params.num_tiles; ++x) {
                    ivec2 a = {x, y};
                    ivec2 b = {x + 1, y};
                    ivec2 c = {x, y + 1};
                    ivec2 d = {x + 1, y + 1};

                    vec3 va = vs[a.y * (params.num_tiles + 1) + a.x];
                    vec3 vb = vs[b.y * (params.num_tiles + 1) + b.x];
                    vec3 vc = vs[c.y * (params.num_tiles + 1) + c.x];
                    vec3 vd = vs[d.y * (params.num_tiles + 1) + d.x];
                    
                    vec3 na = ns[a.y * (params.num_tiles + 1) + a.x];
                    vec3 nb = ns[b.y * (params.num_tiles + 1) + b.x];
                    vec3 nc = ns[c.y * (params.num_tiles + 1) + c.x];
                    vec3 nd = ns[d.y * (params.num_tiles + 1) + d.x];

                    float sa = 0.8f;
                    float sb = 0.875f;

                    axiom::color_vertex3d cva(va, vec4(glm::mix(down_color, up_color, glm::smoothstep(sa, sb, na.z)), 1.0f), na);
                    axiom::color_vertex3d cvb(vb, vec4(glm::mix(down_color, up_color, glm::smoothstep(sa, sb, nb.z)), 1.0f), nb);
                    axiom::color_vertex3d cvc(vc, vec4(glm::mix(down_color, up_color, glm::smoothstep(sa, sb, nc.z)), 1.0f), nc);
                    axiom::color_vertex3d cvd(vd, vec4(glm::mix(down_color, up_color, glm::smoothstep(sa, sb, nd.z)), 1.0f), nd);
                    
                    vec3 normal_a = glm::normalize(glm::cross(va - vd, vb - vd));
                    vec3 normal_b = glm::normalize(glm::cross(va - vc, vd - vc));
                    vec3 avg_n = glm::normalize(normal_a + normal_b);

                    //cva.normal = normal_a;
                    //cvb.normal = normal_a;
                    //cvd.normal = normal_a;
                    mesh_vertices.push_back(cva);
                    mesh_vertices.push_back(cvb);
                    mesh_vertices.push_back(cvd);
                    
                    //cva.normal = normal_b;
                    //cvd.normal = normal_b;
                    //cvc.normal = normal_b;
                    mesh_vertices.push_back(cva);
                    mesh_vertices.push_back(cvd);
                    mesh_vertices.push_back(cvc);

                    collision_triangles.push_back(va);
                    collision_triangles.push_back(vb);
                    collision_triangles.push_back(vd);
                    collision_triangles.push_back(va);
                    collision_triangles.push_back(vd);
                    collision_triangles.push_back(vc);
                }
            }

            axiom::transform3d transform;
            axiom::color_mesh3d mesh;
            //axiom::collider3d collider;
            
            transform.position = vec3(0.0f);
            transform.orientation = glm::identity<mat3>();

            mesh.vs = mesh_vertices;
            //mesh.texture = &axiom::get_system<main_system>().textures["tilesheet"];
            mesh.load();

            //axiom::create_mesh_collider(collider, collision_triangles);
            //collider.is_static = true;

            uint entity = axiom::ecs.insert_entity();
            axiom::ecs.insert_component(entity, transform);
            axiom::ecs.insert_component(entity, mesh);
            //axiom::ecs.insert_component(entity, collider);
        }
    }
}

void make_world(world_params params) {
    std::vector<crater_population> populations;

    // blue moon
    populations.clear();
    populations.push_back({0.025f, 0.075f, 5.0f, 75, 0});
    populations.push_back({0.005f, 0.025f, 25.0f, 50000, 500});
    std::vector<vec3> colors = {
        axiom::hsv_color(5.85, 0.4, 0.15),
        axiom::hsv_color(5.85, 0.4, 0.125),
        axiom::hsv_color(5.85, 0.4, 0.25),
        axiom::hsv_color(5.85, 0.4, 0.25)
        /*
        axiom::hsv_color(0.05, 0.45, 0.25),
        axiom::hsv_color(0.05, 0.45, 0.2),
        axiom::hsv_color(0.05, 0.45, 0.4),
        axiom::hsv_color(0.05, 0.45, 0.4)
        */
    };

    uint seed = 0xB3;
    vec3 position = params.position;
    mat3 orientation = glm::identity<mat3>();
    vec3 dimensions = params.radii;
    float amplitude = 1000.0f;
    float noise_freq = 0.3f;
    float noise_offset = 0.0f;
    float age_value = 1.0f;
    float ejecta_value = 0.25f;
    float blend_value = 0.125f;

    create_planet(seed, position, orientation, dimensions, populations, colors, amplitude, noise_freq, noise_offset, age_value, ejecta_value, blend_value);
}

void create_planet(uint seed, vec3 position, mat3 orientation, vec3 dimensions, std::vector<crater_population> populations, std::vector<vec3> colors, float amplitude, float noise_freq, float noise_offset, float age_value, float ejecta_value, float blend_value) {
    std::vector<mat4> transforms = {
        get_matrix(vec3(0, 0, 1), vec3(1, 0, 0), vec3(1, 0, 0)),
        get_matrix(vec3(0, 0, 1), vec3(0, 1, 0), vec3(0, 1, 0)),
        get_matrix(vec3(-1, 0, 0), vec3(0, 0, 1), vec3(0, 0, 1)),
        get_matrix(vec3(0, 0, 1), vec3(-1, 0, 0), vec3(-1, 0, 0)),
        get_matrix(vec3(0, 0, 1), vec3(0, -1, 0), vec3(0, -1, 0)),
        get_matrix(vec3(1, 0, 0), vec3(0, 0, -1), vec3(0, 0, -1)),
    };
    
    double start_time = axiom::get_time();

    axiom::random32 rand(seed);

    std::unordered_map<ivec3, std::vector<uint>, axiom::hash_coord> partition;
    int num_buckets = 12;

    float avg_dimension = (dimensions.x + dimensions.y + dimensions.z) / 3.0f;
    float max_dimension = glm::max(glm::max(dimensions.x, dimensions.y), dimensions.z);
    vec3 ratio = dimensions / avg_dimension;

    int max_bucket = floor((max_dimension / avg_dimension) * num_buckets);

    struct crater {
        vec3 position;
        float radius = 0.1f;
        float ejecta = 0.0f;
        float age = 0.0f;
        float height = 0.0f;
    };

    std::vector<crater> craters;

    auto smooth_min = [](float a, float b, float k) {
        if(k < 0.0f) {
            a = -a;
            b = -b;
            k = -k;
            
            float r = exp2(-a/k) + exp2(-b/k);
            return k*log2(r);
        } else {
            float r = exp2(-a/k) + exp2(-b/k);
            return -k*log2(r);
        }
    };

    auto crater_func = [&](float f, float depth, float steepness_inner, float steepness_outer, float rim_width) {
        float walls = (f * f - 1) * steepness_inner;
        float rim = (f - (1.0f + rim_width));
        rim = rim * rim * steepness_outer;

        float result = smooth_min(walls, rim, 0.05f);

        return result;
    };

    auto big_crater_func = [&](float f, float depth, float steepness_inner, float steepness_outer, float rim_width, float steepness_center, float width_center) {
        float walls = (f * f - 1) * steepness_inner;
        float rim = (f - (1.0f + rim_width));
        rim = rim * rim * steepness_outer;

        float peak_pos = f - width_center;
        peak_pos = peak_pos * peak_pos * steepness_center;
        peak_pos += depth;
        if(f > width_center) peak_pos = glm::mix(depth, -1.0f, glm::smoothstep(width_center, 1.5f, f));

        float peak_neg = f + width_center;
        peak_neg = glm::max(peak_neg, 0.0f);
        peak_neg = peak_neg * peak_neg * steepness_center;

        float crater_floor = glm::mix(depth, -1.0f, glm::smoothstep(1.0f, 1.5f, f));

        float peak = smooth_min(peak_pos, peak_neg, 0.05f);

        float result = smooth_min(walls, rim, 0.05f);
        if(steepness_center != 0.0f) result = smooth_min(result, peak, -0.05f);
        result = smooth_min(result, crater_floor, -0.05f);

        return result;
    };

    auto bias_func = [](float x, float bias) {
        float k = pow(1 - bias, 3);
        return x * k / (x * k - x + 1);
    };
    
    int num_prev = 0;
    for(int j = 0; j < populations.size(); ++j) {
        crater_population& pop = populations[j];

        for(int i = 0; i < pop.num_craters; ++i) {
            crater c;
            c.position = rand.unit_vector() * ratio;
            float r = abs(rand());
            r = exp(-r * pop.distribution);

            c.radius = r * (pop.max_size - pop.min_size) + pop.min_size;

            if(pop.num_craters - i < pop.num_ejecta) {
                c.ejecta = c.radius * (5.0f + 15.0f * abs(rand()));
                //c.ejecta = c.radius * 9.0f;

                c.age = float(pop.num_craters - i) / pop.num_ejecta;
                c.age = pow(c.age, age_value);
                //c.age = pow(c.age, 4.0f);
            }

            craters.push_back(c);



            float max_rad = glm::max(c.radius * 1.5f, c.ejecta);

            vec3 mmin = c.position - max_rad;
            vec3 mmax = c.position + max_rad;
            ivec3 rmin = floor(mmin * float(num_buckets));
            ivec3 rmax = floor(mmax * float(num_buckets));

            for(int z = rmin.z; z <= rmax.z; ++z) {
                for(int y = rmin.y; y <= rmax.y; ++y) {
                    for(int x = rmin.x; x <= rmax.x; ++x) {
                        ivec3 bucket = ivec3(x, y, z);

                        if(!partition.contains(bucket)) partition.emplace(bucket, std::vector<uint>());

                        auto& p = partition[bucket];

                        p.push_back(i + num_prev);
                    }
                }
            }
        }

        num_prev += pop.num_craters;
    }

    auto sample_moon_noise = [](vec3 pos, float seed) {
        float n0 = axiom::noise_gen::perlin_noise(pos, 0.45f, 5, seed, 0.5075f);

        vec3 forward = vec3(1, 0, 0);
        float d = glm::smoothstep(0.0f, 1.0f, dot(normalize(pos), forward));
        d -= 0.15f;
        n0 -= d * 0.7f;
        n0 += 0.5f;
        n0 *= 1.5f;

        return n0;
    };

    auto get_noise = [&](vec3 pos, float seed, float amplitude, int num_craters) {
        float n = sample_moon_noise(pos / avg_dimension, seed);
        n *= amplitude;
        
        vec3 n_pos = pos / avg_dimension;
        ivec3 b = floor(n_pos * float(num_buckets));
        auto& bucket = partition[b];

        float crater_depth = n;
        
        for(int i : bucket) {
            if(i < num_craters) {
                crater& c = craters[i];
                
                vec3 rel = c.position - n_pos;
                float dist = length(rel);
                dist /= c.radius;

                float variation = 0.2f;

                if(dist < 1.5f + variation) {
                    float noise_v = axiom::noise_gen::perlin_noise(rel / c.radius, 0.2f, 2.0f, seed) * 0.25f;
                    noise_v += axiom::noise_gen::perlin_noise(rel / c.radius, 0.5f, 2.0f, seed);
                    dist += noise_v * variation;
                    dist = glm::max(0.001f, dist);

                    if(dist < 1.5f) {
                        float crater_scale = c.radius * 0.075;
                        float height = crater_scale * avg_dimension;
                        float depth = height;

                        float ret;
                        if(c.radius > 0.175f) ret = big_crater_func(dist, -c.radius * 0.125f, 1.75f, 1.75f, 0.5f, 3.0f, 0.5f);
                        else ret = crater_func(dist, -depth, 1.25f, 1.75f, 0.5f);

                        float new_depth = -ret;
                        
                        float floor = c.height - height;
                        float target_height = floor - crater_depth;
                        if(target_height < 0.0) {
                            float a = target_height * new_depth;
                            crater_depth = a + crater_depth;
                        }
                    }
                }
            }
        }
    
        return crater_depth;
    };

    auto get_color = [&](vec3 pos, float elevation) {
        vec3 p_i = pos * dimensions;

        float sep = blend_value * amplitude;
        float blend = (elevation + (sep * 0.5f)) / sep;
        blend = glm::clamp(blend, 0.0f, 1.0f);
        
        vec3 n_pos = p_i / avg_dimension;
        ivec3 b = floor(n_pos * float(num_buckets));
        auto& bucket = partition[b];

        float ff = 0.0;
        
        for(int i : bucket) {
            crater& c = craters[i];

            vec3 rel_pos = c.position - n_pos;

            float dist = length(rel_pos);

            if(c.ejecta != 0.0f) {
                //if(dist < c.ejecta) ff = 1.0;
                float dist2 = glm::max(0.0f, (dist - c.radius) / (c.ejecta - c.radius));
                if(dist2 < 1.0) { 
                    vec3 flattened = glm::normalize(rel_pos - c.position * dot(rel_pos, c.position));
                    float noise = axiom::noise_gen::ridged_perlin_noise(flattened, 0.25f, 2, i);
                    noise = 1.0f - noise;

                    dist2 = pow(dist2, 0.75f);

                    float fade = (dist + c.radius * noise * 0.25f - c.radius) / (c.radius * 0.5f);
                    fade = glm::clamp(fade, 0.0f, 1.0f);
                    fade = glm::smoothstep(0.0f, 1.0f, fade);
                    float fade2 = (1.0f - fade) * 0.75f;
                    fade = 0.75f + fade * 0.25f;

                    float f = (noise * (1.0f - ejecta_value) + ejecta_value) - dist2;
                    f = glm::max(f * fade, fade2);

                    f = glm::clamp(f, 0.0f, 1.0f);

                    ff = glm::max(ff, f * c.age);
                }
            }
        }

        vec3 c0 = glm::mix(colors[1], colors[3], glm::clamp(ff, 0.0f, 1.0f));
        vec3 c1 = glm::mix(colors[0], colors[2], glm::clamp(ff, 0.0f, 1.0f));

        vec3 color = glm::mix(c0, c1, blend);

        return color;
    };
    
    for(int i = 0; i < craters.size(); ++i) {
        crater& c = craters[i];
        vec3 pos = c.position * avg_dimension;

        float height = get_noise(pos, seed, amplitude, i - 1);
        c.height = height;
    }
    
    auto get_normal = [&](vec3 pos, vec3 axes) {
        return normalize(vec3(pos.x / (axes.x * axes.x), pos.y / (axes.y * axes.y), pos.z / (axes.z * axes.z)));
    };

    auto project = [&](vec3 normal, vec3 axes) {
        return normalize(normal / axes) * axes;
    };
    
    auto create_m = [&](float width, uint tiles, vec3 origin, mat3 orientation, vec3 p_size, vec3 position, uint seed, mat3 planet_ori) {
        //axiom::collider3d collider;
        //collider.allow_rotation = false;
        //collider.allow_gravity = false;
        //collider.is_static = true;

        std::vector<axiom::texture_range_vertex3d> mesh_vertices;
        std::vector<uint> mesh_indices;

        std::vector<float> values;

        auto find_pos = [&](ivec2 pos) {
            vec3 mpos = origin + vec3(vec2(pos) / float(tiles) * width, 0);
            mpos = orientation * mpos;
            return normalize(mpos);
        };

        float diff = width / tiles * glm::max(glm::max(p_size.x, p_size.y), p_size.z);

        for(int y = 0; y < tiles + 1; ++y) {
            for(int x = 0; x < tiles + 1; ++x) {
                vec3 pos = find_pos({x, y});
                values.push_back(get_noise(pos * p_size, seed, amplitude, craters.size()));
            }
        }

        std::vector<vec3> mesh_colors;
        std::vector<vec3> mesh_normals;
        for(int y = 0; y < tiles + 1; ++y) {
            float multiplier_y = -1.0f;
            int y1 = y + 1;

            for(int x = 0; x < tiles + 1; ++x) {
                float multiplier_x = -1.0f;
                int x1 = x + 1;

                ivec2 vvi = ivec2(x, y);

                vec3 pos = find_pos(vvi);
                vec3 p_i = pos * p_size;

                mat3 ori = axiom::rotate_to(vec3(0, 0, 1), glm::normalize(p_i));

                vec3 delta_x = ori * vec3(1, 0, 0);
                vec3 delta_y = ori * vec3(0, 1, 0);

                vec3 p_x = project(p_i + delta_x * diff, p_size);
                vec3 p_y = project(p_i + delta_y * diff, p_size);

                float vi = values[vvi.y * (tiles + 1) + vvi.x];
                float vx = get_noise(p_x, seed, amplitude, craters.size());
                float vy = get_noise(p_y, seed, amplitude, craters.size());

                vec3 pos_i = p_i + get_normal(p_i, p_size) * vi;
                vec3 pos_x = p_x + get_normal(p_x, p_size) * (vx);
                vec3 pos_y = p_y + get_normal(p_y, p_size) * (vy);

                vec3 normal = cross(pos_x - pos_i, pos_y - pos_i);
                normal = normalize(normal);

                mesh_normals.push_back(normal);

                vec3 color = get_color(pos, vi);
                mesh_colors.push_back(color);
            }
        }
        
        std::vector<ivec2> indices = {
            {0, 0},
            {1, 0},
            {1, 1},
            {0, 0},
            {1, 1},
            {0, 1}
        };
        
        for(int y = 0; y < tiles; ++y) {
            for(int x = 0; x < tiles; ++x) {
                ivec2 pos = {x, y};

                for(ivec2 v : indices) {
                    ivec2 new_pos = pos + v;

                    vec3 normal = mesh_normals[new_pos.y * (tiles + 1) + new_pos.x];
                    vec3 color = mesh_colors[new_pos.y * (tiles + 1) + new_pos.x];
                    float h = values[new_pos.y * (tiles + 1) + new_pos.x];

                    axiom::texture_range_vertex3d mv;
                    mv.position = find_pos(new_pos) * p_size;
                    mv.position += get_normal(mv.position, p_size) * h;
                    mv.normal = normal;

                    mv.color = vec4(color, 1.0f);

                    mesh_indices.push_back(mesh_vertices.size());
                    mesh_vertices.push_back(mv);
                }
            }
        }

        std::vector<vec3> triangles;

        // texture mesh
        vec3 mesh_avg = vec3(0.0f);
        for(int tt = 0; tt < mesh_indices.size() / 3; ++tt) {
            axiom::texture_range_vertex3d& v0 = mesh_vertices[tt * 3];
            axiom::texture_range_vertex3d& v1 = mesh_vertices[tt * 3 + 1];
            axiom::texture_range_vertex3d& v2 = mesh_vertices[tt * 3 + 2];

            mesh_avg += v0.position;
            mesh_avg += v1.position;
            mesh_avg += v2.position;

            vec3 normal = glm::normalize(glm::cross(v0.position - v2.position, v1.position - v2.position));
            vec3 n = normal;

            normal = normalize(round(normal / glm::max(abs(normal.x), glm::max(abs(normal.y), abs(normal.z)))));

            vec3 tex_x = cross(normal, vec3(0, 1, 0));
            if(length(tex_x) == 0.0f) tex_x = cross(normal, vec3(0, 0, 1));
            tex_x = normalize(tex_x);
            vec3 tex_y = normalize(cross(normal, tex_x));

            vec3 avg_pos = v0.position + v1.position + v2.position;
            avg_pos /= 3.0f;

            v0.texture = vec2(dot(tex_x, v0.position), dot(tex_y, v0.position)) * 16.0f;
            v1.texture = vec2(dot(tex_x, v1.position), dot(tex_y, v1.position)) * 16.0f;
            v2.texture = vec2(dot(tex_x, v2.position), dot(tex_y, v2.position)) * 16.0f;

            v0.texture_range = vec4(72, 0, 16, 16);
            v1.texture_range = vec4(72, 0, 16, 16);
            v2.texture_range = vec4(72, 0, 16, 16);

            // change
            //v0.normal = n;
            //v1.normal = n;
            //v2.normal = n;
        }
        if(mesh_indices.size()) mesh_avg /= mesh_indices.size();

        for(axiom::texture_range_vertex3d& v : mesh_vertices) {
            v.position -= mesh_avg;
        }
        
        for(int tt = 0; tt < mesh_indices.size() / 3; ++tt) {
            axiom::texture_range_vertex3d& v0 = mesh_vertices[tt * 3];
            axiom::texture_range_vertex3d& v1 = mesh_vertices[tt * 3 + 1];
            axiom::texture_range_vertex3d& v2 = mesh_vertices[tt * 3 + 2];

            triangles.push_back(v0.position);
            triangles.push_back(v1.position);
            triangles.push_back(v2.position);
        }
        
        axiom::transform3d t;
        t.position = mesh_avg;
        t.orientation = planet_ori;

        //t.position = apply_matrix(planet_ori, t.position);
        t.position += position;

        axiom::texture_range_mesh3d mesh;
        mesh.vs = mesh_vertices;
        mesh.load();
        mesh.texture = &axiom::get_texture("tilesheet");
        
        //axiom::create_mesh_collider(collider, triangles);
        //collider.is_static = true;

        //

        uint entity = axiom::ecs.insert_entity();
        axiom::ecs.insert_component(entity, t);
        axiom::ecs.insert_component(entity, mesh);
        //axiom::ecs.insert_component(entity, collider);   
        
        //axiom::collider3d& ccl = ecs.get_component<axiom::collider3d>(entity);
        //axiom::transform3d& tf = ecs.get_component<axiom::transform3d>(entity);

        //axiom::physics_system3d& ps = ecs.get_system<axiom::physics_system3d>();
        //ps.create_bounding_box(ccl, tf);
        //ccl.create_BVH(2, &tf);
    };

    
    double setup_time = axiom::get_time();

    int split = 6;
    float size = 1.0f;
    uint tiles_per_split = 32;

    for(int i = 0; i < 6; ++i) {
        mat3 matrix = transforms[i];
        for(int x = 0; x < split; ++x) {
            for(int y = 0; y < split; ++y) {
                vec3 origin = vec3(size * 2.0f / split * x - size, size * 2.0f / split * y - size, size);
                //std::cout << i << " " << x << " " << y << "\n";
                
                //if(i == 0 && x < 16 && y < 16) 
                create_m(size * 2.0f / split, tiles_per_split, origin, matrix, dimensions, position, seed, orientation);
            }
        }
    }
    
    // /profiler_planet.output();
    
    float width = size * 2.0f / split;
    float tiles = tiles_per_split;
    float diff = width / tiles * glm::max(glm::max(dimensions.x, dimensions.y), dimensions.z);
    
    double end_time = axiom::get_time();

    //std::cout << "setup: " << setup_time - start_time << "\n";
    //std::cout << "loop: " << end_time - setup_time << "\n";
    //std::cout << "total: " << end_time - start_time << "\n\n";

    /*
    int image_size = 1024;
    std::vector<uint8_t> pixels(image_size * (image_size / 2) * 4);
    
    if(false) {
        for(int y = 0; y < image_size / 2; ++y) {
            for(int x = 0; x < image_size; ++x) {
                int i = y * image_size + x;

                vec2 p = map_project(ivec2(x, y), ivec2(image_size, image_size / 2));

                vec3 color = vec3(0.0f);
                if(p.x >= 0.0f && p.x < 1.0f && p.y >= 0.0f && p.y < 1.0f) {
                    mat3 r = rotate(float(M_PI), vec3(0.0f, 0.0f, 1.0f));
                    vec3 pos = r * wrap(p);
                    float elev = get_noise(pos * dimensions, seed, amplitude, craters.size());

                    color = get_color(pos, elev);

                    // normal

                    vec3 base_sun = normalize(vec3(1, 0, 1));

                    mat3 ori = rotate_to(vec3(0, 0, 1), pos);

                    vec3 delta_x = ori * vec3(1, 0, 0);
                    vec3 delta_y = ori * vec3(0, 1, 0);

                    vec3 p_i = pos * dimensions;
                    vec3 p_x = project(p_i + delta_x * diff, dimensions);
                    vec3 p_y = project(p_i + delta_y * diff, dimensions);

                    float vx = get_noise(p_x, seed, amplitude, craters.size());
                    float vy = get_noise(p_y, seed, amplitude, craters.size());

                    vec3 pos_i = p_i + get_normal(p_i, dimensions) * elev;
                    vec3 pos_x = p_x + get_normal(p_x, dimensions) * vx;
                    vec3 pos_y = p_y + get_normal(p_y, dimensions) * vy;

                    vec3 normal = cross(pos_x - pos_i, pos_y - pos_i);
                    normal = normalize(normal);

                    base_sun = ori * base_sun;
                    float factor = dot(normal, base_sun);
                    factor = (factor - 0.5f) * 2.0f + 0.5f;
                    factor = clamp(factor, 0.0f, 1.0f);
                    color *= factor;
                }

                pixels[i * 4] = color.x * 255;
                pixels[i * 4 + 1] = color.y * 255;
                pixels[i * 4 + 2] = color.z * 255;
                pixels[i * 4 + 3] = 255;
            }
        }

        std::string filename = "output/map_moon" + std::to_string(uint64_t(get_absolute_time() * 10)) + ".png";
        stbi_flip_vertically_on_write(true);
        stbi_write_png(filename.c_str(), image_size, image_size / 2, 4, pixels.data(), 4 * image_size);
    }*/
};

//

std::vector<axiom::shape_face> get_faces(std::vector<axiom::vertex_element3d>& elements) {
    std::vector<axiom::output_vertex> surface_vertices;
    std::vector<uint> surface_indices;
    axiom::create_mesh(elements, &surface_vertices, &surface_indices);

    vec3 center = vec3(0.0f);
    for(auto& element : elements) center += element.center;
    center /= elements.size();

    std::vector<axiom::shape_face> faces;

    for(int i = 0; i < surface_indices.size(); i += 3) {
        uint a = surface_indices[i];
        uint b = surface_indices[i + 1];
        uint c = surface_indices[i + 2];
        a = surface_vertices[a].element;
        b = surface_vertices[b].element;
        c = surface_vertices[c].element;

        vec3 va = elements[a].center;
        vec3 vb = elements[b].center;
        vec3 vc = elements[c].center;

        vec3 normal = glm::normalize(cross(va - vc, vb - vc));
        if(dot(normal, va - center) < 0.0f) normal = -normal;
        
        std::vector<uint> is = {a, b, c};
        std::sort(is.begin(), is.end());

        faces.push_back({is, normal});
    }

    for(int i = 0; i < faces.size(); ++i) {
        for(int j = i + 1; j < faces.size(); ++j) {
            axiom::shape_face face_i = faces[i];
            axiom::shape_face face_j = faces[j];

            if(dot(face_i.normal, face_j.normal) > 0.98f) {
                std::vector<uint> new_i;
                std::set_union(face_i.vertices.begin(), face_i.vertices.end(), face_j.vertices.begin(), face_j.vertices.end(), std::back_inserter(new_i));

                face_i.normal = normalize(face_i.normal + face_j.normal);
                face_i.vertices = new_i;
                faces[i] = face_i;

                faces.erase(faces.begin() + j);
                --j;
            }
        }
    }

    for(auto& face : faces) {
        vec3 center = vec3(0.0f);
        for(uint i : face.vertices) center += elements[i].center;
        center /= float(face.vertices.size());

        vec3 x = glm::normalize(elements[face.vertices[0]].center - center);
        vec3 y = glm::normalize(glm::cross(face.normal, x));

        std::sort(face.vertices.begin(), face.vertices.end(), 
            [center, norm = face.normal, x, y, &elements](const uint& a, const uint& b) {
                vec3 va = elements[a].center - center;
                vec3 vb = elements[b].center - center;

                float sa = dot(va, x);
                float sb = dot(vb, x);
                bool ba = sa >= 0.0f;
                bool bb = sb >= 0.0f;
                if(ba != bb) return sa < sb;
                
                return dot(norm, cross(va, vb)) < 0.0f;
            }
        );
    }

    return faces;
};

void create_collider(axiom::collider3d& collider, axiom::transform3d& transform, std::vector<axiom::vertex_element3d>& elements, float mass = 1.0f, bool is_static = false) {
    axiom::collision_shape3d shape;
    shape.elements = elements;
    collider.shapes = {shape};

    collider.is_static = is_static;
    collider.allow_rotation = true;

    vec3 offset = axiom::initialize_collider(collider, {mass});
    transform.position += transform.orientation * offset;
    
    axiom::create_bounding_box(collider);


    // get faces
    std::vector<axiom::shape_face> faces;

    if(collider.shapes[0].elements.size() >= 3) collider.shapes[0].faces = get_faces(collider.shapes[0].elements);
};

void create_mesh(axiom::color_mesh3d& mesh, std::vector<axiom::vertex_element3d>& elements, vec3 color) {
    std::vector<axiom::output_vertex> surface_vertices;
    std::vector<uint> surface_indices;
    axiom::create_mesh(elements, &surface_vertices, &surface_indices);

    std::unordered_map<uint, vec3> normals;

    for(uint i = 0; i < surface_indices.size(); i += 3) {
        uint i0 = surface_indices[i];
        uint i1 = surface_indices[i + 1];
        uint i2 = surface_indices[i + 2];

        axiom::output_vertex v0 = surface_vertices[i0];
        axiom::output_vertex v1 = surface_vertices[i1];
        axiom::output_vertex v2 = surface_vertices[i2];

        vec3 scaled_normal = cross(v0.position - v2.position, v1.position - v2.position);

        if(v0.element != v1.element || v1.element != v2.element || v2.element != v0.element) {
            i0 |= 0x80000000;
            i1 |= 0x80000000;
            i2 |= 0x80000000;
        }
        
        if(!normals.contains(i0)) normals[i0] = vec3(0.0f);
        if(!normals.contains(i1)) normals[i1] = vec3(0.0f);
        if(!normals.contains(i2)) normals[i2] = vec3(0.0f);

        if(!(elements[v0.element].radii.x + elements[v0.element].radii.y == 0.0f || 
        elements[v0.element].radii.y + elements[v0.element].radii.z == 0.0f || 
        elements[v0.element].radii.z + elements[v0.element].radii.x == 0.0f)) {
            normals[i0] += scaled_normal;
        }

        if(!(elements[v1.element].radii.x + elements[v1.element].radii.y == 0.0f || 
        elements[v1.element].radii.y + elements[v1.element].radii.z == 0.0f || 
        elements[v1.element].radii.z + elements[v1.element].radii.x == 0.0f)) {
            normals[i1] += scaled_normal;
        }

        if(!(elements[v2.element].radii.x + elements[v2.element].radii.y == 0.0f || 
        elements[v2.element].radii.y + elements[v2.element].radii.z == 0.0f || 
        elements[v2.element].radii.z + elements[v2.element].radii.x == 0.0f)) {
            normals[i2] += scaled_normal;
        }

        //
    };

    for(auto& [key, normal] : normals) {
        float len = length(normal);
        if(len != 0.0f) normal /= len;
    }

    for(uint i = 0; i < surface_indices.size(); i += 3) {
        uint i0 = surface_indices[i];
        uint i1 = surface_indices[i + 1];
        uint i2 = surface_indices[i + 2];

        axiom::output_vertex v0 = surface_vertices[i0];
        axiom::output_vertex v1 = surface_vertices[i1];
        axiom::output_vertex v2 = surface_vertices[i2];
        
        if(v0.element != v1.element || v0.element != v2.element || v1.element != v2.element) {
            i0 |= 0x80000000;
            i1 |= 0x80000000;
            i2 |= 0x80000000;
        }
        
        axiom::color_vertex3d vertex;
        vertex.color = vec4(color, 1.0);

        vec3 n0 = normals[i0];
        vec3 n1 = normals[i1];
        vec3 n2 = normals[i2];

        bool o0 = length(n0) == 0.0f;
        bool o1 = length(n1) == 0.0f;
        bool o2 = length(n2) == 0.0f;

        if(o0 || o1 || o2) {
            vec3 normal = normalize(cross(v0.position - v2.position, v1.position - v2.position));

            if(o0) n0 = normal;
            if(o1) n1 = normal;
            if(o2) n2 = normal;
        }

        vertex.position = v0.position;
        vertex.normal = n0;
        mesh.vs.push_back(vertex);
        
        vertex.position = v1.position;
        vertex.normal = n1;
        mesh.vs.push_back(vertex);
        
        vertex.position = v2.position;
        vertex.normal = n2;
        mesh.vs.push_back(vertex);
    };
    
    mesh.load();
};

void create_mesh(axiom::color_mesh3d& mesh, std::vector<std::vector<axiom::vertex_element3d>>& elements, vec3 color) {
    for(int j = 0; j < elements.size(); ++j) {
        std::vector<axiom::output_vertex> surface_vertices;
        std::vector<uint> surface_indices;
        axiom::create_mesh(elements[j], &surface_vertices, &surface_indices);

        std::unordered_map<uint, vec3> normals;

        for(uint i = 0; i < surface_indices.size(); i += 3) {
            uint i0 = surface_indices[i];
            uint i1 = surface_indices[i + 1];
            uint i2 = surface_indices[i + 2];

            axiom::output_vertex v0 = surface_vertices[i0];
            axiom::output_vertex v1 = surface_vertices[i1];
            axiom::output_vertex v2 = surface_vertices[i2];

            vec3 scaled_normal = cross(v0.position - v2.position, v1.position - v2.position);

            if(v0.element != v1.element || v1.element != v2.element || v2.element != v0.element) {
                i0 |= 0x80000000;
                i1 |= 0x80000000;
                i2 |= 0x80000000;
            }
            
            if(!normals.contains(i0)) normals[i0] = vec3(0.0f);
            if(!normals.contains(i1)) normals[i1] = vec3(0.0f);
            if(!normals.contains(i2)) normals[i2] = vec3(0.0f);

            if(!(elements[j][v0.element].radii.x + elements[j][v0.element].radii.y == 0.0f || 
            elements[j][v0.element].radii.y + elements[j][v0.element].radii.z == 0.0f || 
            elements[j][v0.element].radii.z + elements[j][v0.element].radii.x == 0.0f)) {
                normals[i0] += scaled_normal;
            }

            if(!(elements[j][v1.element].radii.x + elements[j][v1.element].radii.y == 0.0f || 
            elements[j][v1.element].radii.y + elements[j][v1.element].radii.z == 0.0f || 
            elements[j][v1.element].radii.z + elements[j][v1.element].radii.x == 0.0f)) {
                normals[i1] += scaled_normal;
            }

            if(!(elements[j][v2.element].radii.x + elements[j][v2.element].radii.y == 0.0f || 
            elements[j][v2.element].radii.y + elements[j][v2.element].radii.z == 0.0f || 
            elements[j][v2.element].radii.z + elements[j][v2.element].radii.x == 0.0f)) {
                normals[i2] += scaled_normal;
            }

            //
        };

        for(auto& [key, normal] : normals) {
            float len = length(normal);
            if(len != 0.0f) normal /= len;
        }

        for(uint i = 0; i < surface_indices.size(); i += 3) {
            uint i0 = surface_indices[i];
            uint i1 = surface_indices[i + 1];
            uint i2 = surface_indices[i + 2];

            axiom::output_vertex v0 = surface_vertices[i0];
            axiom::output_vertex v1 = surface_vertices[i1];
            axiom::output_vertex v2 = surface_vertices[i2];
            
            if(v0.element != v1.element || v0.element != v2.element || v1.element != v2.element) {
                i0 |= 0x80000000;
                i1 |= 0x80000000;
                i2 |= 0x80000000;
            }
            
            axiom::color_vertex3d vertex;
            vertex.color = vec4(color, 1.0);

            vec3 n0 = normals[i0];
            vec3 n1 = normals[i1];
            vec3 n2 = normals[i2];

            bool o0 = length(n0) == 0.0f;
            bool o1 = length(n1) == 0.0f;
            bool o2 = length(n2) == 0.0f;

            if(o0 || o1 || o2) {
                vec3 normal = normalize(cross(v0.position - v2.position, v1.position - v2.position));

                if(o0) n0 = normal;
                if(o1) n1 = normal;
                if(o2) n2 = normal;
            }

            vertex.position = v0.position;
            vertex.normal = n0;
            mesh.vs.push_back(vertex);
            
            vertex.position = v1.position;
            vertex.normal = n1;
            mesh.vs.push_back(vertex);
            
            vertex.position = v2.position;
            vertex.normal = n2;
            mesh.vs.push_back(vertex);
        };
    }
    
    mesh.load();
};

void create_collider(axiom::collider3d& collider, axiom::transform3d& transform, std::vector<std::vector<axiom::vertex_element3d>>& elements, std::vector<float> masses, bool is_static = false) {
    for(int i = 0; i < elements.size(); ++i) {
        axiom::collision_shape3d shape;
        shape.elements = elements[i];
        collider.shapes.push_back(shape);
    }
    
    collider.is_static = is_static;
    collider.allow_rotation = true;

    vec3 offset = axiom::initialize_collider(collider, masses);
    transform.position += transform.orientation * offset;
    axiom::create_bounding_box(collider);

    for(auto& ee : elements) for(auto& element : ee) element.center -= offset;

    for(int i = 0; i < elements.size(); ++i) if(collider.shapes[i].elements.size() >= 3) collider.shapes[i].faces = get_faces(collider.shapes[i].elements);
};

void create_cube(vec3 position, mat3 orientation, float diameter, vec3 color, float mass = 1.0f) {
    axiom::transform3d transform;
    axiom::color_mesh3d mesh;
    axiom::collider3d collider;
    
    std::vector<axiom::vertex_element3d> elements = {
        axiom::vertex_element3d{vec3(-1.0f, -1.0f, -1.0f) * 0.5f / axiom::sqrt3 * diameter},
        axiom::vertex_element3d{vec3(1.0f, -1.0f, -1.0f) * 0.5f / axiom::sqrt3 * diameter},
        axiom::vertex_element3d{vec3(-1.0f, 1.0f, -1.0f) * 0.5f / axiom::sqrt3 * diameter},
        axiom::vertex_element3d{vec3(1.0f, 1.0f, -1.0f) * 0.5f / axiom::sqrt3 * diameter},
        axiom::vertex_element3d{vec3(-1.0f, -1.0f, 1.0f) * 0.5f / axiom::sqrt3 * diameter},
        axiom::vertex_element3d{vec3(1.0f, -1.0f, 1.0f) * 0.5f / axiom::sqrt3 * diameter},
        axiom::vertex_element3d{vec3(-1.0f, 1.0f, 1.0f) * 0.5f / axiom::sqrt3 * diameter},
        axiom::vertex_element3d{vec3(1.0f, 1.0f, 1.0f) * 0.5f / axiom::sqrt3 * diameter},
    };

    transform.position = position;
    transform.orientation = orientation;

    create_mesh(mesh, elements, color);
    create_collider(collider, transform, elements, mass);

    uint entity = axiom::ecs.insert_entity();
    axiom::ecs.insert_component(entity, transform);
    axiom::ecs.insert_component(entity, mesh);
    axiom::ecs.insert_component(entity, collider);
}


void create_cuboid(vec3 position, mat3 orientation, vec3 axes, vec3 color, float mass = 1.0f) {
    axiom::transform3d transform;
    axiom::color_mesh3d mesh;
    axiom::collider3d collider;
    
    std::vector<axiom::vertex_element3d> elements = {
        axiom::vertex_element3d{vec3(-1.0f, -1.0f, -1.0f) * 0.5f * axes},
        axiom::vertex_element3d{vec3(1.0f, -1.0f, -1.0f) * 0.5f * axes},
        axiom::vertex_element3d{vec3(-1.0f, 1.0f, -1.0f) * 0.5f * axes},
        axiom::vertex_element3d{vec3(1.0f, 1.0f, -1.0f) * 0.5f * axes},
        axiom::vertex_element3d{vec3(-1.0f, -1.0f, 1.0f) * 0.5f * axes},
        axiom::vertex_element3d{vec3(1.0f, -1.0f, 1.0f) * 0.5f * axes},
        axiom::vertex_element3d{vec3(-1.0f, 1.0f, 1.0f) * 0.5f * axes},
        axiom::vertex_element3d{vec3(1.0f, 1.0f, 1.0f) * 0.5f * axes},
    };

    transform.position = position;
    transform.orientation = orientation;

    create_mesh(mesh, elements, color);
    create_collider(collider, transform, elements, mass, mass == 0.0f);

    uint entity = axiom::ecs.insert_entity();
    axiom::ecs.insert_component(entity, transform);
    axiom::ecs.insert_component(entity, mesh);
    axiom::ecs.insert_component(entity, collider);
}

void create_tetrahedron(vec3 position, mat3 orientation, float diameter, vec3 color, float mass = 1.0f) {
    axiom::transform3d transform;
    axiom::color_mesh3d mesh;
    axiom::collider3d collider;
    uint entity = axiom::ecs.insert_entity();

    std::vector<axiom::vertex_element3d> elements = {
        axiom::vertex_element3d{vec3(1.0f, -1.0f, -1.0f) / axiom::sqrt3 * 0.5f * diameter},
        axiom::vertex_element3d{vec3(-1.0f, 1.0f, -1.0f) / axiom::sqrt3 * 0.5f * diameter},
        axiom::vertex_element3d{vec3(-1.0f, -1.0f, 1.0f) / axiom::sqrt3 * 0.5f * diameter},
        axiom::vertex_element3d{vec3(1.0f, 1.0f, 1.0f) / axiom::sqrt3 * 0.5f * diameter},
    };

    transform.position = position;
    transform.orientation = orientation;

    create_mesh(mesh, elements, color);
    create_collider(collider, transform, elements, mass, mass == 0.0f);

    axiom::ecs.insert_component(entity, transform);
    axiom::ecs.insert_component(entity, mesh);
    axiom::ecs.insert_component(entity, collider);
}

void create_bipyramid(vec3 position, mat3 orientation, float diameter, int num_vertices, vec3 color, float mass = 1.0f) {
    axiom::transform3d transform;
    axiom::color_mesh3d mesh;
    axiom::collider3d collider;
    
    std::vector<axiom::vertex_element3d> elements = {
        axiom::vertex_element3d{vec3(0.0f, 0.0f, 1.0f) * 0.5f * diameter},
        axiom::vertex_element3d{vec3(0.0f, 0.0f, -1.0f) * 0.5f * diameter},
    };

    for(int i = 0; i < num_vertices; ++i) {
        float angle = axiom::pi * 2.0f * (float(i) / num_vertices);
        
        auto element = axiom::vertex_element3d{vec3(cos(angle), sin(angle), 0) * 0.5f * diameter};
        elements.push_back(element);
    }

    transform.position = position;
    transform.orientation = orientation;

    create_mesh(mesh, elements, color);
    create_collider(collider, transform, elements, mass, mass == 0.0f);
    
    uint entity = axiom::ecs.insert_entity();
    axiom::ecs.insert_component(entity, transform);
    axiom::ecs.insert_component(entity, mesh);
    axiom::ecs.insert_component(entity, collider);
}

void create_dodecahedron(vec3 position, mat3 orientation, float diameter, vec3 color, float mass = 1.0f) {
    axiom::transform3d transform;
    axiom::color_mesh3d mesh;
    axiom::collider3d collider;

    float phi = (1.0f + sqrt(5.0f)) / 2.0f;

    std::vector<vec3> vs = {
        vec3(-1, -1, -1),
        vec3(1, -1, -1),
        vec3(-1, 1, -1),
        vec3(1, 1, -1),
        vec3(-1, -1, 1),
        vec3(1, -1, 1),
        vec3(-1, 1, 1),
        vec3(1, 1, 1),
        
        vec3(0, -phi, -1.0f / phi),
        vec3(0, phi, -1.0f / phi),
        vec3(0, -phi, 1.0f / phi),
        vec3(0, phi, 1.0f / phi),
        
        vec3(-1.0f / phi, 0, -phi),
        vec3(1.0f / phi, 0, -phi),
        vec3(-1.0f / phi, 0, phi),
        vec3(1.0f / phi, 0, phi),

        vec3(-phi, -1.0f / phi, 0),
        vec3(phi, -1.0f / phi, 0),
        vec3(-phi, 1.0f / phi, 0),
        vec3(phi, 1.0f / phi, 0),
    };

    std::vector<axiom::vertex_element3d> elements;
    for(vec3 v : vs) elements.push_back(axiom::vertex_element3d{v / axiom::sqrt3 * diameter * 0.5f});

    transform.position = position;
    transform.orientation = orientation;

    create_mesh(mesh, elements, color);
    create_collider(collider, transform, elements, mass, mass == 0.0f);
    
    uint entity = axiom::ecs.insert_entity();
    axiom::ecs.insert_component(entity, transform);
    axiom::ecs.insert_component(entity, mesh);
    axiom::ecs.insert_component(entity, collider);
}

void create_icosahedron(vec3 position, mat3 orientation, float diameter, vec3 color, float mass = 1.0f) {
    axiom::transform3d transform;
    axiom::color_mesh3d mesh;
    axiom::collider3d collider;

    float phi = (1.0f + sqrt(5.0f)) / 2.0f;

    std::vector<vec3> vs = {
        vec3(0.0f, -1.0f, -phi),
        vec3(0.0f, 1.0f, -phi),
        vec3(0.0f, -1.0f, phi),
        vec3(0.0f, 1.0f, phi),

        vec3(-1.0f, -phi, 0.0f),
        vec3(1.0f, -phi, 0.0f),
        vec3(-1.0f, phi, 0.0f),
        vec3(1.0f, phi, 0.0f),

        vec3(-phi, 0.0f, -1.0f),
        vec3(phi, 0.0f, -1.0f),
        vec3(-phi, 0.0f, 1.0f),
        vec3(phi, 0.0f, 1.0f),
    };

    float rad = sqrt(1.0f + phi * phi);

    std::vector<axiom::vertex_element3d> elements;
    for(vec3 v : vs) elements.push_back(axiom::vertex_element3d{v / rad * 0.5f * diameter});

    transform.position = position;
    transform.orientation = orientation;

    create_mesh(mesh, elements, color);
    create_collider(collider, transform, elements, mass, mass == 0.0f);
    
    uint entity = axiom::ecs.insert_entity();
    axiom::ecs.insert_component(entity, transform);
    axiom::ecs.insert_component(entity, mesh);
    axiom::ecs.insert_component(entity, collider);
}