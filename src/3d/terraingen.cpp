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

std::vector<mat2> get_rotations() {
    
}

int terrain_generator::get_index(ivec3 pos) {
    return pos.z * size * size + pos.y * size + pos.x;
}

float cross_2d(vec2 a, vec2 b) {
    return a.x * b.y - a.y * b.x;
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

    //

    orientations = get_orientations();

    edges = {
        3, 2, 5, 4,
        2, 3, 5, 4,
        0, 1, 5, 4,
        1, 0, 5, 4,
        3, 2, 0, 1,
        3, 2, 1, 0
    };

    std::vector<mat2> ret(24);

    for(uint i = 0; i < 24; ++i) {
        ivec2 edge = ivec2(i / 4, edges[i]);

        mat3 src_edge = orientations[edge.x];
        mat3 dst_edge = orientations[edge.y];

        mat3 rot_to = axiom::rotate_to(src_edge[2], dst_edge[2]);
        mat3 r = rot_to * src_edge;


        vec2 x_vec = vec2(dot(dst_edge[0], r[0]), dot(dst_edge[1], r[0]));
        vec2 y_vec = vec2(-dot(dst_edge[1], r[0]), dot(dst_edge[0], r[0]));

        mat2 new_orientation = mat2(x_vec, y_vec);

        ret[i] = new_orientation;
    }

    rotations = ret;
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

float orient(vec2 a, vec2 b, vec2 c) {
    return cross_2d(b - a, c - a);
}

bool segments_intersect(vec2 a, vec2 b, vec2 c, vec2 d) {
    float o1 = orient(a, b, c);
    float o2 = orient(a, b, d);
    float o3 = orient(c, d, a);
    float o4 = orient(c, d, b);

    return (o1 * o2 < 0.0f) && (o3 * o4 < 0.0f);
}

int is_hourglass(vec2 a, vec2 b, vec2 c, vec2 d) {
    if(segments_intersect(a, b, c, d)) return 1;
    if(segments_intersect(b, c, d, a)) return 2;
    return 0;       
}

vec2 intersect(vec2 v1, vec2 v2, vec2 v3, vec2 v4, float& t) {
    t = ((v1.x - v3.x) * (v3.y - v4.y) - (v1.y - v3.y) * (v3.x - v4.x)) / ((v1.x - v2.x) * (v3.y - v4.y) - (v1.y - v2.y) * (v3.x - v4.x));

    return v1 + t * (v2 - v1);
}

vec3 barycentric(vec2 a, vec2 b, vec2 c, vec2 d) {
    float denom = cross_2d(a - c, b - c);

    float v0 = cross_2d(d - c, b - c) / denom;
    float v1 = cross_2d(a - c, d - c) / denom;
    float v2 = 1.0f - v0 - v1;

    return vec3{v0, v1, v2};
};

vec4 mean_value(vec2 a, vec2 b, vec2 c, vec2 d, vec2 e) {
    vec2 diff_a = a - e;
    vec2 diff_b = b - e;
    vec2 diff_c = c - e;
    vec2 diff_d = d - e;

    float ra = length(a - e);
    float rb = length(b - e);
    float rc = length(c - e);
    float rd = length(d - e);

    float tab = cross_2d(diff_a, diff_b) / (ra * rb + dot(diff_a, diff_b));
    float tbc = cross_2d(diff_b, diff_c) / (rb * rc + dot(diff_b, diff_c));
    float tcd = cross_2d(diff_c, diff_d) / (rc * rd + dot(diff_c, diff_d));
    float tda = cross_2d(diff_d, diff_a) / (rd * ra + dot(diff_d, diff_a));

    float wa = (tda + tab) / ra;
    float wb = (tab + tbc) / rb;
    float wc = (tbc + tcd) / rc;
    float wd = (tcd + tda) / rd;

    float sum = wa + wb + wc + wd;

    return vec4(wa / sum, wb / sum, wc / sum, wd / sum);
};

vec4 interpolate(vec2 v1, vec2 v2, vec2 v3, vec2 v4, vec2 e) {
    return mean_value(v1, v2, v3, v4, e);
}

vec2 newton(vec2 a, vec2 b, vec2 c, vec2 d, vec2 p) {
    vec2 guess = vec2(0.25f, 0.25f);

    for(int i = 0; i < 12; ++i) {
        vec2 q = glm::mix(glm::mix(a, b, guess.x), glm::mix(c, d, guess.x), guess.y);
        vec2 error = q - p;

        vec2 dQdu = (1 - guess.y) * (b - a) + guess.y * (d - c);
        vec2 dQdv = (1 - guess.x) * (c - a) + guess.x * (d - b);

        float det = cross_2d(dQdu, dQdv);

        if(std::abs(det) < 1e-8) break;

        float du = cross_2d(error, dQdv) / det;
        float dv = cross_2d(dQdu, error) / det;

        guess.x -= du;
        guess.y -= dv;
    }

    return guess;
}

axiom::texture_asset terrain_generator::render() {
    size = 128;
    axiom::random32 rand(0x1B);

    struct voronoi_cell {
        vec3 position;
        uint plate;

        bool edge = false;
        std::vector<uint> neighbors;
    };

    struct tectonic_plate {
        vec3 origin;
        vec3 craton;
        vec3 rotation;
        float weight;
        float continental;
    };

    struct mountain_range {
        uint origin;
        vec3 direction;
        float weight;
    };

    std::vector<voronoi_cell> cells;
    std::vector<tectonic_plate> plates;
    std::vector<mountain_range> ranges;
    
    uint num_cells = 500;
    uint num_plates = 25;

    uint num_ranges = 5;

    for(int i = 0; i < num_plates; ++i) {
        tectonic_plate plate;

        plate.origin = rand.unit_vector();
        plate.weight = rand() * 0.6 + 0.4;
        plate.rotation = rand.unit_vector();
        plate.continental = rand() - 0.2f;
        plate.craton = normalize(plate.origin + rand.unit_vector() * plate.weight * 0.35f);

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
        pos += rand.unit_vector() * (4.0f * axiom::pi) * (float)sqrt(1.0f / num_cells) * 0.125f;
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

        std::vector<uint> ns2;
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

        cell0.neighbors.push_back(e1);
        cell1.neighbors.push_back(e0);

        uint i0 = vs[0];
        uint i1 = vs[1];

        //
        
        if(!node_mapping.contains(i0)) {
            node_mapping.emplace(i0, nodes.size());

            nodes.push_back(plate_node(centers[i0]));
        }
        
        if(!node_mapping.contains(i1)) {
            node_mapping.emplace(i1, nodes.size());
            
            nodes.push_back(plate_node(centers[i1]));
        }
        
        nodes[node_mapping[i0]].ns2.push_back(i1);
        nodes[node_mapping[i1]].ns2.push_back(i0);
        
        nodes[node_mapping[i0]].plates.push_back(e0);
        nodes[node_mapping[i0]].plates.push_back(e1);
        
        nodes[node_mapping[i1]].plates.push_back(e0);
        nodes[node_mapping[i1]].plates.push_back(e1);

        if(cell0.plate != cell1.plate) {
            uint connection = (glm::max(i0, i1) << 16 | glm::min(i0, i1));
            connections.push_back(connection);
            
            nodes[node_mapping[i0]].neighbors.push_back(i1);
            nodes[node_mapping[i1]].neighbors.push_back(i0);
        }
    }

    for(plate_node& node : nodes) {
        std::set<uint> p;
        for(uint plate_i : node.plates) {
            uint pp = cells[plate_i].plate;
            if(!p.contains(pp)) {
                tectonic_plate& plate = plates[cells[plate_i].plate];

                vec3 dir = normalize(node.position - plate.origin);
                vec3 move = cross(node.position, plate.rotation);

                node.convergence += dot(move, dir);
            }
            p.emplace(pp);
        }
        
        float divergent_time = abs(axiom::noise_gen::perlin_noise(node.position, scale, 1, 0xF1F));
        if(node.convergence < 0.0f) node.divergent_time = divergent_time * 12.5f;
        else node.divergent_time = 1.0f;

        mat3 rot = axiom::rotate_to(normalize(node.position), vec3(0.0f, 0.0f, 1.0f));

        std::sort(node.plates.begin(), node.plates.end());
        auto iter = std::unique(node.plates.begin(), node.plates.end());
        node.plates.erase(iter, node.plates.end());

        if(node.neighbors.size()) {
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
        }

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

    struct tectonic_quad {
        float conv0;
        float conv1;
        vec3 v0;
        vec3 v1;
        vec3 d0;
        vec3 d1;
    };

    std::vector<tectonic_quad> quads;
    
    for(uint i = 0; i < 50; ++i) {
        uint origin;

        while(true) {
            uint r = rand.next() % node_mapping.size();

            auto& node = nodes[node_mapping[r]];
            
            bool is_edge = false;
            
            origin = r;

            break;
        }

        vec3 direction = rand.unit_vector();
        direction = normalize(direction - nodes[node_mapping[origin]].position * dot(direction, nodes[node_mapping[origin]].position));
        
        vec3 direction2 = rand.unit_vector();
        direction2 = normalize(direction2 - nodes[node_mapping[origin]].position * dot(direction2, nodes[node_mapping[origin]].position));

        uint prev = origin;

        for(int i = 0; i < 20; ++i) {
            uint next;

            float max_dot = -axiom::max_float;
            uint n;
            for(uint n2 : nodes[node_mapping[prev]].ns2) {
                vec3 dir = nodes[node_mapping[n2]].position - nodes[node_mapping[prev]].position;

                float d = dot(direction2, dir);

                if(d > max_dot) {
                    max_dot = d;
                    n = n2;
                }
            }

            vec3 v0 = nodes[node_mapping[prev]].position;
            vec3 v1 = nodes[node_mapping[n]].position;

            prev = n;

            tectonic_quad quad;
            quad.conv0 = 1.0f - abs((float(i) / 21) - 0.5) * 2.0f + 0.001f;
            quad.conv1 = 1.0f - abs((float(i + 1) / 21) - 0.5) * 2.0f + 0.001f;

            std::cout << quad.conv0 << " " << quad.conv1 << "\n";
 
            quad.v0 = v0;
            quad.v1 = v1;
            quad.d0 = normalize(direction - v0 * dot(v0, direction));
            quad.d1 = normalize(direction - v1 * dot(v1, direction));

            quads.push_back(quad);
        }
    }

    uint flag = 0;

    for(uint connection : connections) {
        uint n0 = connection & 0xFFFF;
        uint n1 = connection >> 16;

        auto& node0 = nodes[node_mapping[n0]];
        auto& node1 = nodes[node_mapping[n1]];

        //
        vec3 v0 = node0.position;
        vec3 v1 = node1.position;

        vec3 normal = normalize(v0 + v1);

        float len01 = length(v1 - v0);
        vec3 norm01 = normalize(v1 - v0);

        //
        
        vec3 vel0a = vec3(0.0f);
        vec3 vel0b = vec3(0.0f);
        vec3 vel1a = vec3(0.0f);
        vec3 vel1b = vec3(0.0f);

        for(int i = 0; i < node0.neighbors.size(); ++i) {
            if(node0.neighbors[i] == n1) {
                if(node0.neighbors.size() == 2) {
                    std::vector<uint> nplates = node0.plates;

                    if(nplates.size() == 3) {
                        if(dot(cross(nodes[node_mapping[node0.neighbors[0]]].position - node0.position, nodes[node_mapping[node0.neighbors[1]]].position - node0.position), node0.position) < 0.0f) {
                            nplates = {nplates[2], nplates[0], nplates[1], nplates[2]};
                        } else nplates = {nplates[2], nplates[0], nplates[0], nplates[1]};
                    } else {
                        nplates = {nplates[3], nplates[0], nplates[1], nplates[2]};
                    }

                    auto& plate0 = plates[cells[nplates[i * 2]].plate];
                    auto& plate1 = plates[cells[nplates[i * 2 + 1]].plate];

                    vec3 v0 = cross(node0.position, plate0.rotation);
                    vec3 v1 = cross(node0.position, plate1.rotation);

                    vec3 vv = normalize(v0 - v1);

                    vel0a = vv;
                    vel0b = -vv;
                } else {
                    {
                        /*
                        auto& cell0 = cells[node0.plates[i]];
                        auto& cell1 = cells[node0.plates[(i + 1) % node0.plates.size()]];
                        auto& cell2 = cells[node0.plates[(i + 2) % node0.plates.size()]];
                        
                        vel0a = normalize(cell2.position - node0.position);
                        vel0b = normalize(cell0.position - node0.position);
                        */

                        auto& plate0 = plates[cells[node0.plates[0]].plate];
                        auto& plate1 = plates[cells[node0.plates[1]].plate];
                        auto& plate2 = plates[cells[node0.plates[2]].plate];

                        vec3 v0 = cross(node1.position, plate0.rotation);
                        vec3 v1 = cross(node1.position, plate1.rotation);
                        vec3 v2 = cross(node1.position, plate2.rotation);

                        vec3 vv0 = v2 - v0;
                        vec3 vv1 = v0 - v1;
                        vec3 vv2 = v1 - v2;

                        if(dot(vv0, vv1) < 0.0f) vv1 = -vv1;
                        if(dot(vv0, vv2) < 0.0f) vv2 = -vv2;

                        vec3 vv = normalize(vv0 + vv1 + vv2);

                        vel0a = vv;
                        vel0b = -vv;
                    }
                }
            }
        }
        
        for(int i = 0; i < node1.neighbors.size(); ++i) {
            if(node1.neighbors[i] == n0) {
                if(node1.neighbors.size() == 2) {
                    std::vector<uint> nplates = node1.plates;

                    if(nplates.size() == 3) {
                        if(dot(cross(nodes[node_mapping[node1.neighbors[0]]].position - node1.position, nodes[node_mapping[node1.neighbors[1]]].position - node1.position), node1.position) < 0.0f) {
                            nplates = {nplates[2], nplates[0], nplates[1], nplates[2]};
                        } else nplates = {nplates[2], nplates[0], nplates[0], nplates[1]};
                    } else {
                        nplates = {nplates[3], nplates[0], nplates[1], nplates[2]};
                    }

                    auto& plate0 = plates[cells[nplates[i * 2]].plate];
                    auto& plate1 = plates[cells[nplates[i * 2 + 1]].plate];

                    vec3 v0 = cross(node1.position, plate0.rotation);
                    vec3 v1 = cross(node1.position, plate1.rotation);

                    vec3 vv = normalize(v0 - v1);

                    vel1a = -vv;
                    vel1b = vv;
                } else {
                    {
                        /*
                        auto& cell0 = cells[node1.plates[i]];
                        auto& cell1 = cells[node1.plates[(i + 1) % node1.plates.size()]];
                        auto& cell2 = cells[node1.plates[(i + 2) % node1.plates.size()]];
                        
                        vel1a = normalize(cell0.position - node1.position);
                        vel1b = normalize(cell2.position - node1.position);
                        */

                        auto& plate0 = plates[cells[node1.plates[0]].plate];
                        auto& plate1 = plates[cells[node1.plates[1]].plate];
                        auto& plate2 = plates[cells[node1.plates[2]].plate];

                        vec3 v0 = cross(node1.position, plate0.rotation);
                        vec3 v1 = cross(node1.position, plate1.rotation);
                        vec3 v2 = cross(node1.position, plate2.rotation);

                        vec3 vv0 = v2 - v0;
                        vec3 vv1 = v0 - v1;
                        vec3 vv2 = v1 - v2;

                        if(dot(vv0, vv1) < 0.0f) vv1 = -vv1;
                        if(dot(vv0, vv2) < 0.0f) vv2 = -vv2;

                        vec3 vv = normalize(vv0 + vv1 + vv2);

                        vel1a = vv;
                        vel1b = -vv;
                    }
                }
            }
        }

        if(node0.neighbors.size() == 3) {
            if(dot(vel1a, vel0a) < 0.0f || dot(vel1b, vel0b) < 0.0f) {
                vel0a = -vel0a;
                vel0b = -vel0b;
            }
        }
        
        if(node1.neighbors.size() == 3) {
            if(dot(vel1a, vel0a) < 0.0f || dot(vel1b, vel0b) < 0.0f) {
                vel1a = -vel1a;
                vel1b = -vel1b;
            }
        }
        
        tectonic_quad quad;
        quad.conv0 = node0.convergence;
        quad.conv1 = node1.convergence;

        quad.v0 = v0;
        quad.v1 = v1;
        quad.d0 = normalize(vel0a - v0 * dot(v0, vel0a));
        quad.d1 = normalize(vel1a - v1 * dot(v1, vel1a));

        quads.push_back(quad);
    }

    auto sample_elev = [&](vec3 pos) {
        flag = 0;

        float convergence = 0.0f;
        float divergence = 0.0f;

        for(auto& quad : quads) {
            vec3 v0 = quad.v0;
            vec3 v1 = quad.v1;
            vec3 vel0 = quad.d0;
            vec3 vel1 = quad.d1;

            vec3 normal = normalize(v0 + v1);

            if(dot(normal, pos) < 0.0f) continue;

            mat3 rel_matrix = axiom::rotate_to(normal, vec3(0.0f, 0.0f, 1.0f));

            vec2 tv0 = (rel_matrix * v0).xy();
            vec2 tv1 = (rel_matrix * v1).xy();
            vec2 tpos = (rel_matrix * pos).xy();

            vec2 tvel0 = (rel_matrix * vel0).xy();
            vec2 tvel1 = (rel_matrix * vel1).xy();

            //
            
            float w = 1.0f;

            auto is_include = [](vec2 a, vec2 b, vec2 c, vec2 d) {
                vec3 v = barycentric(a, b, c, d);
                return !(v.x < 0.0f || v.y < 0.0f || v.z < 0.0f);
            };

            vec2 va = tv0 + tvel0 * w;
            vec2 vb = tv0 - tvel0 * w;
            vec2 vc = tv1 + tvel1 * w;
            vec2 vd = tv1 - tvel1 * w;

            float dist = 1000.0f;
            float conv = 0.0f;

            {
                vec4 is = mean_value(va, vb, vd, vc, tpos);
                if(is.x >= -0.015f && is.y >= -0.015f && is.z >= -0.015f && is.w >= -0.015f) {
                    float v1 = is.x + is.w;
                    float v2 = is.y + is.z;

                    dist = glm::min(dist, abs((v1 - v2) * 2.0f));
                    
                    float v3 = is.x + is.y;
                    float v4 = is.z + is.w;

                    conv = quad.conv0 * v3 + quad.conv1 * v4;
                }
            }
            
            if(divergence != 10.0f) {
                if(conv > 0.0f) {
                    conv = glm::max(0.0f, conv);

                    float factor = glm::smoothstep(abs(conv), 0.0f, dist * 5.0f) * glm::abs(conv);
                    convergence = glm::max(factor, convergence);
                } else {
                    conv = glm::min(0.0f, conv);

                    float factor = glm::smoothstep(abs(conv), 0.0f, dist) * glm::abs(conv);
                    divergence = glm::max(factor, divergence);
                }
            }
        }

        //

        float min_dist = axiom::max_float;
        uint min_plate = 0;

        for(auto& cell : cells) {
            float dist = length(cell.position - pos);
            
            if(dist < min_dist) {
                min_dist = dist;
                min_plate = cell.plate;
            }
        }

        auto& plate = plates[min_plate];

        vec3 offset = {axiom::noise_gen::perlin_noise(pos, 0.5f, 3, 0xFF5), axiom::noise_gen::perlin_noise(pos, 0.5f, 3, 0xF61), axiom::noise_gen::perlin_noise(pos, 0.5f, 3, 0x105)};
        
        float continental = -0.5f;//axiom::noise_gen::perlin_noise(pos + offset * 0.75f, 0.5, 4, 0x15) * 1.0f - 0.5f;
        
        if(plate.continental > 0.0f) {
            vec3 diff = pos - plate.craton;
            float dist = length(diff) / (plate.weight * 0.85f) + axiom::noise_gen::perlin_noise(pos, 0.35, 7, 0x15) * 4.0f;

            continental += glm::mix(1.0f, 0.0f, glm::clamp(dist - 1.0f, 0.0f, 1.0f));
        }

        float convergence_factor = glm::sign(continental);

        return continental + convergence * convergence_factor - divergence;
    };

    std::vector<std::vector<float>> faces(6);

    for(int f = 0; f < 6; ++f) {
        std::vector<float>& face = faces[f];
        face.resize(size * size);
        for(int y = 0; y < size; ++y) {
            for(int x = 0; x < size; ++x) {
                terrain_pixel& px = terrain_pixels[get_index(ivec3(x, y, f))];
                vec3 pos = vec3(float(x + 0.5f) / size * 2.0f - 1.0f, float(y + 0.5f) / size * 2.0f - 1.0f, 1.0f);
                pos = orientations[f] * pos;

                vec3 offset = {axiom::noise_gen::perlin_noise(pos, scale * 0.05f, 3, 0xFF5), axiom::noise_gen::perlin_noise(pos, scale * 0.05f, 3, 0xF61), axiom::noise_gen::perlin_noise(pos, scale * 0.05f, 3, 0x105)};
                
                pos = normalize(pos);
                pos += offset * 0.033f;

                auto elev = sample_elev(pos);

                face[y * size + x] = elev;
            }
        }
    }
    
    std::vector<std::vector<float>> src_faces = faces;

    int kernel_size = 3;
    std::vector<float> kernel(kernel_size * kernel_size);
    for(int yy = -(kernel_size / 2); yy <= (kernel_size / 2); ++yy) {
        for(int xx = -(kernel_size / 2); xx <= (kernel_size / 2); ++xx) {
            vec2 pos = vec2(xx, yy);

            ivec2 kpos = {xx + (kernel_size / 2), yy + (kernel_size / 2)};

            kernel[kpos.y * kernel_size + kpos.x] = 1.0f / (kernel_size * kernel_size);
        }
    }
    
    for(int f = 0; f < 6; ++f) {
        for(int y = 0; y < size; ++y) {
            for(int x = 0; x < size; ++x) {
                ivec2 pos = {x, y};

                float v = 0.0f;

                for(int yy = -(kernel_size / 2); yy <= (kernel_size / 2); ++yy) {
                    for(int xx = -(kernel_size / 2); xx <= (kernel_size / 2); ++xx) {
                        ivec2 npos = {x + xx, y + yy};
                        ivec2 kpos = {xx + (kernel_size / 2), yy + (kernel_size / 2)};

                        float sum = 0.0f;
                        float accum = 0.0f;

                        if(npos.x < 0) {
                            ivec2 nnpos = npos;
                            nnpos.y = glm::clamp(nnpos.y, 0, size - 1);

                            vec2 nn = (nnpos + size) % size;
                            nn += 0.5f;
                            nn -= size * 0.5f;

                            uint dst = edges[f * 4];

                            mat2 rotation = rotations[f * 4];
                            nnpos = rotation * nn;
                            nnpos += size * 0.5f;

                            sum += src_faces[dst][nnpos.y * size + nnpos.x];

                            accum += 1.0f;
                        } else if(npos.x >= size) {
                            ivec2 nnpos = npos;
                            nnpos.y = glm::clamp(nnpos.y, 0, size - 1);
                            
                            vec2 nn = (nnpos + size) % size;
                            nn += 0.5f;
                            nn -= size * 0.5f;

                            uint dst = edges[f * 4 + 1];

                            mat2 rotation = rotations[f * 4 + 1];
                            nnpos = rotation * nn;
                            nnpos += size * 0.5f;

                            sum += src_faces[dst][nnpos.y * size + nnpos.x];
                            
                            accum += 1.0f;
                        }
                        
                        if(npos.y < 0) {
                            ivec2 nnpos = npos;
                            nnpos.x = glm::clamp(nnpos.x, 0, size - 1);
                            
                            vec2 nn = (nnpos + size) % size;
                            nn += 0.5f;
                            nn -= size * 0.5f;

                            uint dst = edges[f * 4 + 2];

                            mat2 rotation = rotations[f * 4 + 2];
                            nnpos = rotation * nn;
                            nnpos += size * 0.5f;

                            sum += src_faces[dst][nnpos.y * size + nnpos.x];

                            accum += 1.0f;
                        } else if(npos.y >= size) {
                            ivec2 nnpos = npos;
                            nnpos.x = glm::clamp(nnpos.x, 0, size - 1);
                            
                            vec2 nn = (nnpos + size) % size;
                            nn += 0.5f;
                            nn -= size * 0.5f;

                            uint dst = edges[f * 4 + 3];

                            mat2 rotation = rotations[f * 4 + 3];
                            nnpos = rotation * nn;
                            nnpos += size * 0.5f;

                            sum += src_faces[dst][nnpos.y * size + nnpos.x];

                            accum += 1.0f;
                        }

                        if(accum == 0.0f) {
                            accum = 1.0f;
                            sum = src_faces[f][npos.y * size + npos.x];
                        }

                        sum /= accum;

                        v += sum * kernel[kpos.y * (kernel_size / 2) + kpos.x];
                    }
                }

                faces[f][pos.y * size + pos.x] = v;
            }
        }
    }

    std::vector<std::vector<uint8_t>> texture_faces(6);

    for(int f = 0; f < 6; ++f) {
        std::vector<uint8_t>& face = texture_faces[f];
        face.resize(size * size * 4);

        for(int y = 0; y < size; ++y) {
            for(int x = 0; x < size; ++x) {
                float elev = faces[f][y * size + x];

                float ocean_level = 0.25f;

                vec4 color;
                if(elev > 1.0f) color = glm::mix(vec4(1.0f, 0.5f, 1.0f, 1.0f), vec4(1.0f, 1.0f, 1.0f, 1.0f), glm::clamp(elev - 1.0f, 0.0f, 1.0f));
                else if(elev > ocean_level) color = glm::mix(vec4(1.0f, 0.0f, 1.0f, 1.0f), vec4(1.0f, 0.5f, 0.0f, 1.0f), glm::clamp((elev - ocean_level) / (1.0f - ocean_level), 0.0f, 1.0f));
                else color = glm::mix(vec4(0.0f, 0.5f, 1.0f, 1.0f), vec4(0.0f, 0.0f, 1.0f, 1.0f), glm::clamp(-(elev - ocean_level), 0.0f, 1.0f));

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