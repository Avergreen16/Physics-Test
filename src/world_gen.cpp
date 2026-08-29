#include <utilities.hpp>
#include <render.hpp>
#include <physics-3d.hpp>

#include "world_gen.hpp"

#include <iostream>

//

axiom::texture& get_texture(std::string name);

//

mat4 get_matrix(vec3 y, vec3 z, vec3 origin) {
    mat3 m = mat3(glm::cross(y, z), y, z);

    return glm::translate(origin) * mat4(m);
}

void create_planet(uint seed, vec3 position, mat3 orientation, vec3 dimensions, std::vector<crater_population> populations, std::vector<vec3> colors, float amplitude, float noise_freq, float noise_offset, float age_value, float ejecta_value, float blend_value, uint num_chunks, uint num_tiles) {
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
            r = bias_func(r, 0.6f);

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
        float n0 = 1.4f + axiom::noise_gen::perlin_noise(pos, 0.45f, 3, seed, 0.5075f) * 1.5f;

        vec3 forward = vec3(1, 0, 0);
        float d = glm::smoothstep(0.0f, 1.0f, dot(normalize(pos), forward) * 2.0f - 0.725f);
        n0 -= d;
        n0 = glm::smoothstep(0.0f, 1.0f, n0) - 0.5f;

        n0 += axiom::noise_gen::perlin_noise(pos, 0.1f, 2.0f, seed) * 0.025f;

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

                float variation = 0.35f;

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
                        if(c.radius > 0.05f) ret = big_crater_func(dist, -c.radius * 0.125f, 1.75f, 1.75f, 0.5f, 3.0f, 0.5f);
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
        axiom::collider3d collider;
        collider.allow_rotation = false;
        collider.allow_gravity = false;
        collider.is_static = true;

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

            v0.texture = vec2(dot(tex_x, v0.position), dot(tex_y, v0.position)) * 48.0f;
            v1.texture = vec2(dot(tex_x, v1.position), dot(tex_y, v1.position)) * 48.0f;
            v2.texture = vec2(dot(tex_x, v2.position), dot(tex_y, v2.position)) * 48.0f;

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
        t.position = planet_ori * mesh_avg;
        t.orientation = planet_ori;

        //t.position = apply_matrix(planet_ori, t.position);
        t.position += position;

        axiom::texture_range_mesh3d mesh;
        mesh.vs = mesh_vertices;
        mesh.load();
        mesh.texture = &get_texture("tilesheet");
        
        //axiom::create_mesh_collider(collider, triangles);
        //collider.is_static = true;

        //

        uint entity = axiom::insert_entity();
        axiom::insert_component(entity, t);
        axiom::insert_component(entity, mesh);
        //axiom::insert_component(entity, collider);   
        
        //axiom::collider3d& ccl = ecs.get_component<axiom::collider3d>(entity);
        //axiom::transform3d& tf = ecs.get_component<axiom::transform3d>(entity);

        //axiom::physics_system3d& ps = ecs.get_system<axiom::physics_system3d>();
        //ps.create_bounding_box(ccl, tf);
        //ccl.create_BVH(2, &tf);
    };

    
    double setup_time = axiom::get_time();

    int split = num_chunks;
    float size = 1.0f;
    uint tiles_per_split = num_tiles;

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
    
    float width = size * 2.0f / split;
    float tiles = tiles_per_split;
    float diff = width / tiles * glm::max(glm::max(dimensions.x, dimensions.y), dimensions.z);
    
    double end_time = axiom::get_time();

    std::cout << "setup: " << setup_time - start_time << "\n";
    std::cout << "loop: " << end_time - setup_time << "\n";
    std::cout << "total: " << end_time - start_time << "\n\n";
};