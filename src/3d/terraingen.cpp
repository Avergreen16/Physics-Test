#include "terraingen.hpp"

#include <include/core.hpp>
#include <include/physics3d.hpp>

#include "stb_image.h"
#include "stb_image_write.h"

std::vector<mat3> get_orientations() {
    std::vector<mat3> ret = {
        mat3{vec3(0, 1, 0), vec3(0, 0, 1), vec3(1, 0, 0)},
        mat3{vec3(0, -1, 0), vec3(0, 0, 1), vec3(-1, 0, 0)},
        mat3{vec3(-1, 0, 0), vec3(0, 0, 1), vec3(0, 1, 0)},
        mat3{vec3(1, 0, 0), vec3(0, 0, 1), vec3(0, -1, 0)},
        mat3{vec3(0, 1, 0), vec3(-1, 0, 0), vec3(0, 0, 1)},
        mat3{vec3(0, 1, 0), vec3(1, 0, 0), vec3(0, 0, -1)},
    };

    return ret;
}

int terrain_generator::get_index(ivec3 pos) {
    return pos.z * size * size + pos.y * size + pos.x;
}

void terrain_generator::init() {
    terrain_pixels.resize(6 * size * size);

    for(int f = 0; f < 6; ++f) {
        for(int y = 0; y < size; ++y) {
            for(int x = 0; x < size; ++x) {
                terrain_pixel px;
                px.elevation = 0.0f;

                terrain_pixels[get_index({x, y, f})] = px;
            }
        }
    }
}

float phi = (1.0f + sqrt(5.0f)) / 2;

float line_line(vec3 origin0, vec3 dir0, vec3 origin1, vec3 dir1) {
    vec3 W = origin0 - origin1;
    float a = dot(W, dir0);
    float b = dot(W, dir1);
    float c = dot(dir0, dir1);
    float d = dot(dir0, dir0);
    float e = dot(dir1, dir1);

    float denom = (d * e - c * c);
    float s = (b * c - a * e) / denom;
    float t = (b * d - c * a) / denom;

    vec3 ps = origin0 + dir0 * s;
    vec3 pt = origin1 + dir1 * t;

    return length(ps - pt);
}

float line_ray(vec3 origin_line, vec3 dir_line, vec3 origin_plane, vec3 normal_plane) {
    vec3 rel_origin = origin_line - origin_plane;

    float dot_norm = dot(dir_line, normal_plane);
    float dot_origin = dot(rel_origin, normal_plane);

    return -dot_origin / dot_norm;
}

axiom::texture_asset terrain_generator::render() {
    axiom::random32 rand(0xFF1);

    struct voronoi_cell {
        vec3 position;
        uint plate;
    };

    struct tectonic_plate {
        vec3 origin;
        vec3 rotation;
        float weight;
    };

    std::vector<voronoi_cell> cells;
    std::vector<tectonic_plate> plates;
    
    uint num_cells = 300;
    uint num_plates = 12;

    for(int i = 0; i < num_plates; ++i) {
        tectonic_plate plate;

        plate.origin = rand.unit_vector();
        plate.weight = rand() * 0.5 + 0.5;
        plate.rotation = rand.unit_vector();

        plates.push_back(plate);
    }

    for(int i = 0; i < num_cells; ++i) {
        voronoi_cell cell;

        float f = float(i + 1) / float(num_cells + 1);

        float golden_angle = 2.0f * axiom::pi / (phi * phi);

        float angle = golden_angle * i;
        float h = f * 2.0f - 1.0f;
        float w = sqrt(1.0f - h * h);

        vec3 pos = vec3(cos(angle) * w, sin(angle) * w, h);
        //pos += rand.unit_vector() * (4.0f * axiom::pi) * (float)sqrt(1.0f / num_cells) * 0.25f;
        pos = normalize(pos);

        cell.position = pos;

        float min_dist = axiom::max_float;
        int plate_id = 0;
        for(int j = 0; j < num_plates; ++j) {
            tectonic_plate& plate = plates[j];

            float current_dist = length(plate.origin - pos);

            if(current_dist < min_dist) {
                min_dist = current_dist;
                plate_id = j;
            }
        }

        cell.plate = plate_id;

        cells.push_back(cell);
    }

    std::vector<uint8_t> img(size * 3 * size * 4 * 4, 0);
    std::vector<ivec2> positions = {
        ivec2(1, 1),
        ivec2(3, 1),
        ivec2(2, 1),
        ivec2(0, 1),
        ivec2(1, 2),
        ivec2(1, 0),
    };

    std::vector<vec3> vs;
    std::vector<uint> is;
    for(voronoi_cell& cell : cells) {
        vs.push_back(cell.position);
    }

    axiom::triangulate(vs, &is);

    std::unordered_map<uint, std::vector<uint>> vvs;
    std::vector<vec3> centers;

    for(uint i = 0; i < is.size() / 3; ++i) {
        uint i0 = is[i * 3];
        uint i1 = is[i * 3 + 1];
        uint i2 = is[i * 3 + 2];

        uint e0 = (glm::max(i0, i1) << 16) | glm::min(i0, i1); 
        uint e1 = (glm::max(i1, i2) << 16) | glm::min(i1, i2);
        uint e2 = (glm::max(i2, i0) << 16) | glm::min(i2, i0);

        vvs[e0].push_back(i);
        vvs[e1].push_back(i);
        vvs[e2].push_back(i);

        centers.push_back(normalize((vs[i0] + vs[i1] + vs[i2]) / 3.0f));
    }

    float scale = (4.0f * axiom::pi) * sqrt(1.0f / num_plates);

    struct plate_node {
        vec3 position;
        float convergence = 0.0f;
        float divergent_time = 0.0f;

        std::vector<uint> neighbors;
        std::vector<uint> plates;
    };
    std::vector<plate_node> nodes;
    std::unordered_map<uint, uint> node_mapping;
    std::vector<uint> connections;
    
    for(auto& [i, vs] : vvs) {
        uint e0 = i & 0xFFFF;
        uint e1 = i >> 16;

        voronoi_cell& cell0 = cells[e0];
        voronoi_cell& cell1 = cells[e1];

        if(cell0.plate != cell1.plate) {
            uint i0 = vs[0];
            uint i1 = vs[1];

            if(!node_mapping.contains(i0)) {
                node_mapping.emplace(i0, nodes.size());

                nodes.push_back(plate_node(centers[i0]));
            }
            
            if(!node_mapping.contains(i1)) {
                node_mapping.emplace(i1, nodes.size());
                
                nodes.push_back(plate_node(centers[i1]));
            }

            uint connection = (glm::max(i0, i1) << 16 | glm::min(i0, i1));
            connections.push_back(connection);

            nodes[node_mapping[i0]].neighbors.push_back(i1);
            nodes[node_mapping[i1]].neighbors.push_back(i0);

            nodes[node_mapping[i0]].plates.push_back(e0);
            nodes[node_mapping[i0]].plates.push_back(e1);
            
            nodes[node_mapping[i1]].plates.push_back(e0);
            nodes[node_mapping[i1]].plates.push_back(e1);
        }
    }

    for(plate_node& node : nodes) {
        for(uint plate_i : node.plates) {
            tectonic_plate& plate = plates[plate_i];

            vec3 dir = normalize(node.position - plate.origin);
            vec3 move = cross(node.position, plate.rotation);

            node.convergence += dot(move, dir);
        }
        
        float divergent_time = abs(axiom::noise_gen::perlin_noise(node.position, scale, 1, 0xF1F));
        if(node.convergence < 0.0f) node.divergent_time = divergent_time * 12.5f;
        else node.divergent_time = 1.0f;

        mat3 rot = axiom::rotate_to(normalize(node.position), vec3(0.0f, 0.0f, 1.0f));

        vec2 start_dir = (rot * (nodes[node_mapping[node.neighbors[0]]].position - node.position)).xy();
        float start_angle = std::atan2(start_dir.y, start_dir.x);

        std::sort(node.neighbors.begin(), node.neighbors.end(), 
            [&](const uint& i0, const uint& i1) {
                plate_node& node0 = nodes[node_mapping[i0]];
                plate_node& node1 = nodes[node_mapping[i1]];

                vec3 o0 = node0.position - node.position;
                vec3 o1 = node1.position - node.position;

                vec2 oo0 = (rot * o0).xy();
                vec2 oo1 = (rot * o1).xy();

                float angle0 = std::atan2(oo0.y, oo0.x);
                float angle1 = std::atan2(oo1.y, oo1.x);

                angle0 = glm::mod(angle0 - start_angle, axiom::pi * 2.0f);
                angle1 = glm::mod(angle1 - start_angle, axiom::pi * 2.0f);

                return angle0 < angle1;
            }
        );


        std::sort(node.plates.begin(), node.plates.end());
        auto iter = std::unique(node.plates.begin(), node.plates.end());
        node.plates.erase(iter, node.plates.end());
        
        std::sort(node.plates.begin(), node.plates.end(), 
            [&](const uint& i0, const uint& i1) {
                voronoi_cell& cell0 = cells[i0];
                voronoi_cell& cell1 = cells[i1];

                vec3 o0 = cell0.position - node.position;
                vec3 o1 = cell1.position - node.position;

                vec2 oo0 = (rot * o0).xy();
                vec2 oo1 = (rot * o1).xy();

                float angle0 = std::atan2(oo0.y, oo0.x);
                float angle1 = std::atan2(oo1.y, oo1.x);

                angle0 = glm::mod(angle0 - start_angle, axiom::pi * 2.0f);
                angle1 = glm::mod(angle1 - start_angle, axiom::pi * 2.0f);

                return angle0 < angle1;
            }
        );

        /*
        for(int i = 0; i < neighbors.size(); ++i) {
            uint i0 = i;
            uint i1 = (i + 1) % neighbors.size();

            auto& n0 = nodes[node_mapping[i0]];
            auto& n1 = nodes[node_mapping[i1]];

            vec3 vec_n0 = n0.position - node.position;
            vec3 vec_n1 = n1.position - node.position;
        }
        */
    }

    auto sample_convergence = [&](vec3 pos) {
        float convergence = 0.0f;
        float divergence = 0.0f;

        for(uint connection : connections) {
            uint n0 = connection & 0xFFFF;
            uint n1 = connection >> 16;

            auto& node0 = nodes[node_mapping[n0]];
            auto& node1 = nodes[node_mapping[n1]];

            //
            vec3 v0 = node0.position;
            vec3 v1 = node1.position;

            vec3 normal = normalize(v0 + v1);

            vec3 proj_pos = pos - node0.position;
            if(dot(pos, normal) > 0.0f) {
                proj_pos = proj_pos - normal * dot(proj_pos, normal) + v0;

                float len01 = length(v1 - v0);
                vec3 norm01 = normalize(v1 - v0);

                float frac = glm::clamp(dot(proj_pos - v0, norm01), 0.0f, len01);

                vec3 diff = norm01 * frac + v0 - proj_pos;
                float ff = frac / len01;

                //
                
                vec3 direction = vec3(1.0f, 0.0f, 0.0f);
                
                vec3 vel0a = vec3(0.0f);
                vec3 vel0b = vec3(0.0f);
                vec3 vel1a = vec3(0.0f);
                vec3 vel1b = vec3(0.0f);

                //std::cout << node0.plates.size() << " " << cells[node1.plates.size() << " ";

                for(int i = 0; i < node0.neighbors.size(); ++i) {
                    if(node0.neighbors[i] == n1) {
                        if(node0.neighbors.size() == 2) {
                            /*

                            auto& plate0 = plates[cells[node0.plates[i]].plate];
                            auto& plate1 = plates[cells[node0.plates[i + 1]].plate];

                            vec3 v0 = cross(node0.position, plate0.rotation);
                            vec3 v1 = cross(node0.position, plate1.rotation);

                            vec3 vv = v0 - v1;

                            vel0a = vv;
                            vel0b = -vv;

                            */

                            std::vector<uint> plates = node0.plates;

                            if(plates.size() == 3) {
                                if(dot(cross(nodes[node_mapping[node0.neighbors[0]]].position - node0.position, nodes[node_mapping[node0.neighbors[1]]].position - node0.position), node0.position) < 0.0f) {
                                    plates = {plates[2], plates[0], plates[1], plates[2]};
                                } else plates = {plates[2], plates[0], plates[0], plates[1]};
                            } else {
                                plates = {plates[3], plates[0], plates[1], plates[2]};
                            }

                            vec3 pos_a = cells[plates[i * 2 + 1]].position;
                            vec3 pos_b = cells[plates[(i * 2 + 2) % plates.size()]].position;

                            vec3 avg = normalize(pos_a - node0.position) + normalize(pos_b - node0.position);
                            if(dot(avg, cross(node0.position, node1.position - node0.position)) < 0.0f) avg = -avg;
                            
                            //vel0a = normalize(pos_a - node0.position);
                            //vel0b = normalize(pos_b - node0.position);
                            vel0a = avg;
                        } else {
                            {
                                auto* plate0 = &plates[cells[node0.plates[(i - 2) % node0.plates.size()]].plate];
                                auto* plate1 = &plates[cells[node0.plates[(i - 1) % node0.plates.size()]].plate];
                                auto* plate2 = &plates[cells[node0.plates[i]].plate];

                                vec3 v0 = cross(node0.position, plate0->rotation);
                                vec3 v1 = cross(node0.position, plate1->rotation);
                                vec3 v2 = cross(node0.position, plate2->rotation);

                                vel0a = cells[node0.plates[(i - 1) % node0.plates.size()]].position - node0.position;//(v0 - v1) + (v2 - v1);
                            }

                            {
                                auto* plate0 = &plates[cells[node0.plates[(i - 1) % node0.plates.size()]].plate];
                                auto* plate1 = &plates[cells[node0.plates[i]].plate];
                                auto* plate2 = &plates[cells[node0.plates[(i + 1) % node0.plates.size()]].plate];

                                vec3 v0 = cross(node0.position, plate0->rotation);
                                vec3 v1 = cross(node0.position, plate1->rotation);
                                vec3 v2 = cross(node0.position, plate2->rotation);

                                vel0b = cells[node0.plates[i]].position - node0.position;//(v0 - v1) + (v2 - v1);
                            }
                        }
                    }
                }
                
                for(int i = 0; i < node1.neighbors.size(); ++i) {
                    if(node1.neighbors[i] == n0) {
                        if(node1.neighbors.size() == 2) {

                            /*
                            auto& plate0 = plates[cells[node1.plates[i]].plate];
                            auto& plate1 = plates[cells[node1.plates[i + 1]].plate];

                            vec3 v0 = cross(node1.position, plate0.rotation);
                            vec3 v1 = cross(node1.position, plate1.rotation);

                            vec3 vv = v0 - v1;

                            vel1a = vv;
                            vel1b = -vv;
                            */
                            
                            std::vector<uint> plates = node1.plates;

                            if(plates.size() == 3) {
                                if(dot(cross(nodes[node_mapping[node1.neighbors[0]]].position - node1.position, nodes[node_mapping[node1.neighbors[1]]].position - node1.position), node1.position) < 0.0f) {
                                    plates = {plates[2], plates[0], plates[1], plates[2]};
                                } else plates = {plates[2], plates[0], plates[0], plates[1]};
                            } else {
                                plates = {plates[3], plates[0], plates[1], plates[2]};
                            }

                            vec3 pos_a = cells[plates[i * 2 + 1]].position;
                            vec3 pos_b = cells[plates[(i * 2 + 2) % plates.size()]].position;

                            vec3 avg = normalize(pos_a - node1.position) + normalize(pos_b - node1.position);
                            if(dot(avg, cross(node1.position, node0.position - node1.position)) < 0.0f) avg = -avg;

                            vel1a = avg;
                            
                            //vel1a = normalize(pos_a - node1.position);
                            //vel1b = normalize(pos_b - node1.position);
                        } else {
                            {
                                auto* plate0 = &plates[cells[node1.plates[((i - 2) % node1.plates.size())]].plate];
                                auto* plate1 = &plates[cells[node1.plates[((i - 1) % node1.plates.size())]].plate];
                                auto* plate2 = &plates[cells[node1.plates[i]].plate];

                                vec3 v0 = cross(node1.position, plate0->rotation);
                                vec3 v1 = cross(node1.position, plate1->rotation);
                                vec3 v2 = cross(node1.position, plate2->rotation);

                                vel1a = cells[node1.plates[(i - 1) % node1.plates.size()]].position - node1.position;//(v0 - v1) + (v2 - v1);
                            }

                            {

                                auto* plate0 = &plates[cells[node1.plates[(i - 1) % node1.plates.size()]].plate];
                                auto* plate1 = &plates[cells[node1.plates[i]].plate];
                                auto* plate2 = &plates[cells[node1.plates[(i + 1) % node1.plates.size()]].plate];

                                vec3 v0 = cross(node1.position, plate0->rotation);
                                vec3 v1 = cross(node1.position, plate1->rotation);
                                vec3 v2 = cross(node1.position, plate2->rotation);

                                vel1b = cells[node1.plates[i]].position - node1.position;//(v0 - v1) + (v2 - v1);
                            }
                        }
                    }
                }

                /*
                if(dot(vel0a, vel1a) < 0.0f) vel1a = -vel1a;
                if(dot(vel0b, vel1b) < 0.0f) vel1b = -vel1b;
                if(dot(vel0a, vel0b) > 0.0f) {
                    vel0b = -vel0b;
                    vel1b = -vel1b;
                }
                */

                if(length(vel0a) != 0.0f) vel0a = normalize(vel0a);
                if(length(vel0b) != 0.0f) vel0b = normalize(vel0b);
                if(length(vel1a) != 0.0f) vel1a = normalize(vel1a);
                if(length(vel1b) != 0.0f) vel1b = normalize(vel1b);

                mat3 rel_matrix = axiom::rotate_to(normal, vec3(0.0f, 0.0f, 1.0f));

                vec2 tv0 = (rel_matrix * v0).xy();
                vec2 tv1 = (rel_matrix * v1).xy();
                vec2 tpos = (rel_matrix * pos).xy();

                vec2 tvel0a = (rel_matrix * vel0a).xy();
                vec2 tvel0b = (rel_matrix * vel0b).xy();
                vec2 tvel1a = (rel_matrix * vel1a).xy();
                vec2 tvel1b = (rel_matrix * vel1b).xy();


                vec2 tvel0;
                vec2 tvel1;

                vec2 norm_tn = vec2(tv0.y - tv1.y, -(tv0.x - tv1.x));
                if(dot(norm_tn, tvel0a) < 0.0f) norm_tn = -norm_tn;

                if(dot(norm_tn, tpos - tv1) > 0.0f) {
                    tvel0 = tvel0a;
                    tvel1 = tvel1a;
                } else {
                    tvel0 = tvel0b;
                    tvel1 = tvel1b;
                }
                
                vec2 norm_tvel0 = vec2(tvel0.y, -tvel0.x);
                vec2 norm_tvel1 = vec2(tvel1.y, -tvel1.x);

                float dist = 1000.0f;
                if((dot(norm_tvel0, tpos - tv0) < 0) != (dot(norm_tvel1, tpos - tv1) < 0)) {
                    vec2 ca = tv0;
                    vec2 cb = tv1;
                    vec2 cc = tv0 + tvel0 * 0.15f;
                    vec2 cd = tv1 + tvel1 * 0.15f;

                    float denom = (ca.x - cc.x) * (cb.y - cc.y) - (ca.y - cc.y) * (cb.x - cc.x);
                    float bca0 = ((ca.x - cc.x) * (tpos.y - cc.y) - (ca.y - cc.y) * (tpos.x - cc.x)) / denom;
                    float bcb0 = ((tpos.x - cc.x) * (cb.y - cc.y) - (tpos.y - cc.y) * (cb.x - cc.x)) / denom;
                    float bcc0 = 1.0f - bca0 - bcb0;
                    float bcd0 = 0.0f;

                    if(bca0 < 0.0f || bcb0 < 0.0f || bcc0 < 0.0f) {
                        denom = (cd.x - cc.x) * (cb.y - cc.y) - (cd.y - cc.y) * (cb.x - cc.x);
                        bca0 = 0.0f;
                        bcd0 = ((tpos.x - cc.x) * (cb.y - cc.y) - (tpos.y - cc.y) * (cb.x - cc.x)) / denom;
                        bcb0 = ((cd.x - cc.x) * (tpos.y - cc.y) - (cd.y - cc.y) * (tpos.x - cc.x)) / denom;
                        bcc0 = 1.0f - bcb0 - bcd0;
                        if(bcd0 < 0.0f || bcb0 < 0.0f || bcc0 < 0.0f) {

                        } else {
                            dist = 1.0f - bcb0;
                        }
                    } else {
                        dist = bcc0;
                    }
                }

                frac /= len01;
                float conv = 1.0f;//glm::mix(node0.convergence, node1.convergence, frac) / 3.0f;// * factor;

                /*
                if(length(diff) < 0.01f) {
                    if(ff < 0.5f) {
                        if(node0.neighbors.size() == 2) divergence = 10.0f;
                        else convergence = 10.0f;
                    } else {
                        if(node1.neighbors.size() == 2) divergence = 10.0f;
                        else convergence = 10.0f;
                    }
                }
                */
                if(length(diff) < 0.005f) {
                    divergence = 10.0f;
                }

                float len = 0.05f;

                if(node0.neighbors.size()) {
                    vec3 n = normalize(vel0a - normalize(node0.position) * dot(vel0a, normalize(node0.position)));


                    vec3 npos = pos - normalize(node0.position) * dot(pos - node0.position, normalize(node0.position));

                    vec3 offset = npos - node0.position;
                    float closest_pt = dot(offset, n);
                    if(length(node0.position + closest_pt * n - npos) < 0.005 && closest_pt > 0.0f && closest_pt < len) {
                        convergence = 10.0f;
                    }
                    
                    n = normalize(vel0b - normalize(node0.position) * dot(vel0b, normalize(node0.position)));

                    npos = pos - normalize(node0.position) * dot(pos - node0.position, normalize(node0.position));

                    offset = npos - node0.position;
                    closest_pt = dot(offset, n);
                    if(length(node0.position + closest_pt * n - npos) < 0.005 && closest_pt > 0.0f && closest_pt < len) {
                        convergence = 10.0f;
                    }
                }

                if(convergence == 10.0f) divergence = 0.0f;
                
                if(node1.neighbors.size()) {
                    vec3 n = normalize(vel1a - normalize(node1.position) * dot(vel1a, normalize(node1.position)));


                    vec3 npos = pos - normalize(node1.position) * dot(pos - node1.position, normalize(node1.position));

                    vec3 offset = npos - node1.position;
                    float closest_pt = dot(offset, n);
                    if(length(node1.position + closest_pt * n - npos) < 0.005 && closest_pt > 0.0f && closest_pt < len) {
                        convergence = 10.0f;
                    }
                    
                    n = normalize(vel1b - normalize(node1.position) * dot(vel1b, normalize(node1.position)));

                    npos = pos - normalize(node1.position) * dot(pos - node1.position, normalize(node1.position));

                    offset = npos - node1.position;
                    closest_pt = dot(offset, n);
                    if(length(node1.position + closest_pt * n - npos) < 0.005 && closest_pt > 0.0f && closest_pt < len) {
                        convergence = 10.0f;
                    }
                }
                
                /*
                if(node0.neighbors.size() == 3 || node1.neighbors.size() == 3) {
                    if(conv > 0.0f) {
                        conv = glm::max(0.0f, conv);

                        float factor = glm::smoothstep(abs(conv), 0.0f, dist) * glm::abs(conv);
                        convergence = glm::max(factor, convergence);
                    } else {
                        conv = glm::min(0.0f, conv);

                        float factor = glm::smoothstep(abs(conv), 0.0f, dist) * glm::abs(conv);
                        divergence = glm::max(factor, divergence);
                    }
                }
                */
            }
        }

        return std::vector<float>{convergence, divergence};
    };
    
    for(plate_node& node : nodes) {
        auto convs = sample_convergence(node.position);

        if(node.convergence > 0.0f && convs[1] != 0.0f) node.convergence = 0.0f;//glm::max(0.0f, node.convergence - convs[1]);
    }

    for(int f = 0; f < 6; ++f) {
        std::vector<uint8_t> face(size * size * 4);
        for(int y = 0; y < size; ++y) {
            for(int x = 0; x < size; ++x) {
                terrain_pixel& px = terrain_pixels[get_index(ivec3(x, y, f))];
                vec3 pos = vec3(float(x + 0.5f) / size * 2.0f - 1.0f, float(y + 0.5f) / size * 2.0f - 1.0f, 1.0f);
                pos = orientations[f] * pos;

                vec3 offset = {axiom::noise_gen::perlin_noise(pos, scale * 0.05f, 3, 0xFF5), axiom::noise_gen::perlin_noise(pos, scale * 0.05f, 3, 0xF61), axiom::noise_gen::perlin_noise(pos, scale * 0.05f, 3, 0x105)};
                //pos += offset * 0.1f;
                
                pos = normalize(pos);

                auto convs = sample_convergence(pos);

                /*
                convs[0] = 0.0f;

                vec3 direction = vec3(0.0f, 1.0f, 0.0f);
                vec3 v0 = vec3(1.0f, 0.0f, 0.0f);
                vec3 v1 = vec3(0.0f, 0.0f, 1.0f);
                vec3 plane_origin = v0;
                vec3 plane_normal = cross(plane_origin, normalize(v1 - v0));
                float dist = line_ray(pos, direction, plane_origin, plane_normal);
                float value = glm::clamp(1.0f - abs(dist) * 5.0f, 0.0f, 1.0f);
                
                convs[1] = value;
                */

                float scaled_diff = convs[0] - convs[1];
                vec4 color;
                if(scaled_diff > 0.0f) {
                    color = glm::mix(vec4(0.0f, 0.0f, 1.0f, 1.0f), vec4(1.0f, 0.0f, 0.0f, 1.0f), glm::clamp(abs(scaled_diff), 0.0f, 1.0f));
                } else {
                    color = glm::mix(vec4(0.0f, 0.0f, 1.0f, 1.0f), vec4(1.0f, 1.0f, 0.0f, 1.0f), glm::clamp(abs(scaled_diff), 0.0f, 1.0f));
                }

                face[(y * size + x) * 4] = color.x * 0xFF;
                face[(y * size + x) * 4 + 1] = color.y * 0xFF;
                face[(y * size + x) * 4 + 2] = color.z * 0xFF;
                face[(y * size + x) * 4 + 3] = color.w * 0xFF;
            }
        }

        ivec2 rel_pos = positions[f] * size;
        
        for(int y = 0; y < size; ++y) {
            for(int x = 0; x < size; ++x) {
                int src = y * size + x;
                int dst = (rel_pos.y + y) * size * 4 + (rel_pos.x + x);

                memcpy(&img[dst * 4], &face[src * 4], 4 * sizeof(uint8_t));
            }
        }
    }

    return axiom::texture_asset::load(img, {size * 4, size * 3}, 4);
    
    std::string filepath = "output/planet" + std::to_string(axiom::get_timestamp()) + ".png";
    stbi_flip_vertically_on_write(true);
    stbi_write_png(filepath.c_str(), size * 4, size * 3, 4, img.data(), size * 4 * 4);

    return {};
}