#include <physics-3d/collision.hpp>
#include <physics-3d/collider.hpp>
#include <physics-3d/bounding_box.hpp>
#include <include/math.hpp>
#include <include/utilities.hpp>

#include <iostream>

namespace axiom {

glm::vec3 triangle_project(glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3& point) {
    vec3 e0 = a - c;
    vec3 e1 = b - c;

    float v = dot(c, e0);
    float w = dot(c, e1);
    float x = dot(e0, e0);
    float y = dot(e1, e1);
    float z = dot(e0, e1);

    float denom = (x * y - z * z);
    float alpha = (w * z - v * y) / denom;
    float beta = (v * z - w * x) / denom;
    float gamma = 1.0f - alpha - beta;

    point = a * alpha + b * beta + c * gamma;

    return {alpha, beta, gamma};
}

vec2 line_project(vec3 a, vec3 b) {
    vec3 e = b - a;
    float beta = -dot(a, e) / dot(e, e);
    beta = glm::clamp(beta, 0.0f, 1.0f);

    return {1.0f - beta, beta};
}

uint simplex::find_closest_face(glm::vec3& weights, glm::vec3& dir) {
    float dist = axiom::max_float;
    uint f = -1;
    weights = glm::vec3(-1);

    for(int i = 0; i < 4; ++i) {
        glm::vec3 p;

        glm::vec3 w = triangle_project(vertices[(i <= 0) ? 1 : 0].m, vertices[(i <= 1) ? 2 : 1].m, vertices[(i <= 2) ? 3 : 2].m, p);

        if(w.x < 0.0f || w.y < 0.0f || w.z < 0.0f) {
            float length_p = glm::length(p);

            if(length_p < dist) {
                dir = glm::normalize(glm::cross(vertices[(i <= 0) ? 1 : 0].m - vertices[(i <= 2) ? 3 : 2].m, vertices[(i <= 1) ? 2 : 1].m - vertices[(i <= 2) ? 3 : 2].m));

                dist = length_p;
                f = i;
                weights = w;
            }
        }
    }

    return f;
}

void get_normal(glm::vec3& a, glm::vec3& b, glm::vec3& c, glm::vec3& dir_vertex, glm::vec3& output_normal, glm::vec3& output_centroid) {
    output_centroid = (a + b + c) / 3.0f;

    output_normal = glm::normalize(glm::cross(a - c, b - c));

    if(glm::dot(dir_vertex - output_centroid, output_normal) > 0.0f) output_normal = -output_normal;
}

int simplex::contains(glm::vec3 p, bool output) {
    glm::vec3 centroid;
    glm::vec3 normal;
    float dotp;

    bool v0 = false;
    bool v1 = false;
    bool v2 = false;

    float dotp0 = axiom::max_float;
    float dotp1 = axiom::max_float;
    float dotp2 = axiom::max_float;

    get_normal(vertices[1].m, vertices[2].m, vertices[3].m, vertices[0].m, normal, centroid);
    dotp = glm::dot(p - centroid, normal);
    
    if(dotp > 0.0f) {
        v0 = true;
        dotp0 = dotp;
    }

    get_normal(vertices[0].m, vertices[2].m, vertices[3].m, vertices[1].m, normal, centroid);
    dotp = glm::dot(p - centroid, normal);
    
    if(dotp > 0.0f) {
        v1 = true;
        dotp1 = dotp;
    }

    get_normal(vertices[0].m, vertices[1].m, vertices[3].m, vertices[2].m, normal, centroid);
    dotp = glm::dot(p - centroid, normal);
    
    if(dotp >= 0.0f) {
        v2 = true;
        dotp2 = dotp;
    }

    if(dotp0 != axiom::max_float && dotp0 < dotp1 && dotp0 < dotp2) {
        return 0;
    }
    if(dotp1 != axiom::max_float && dotp1 < dotp0 && dotp1 < dotp2) {
        return 1;
    }
    if(dotp2 != axiom::max_float && dotp2 < dotp0 && dotp2 < dotp1) {
        return 2;
    }

    return -1;
}

vec3 transform_vertices(std::vector<vertex_element3d>& elements, transform3d& transform, vec3 origin) {
    vec3 center = vec3(0.0f);

    for(auto& element : elements) {
        element.orientation = transform.orientation * element.orientation;
        element.center = transform.orientation * element.center + (transform.position - origin);

        center += element.center;
    }

    return center / float(elements.size());
}

polytope_return polytope::find_closest_face() {
    float dist = axiom::max_float;
    polytope_return ret;

    polytope_face* ret_face = nullptr;
    vec3 p;

    bool flip = false;

    for(polytope_face& f : faces) {
        glm::vec3 a = vertices[f.vertices[0]].m;
        glm::vec3 normal = f.normal;

        polytope_face* fface = &f;
        
        float d = -dot(-a, normal);
        if(d < dist) {
            ret_face = fface;
            dist = d;
        }
    }

    //ddd = dist;

    vec3 wv = triangle_project(vertices[ret_face->vertices[0]].m, vertices[ret_face->vertices[1]].m, vertices[ret_face->vertices[2]].m, p);
    ret.normal = ret_face->normal;
    ret.weights = wv;
    ret.vertices = {vertices[ret_face->vertices[0]], vertices[ret_face->vertices[1]], vertices[ret_face->vertices[2]]};

    return ret;
}

void polytope::insert_face(std::vector<uint> v) {
    glm::vec3 centroid = (vertices[v[0]].m + vertices[v[1]].m + vertices[v[2]].m) / 3.0f;
    glm::vec3 normal = glm::normalize(glm::cross(vertices[v[0]].m - vertices[v[2]].m, vertices[v[1]].m - vertices[v[2]].m));

    // error happened here (glitchy box)
    if(glm::dot(normal, (center / float(vertices.size())) - centroid) >= 0.0f) normal = -normal;

    faces.push_back(polytope_face(v, normal));
}

void polytope::from_simplex(simplex s) {
    vertices = {s.vertices[0], s.vertices[1], s.vertices[2], s.vertices[3]};

    center += s.vertices[0].m;
    center += s.vertices[1].m;
    center += s.vertices[2].m;
    center += s.vertices[3].m;

    insert_face({0, 1, 2});
    insert_face({0, 1, 3});
    insert_face({0, 2, 3});
    insert_face({1, 2, 3});
}

std::vector<uint> polytope::expand(simplex_vertex vertex) {
    uint v_n = vertices.size();
    vertices.push_back(vertex);
    center += vertex.m;

    float min_d = FLT_MAX;
    std::vector<uint64_t> edges;
    std::vector<uint> faces_seen;
    for(int i = 0; i < faces.size(); ++i) {
        glm::vec3 diff = vertex.m - vertices[faces[i].vertices[0]].m;

        if(glm::dot(faces[i].normal, diff) > 0.0f) {
            min_d = glm::min(glm::dot(faces[i].normal, diff), min_d);

            faces_seen.push_back(i);
        }
    }

    for(uint f : faces_seen) {
        polytope_face face = faces[f];
        uint64_t edge_a = (uint64_t(glm::min(face.vertices[0], face.vertices[1])) | (uint64_t(std::max(face.vertices[0], face.vertices[1])) << 32));
        uint64_t edge_b = (uint64_t(glm::min(face.vertices[1], face.vertices[2])) | (uint64_t(std::max(face.vertices[1], face.vertices[2])) << 32));
        uint64_t edge_c = (uint64_t(glm::min(face.vertices[0], face.vertices[2])) | (uint64_t(std::max(face.vertices[0], face.vertices[2])) << 32));

        edges.push_back(edge_a);
        edges.push_back(edge_b);
        edges.push_back(edge_c);
    }
    
    int i = 0;
    for(uint f : faces_seen) {
        faces.erase(faces.begin() + f - i);
        ++i;
    }

    for(uint64_t edge : edges) {
        if(std::count(edges.begin(), edges.end(), edge) == 1) {
            uint a = (edge & 0xFFFFFFFFull);
            uint b = (edge >> 32);
            
            insert_face({a, b, v_n});
        }
    }

    return faces_seen;
}

bool contains(std::vector<vertex_element3d>& elements, vec3 point) {
    axiom::simplex simplex;

    float limit = 0.0001;

    vec3 direction = glm::normalize(point - elements[0].center);
    
    vec3 offset = vec3(direction.y, -direction.x, direction.z);

    if(glm::dot(offset, direction) > 0.99) {
        offset = vec3(direction.x, direction.z, -direction.y);
    }

    direction = glm::normalize(direction + offset * 0.1f);

    int iterations = 0;

    bool loop = true;

    while(loop) {
        ++iterations;
        if(iterations > 100) return false;
        
        int size = simplex.vertices.size();
        if(size < 4) {
            vec3 point_a = support(direction, elements);
            vec3 point_b = point;

            vec3 point_m = point_a - point_b;

            for(simplex_vertex& v : simplex.vertices) {
                vec3 difference = point_m - v.m;

                if(glm::length(difference) < limit) {
                    return false;
                }
            }

            if(glm::dot(point_m, direction) < limit * 2) {
                return false;
            }

            simplex.vertices.push_back(simplex_vertex{point_m, point_a, point_b});

            if(size == 0) {
                direction = -glm::normalize(point_m);
            } else if(size == 1) {
                vec3 line_direction = glm::normalize(simplex.vertices[0].m - simplex.vertices[1].m);
                vec3 rel_origin_pos = -simplex.vertices[1].m;

                vec3 closest_point = line_direction * glm::dot(rel_origin_pos, line_direction) + simplex.vertices[1].m;
                direction = glm::normalize(-closest_point);
            } else if(size == 2) {
                vec3 center = (simplex.vertices[0].m + simplex.vertices[1].m + simplex.vertices[2].m) / 3.0f;
                vec3 normal = glm::normalize(glm::cross(simplex.vertices[0].m - simplex.vertices[2].m, simplex.vertices[1].m - simplex.vertices[2].m));
                if(glm::dot(normal, -center) <= 0.0) normal = -normal;

                direction = normal;
            }
        } else {
            int n = simplex.contains(vec3(0, 0, 0));
            if(n == -1) {
                return true;
            } else {
                simplex.vertices.erase(simplex.vertices.begin() + n);

                vec3 center = (simplex.vertices[0].m + simplex.vertices[1].m + simplex.vertices[2].m) / 3.0f;
                vec3 normal = glm::normalize(glm::cross(simplex.vertices[0].m - simplex.vertices[2].m, simplex.vertices[1].m - simplex.vertices[2].m));
                if(glm::dot(normal, -center) < 0.0) normal = -normal;

                direction = normal;
            }
        }
    }
}

std::vector<std::vector<vec3>> debug_vertices = {{}, {}};

//

std::vector<return_point> collide(transform3d& ta, collision_shape3d& ca, transform3d& tb, collision_shape3d& cb, return_tag& tag, collision_event& c_event) {
    std::vector<vertex_element3d> a_vertices = ca.elements;
    std::vector<vertex_element3d> b_vertices = cb.elements;

    float limit = 0.0001f;
    uint32_t iter_limit = 256;

    transform3d tta = ta;
    tta.position += tta.orientation * ca.position;
    tta.orientation = tta.orientation * ca.orientation;
    transform3d ttb = tb;
    ttb.position += ttb.orientation * cb.position;
    ttb.orientation = ttb.orientation * cb.orientation;

    vec3 a_rel_pos = transform_vertices(a_vertices, tta, ta.position);
    vec3 b_rel_pos = transform_vertices(b_vertices, ttb, ta.position);

    bool set = false;

    axiom::simplex simplex;

    //

    vec3 direction = glm::normalize(vec3(a_rel_pos - b_rel_pos));

    vec3 offset = vec3(direction.y, -direction.z, direction.x);

    if(abs(glm::dot(offset, direction)) > 0.95) {
        offset = vec3(direction.z, -direction.y, direction.x);
    }

    direction = glm::normalize(direction + offset * 0.1f);

    int iterations = 0;

    bool loop = true;

    auto get_normal = [&](collision_shape3d& c, mat3 ori, vec3 dir) -> shape_face* {
        float dd = -FLT_MAX;
        vec3 vec = dir;
        int32_t id = -1;

        uint32_t i = 0;
        for(shape_face& t : c.faces) {
            vec3 normal = ori * c.orientation * t.normal;

            float d = dot(normal, dir);

            if(d > dd) {
                vec = normal;
                dd = d;
                id = i;
            }

            ++i;
        }

        if(id >= 0) return &c.faces[id];
        else return nullptr;
    };
    
    float d = 0.0f;

    std::vector<uint32_t> selected;

    axiom::gjk_step gjk_step;
    axiom::epa_step epa_step;

    auto get_closest_points = [](std::vector<simplex_vertex> vs) -> std::vector<vec3> {
        if(vs.size() == 1) return {vs[0].a, vs[0].b};
        else if(vs.size() == 2) {
            vec2 w = line_project(vs[0].m, vs[1].m);
            
            return {vs[0].a * w.x + vs[1].a * w.y, vs[0].b * w.x + vs[1].b * w.y};
        } else {
            vec3 ret_p;
            vec3 w = triangle_project(vs[0].m, vs[1].m, vs[2].m, ret_p);
            vec3 a;
            vec3 b;

            if(w.x < 0.0f) {
                // b-c line
                vec2 w = line_project(vs[1].m, vs[2].m);

                a = vs[1].a * w.x + vs[2].a * w.y;
                b = vs[1].b * w.x + vs[2].b * w.y;
            } else if(w.y < 0.0f) {
                // a-c line
                vec2 w = line_project(vs[0].m, vs[2].m);
                
                a = vs[0].a * w.x + vs[2].a * w.y;
                b = vs[0].b * w.x + vs[2].b * w.y;
            } else if(w.z < 0.0f) {
                // a-b line
                vec2 w = line_project(vs[0].m, vs[1].m);

                a = vs[0].a * w.x + vs[1].a * w.y;
                b = vs[0].b * w.x + vs[1].b * w.y;
            } else {
                a = vs[0].a * w.x + vs[1].a * w.y + vs[2].a * w.z;
                b = vs[0].b * w.x + vs[1].b * w.y + vs[2].b * w.z;
            }
            
            return {a, b};
        }
    };

    while(loop) {
        ++iterations;
        
        int size = simplex.vertices.size();
        if(size < 4) {
            if(iterations > iter_limit) {
                std::cout << "ITER LIMIT\n";
                return {};
            }

            if(glm::isnan(direction.x)) {
                std::cout << "NAN DIRECTION" << size << "\n";
                direction = vec3(1, 0, 0);
            }

            uint32_t ia;
            uint32_t ib;

            vec3 point_a = support(direction, a_vertices);
            vec3 point_b = support(-direction, b_vertices);

            vec3 point_m = point_a - point_b;

            gjk_step.input_point = point_m;

            for(simplex_vertex& v : simplex.vertices) {
                vec3 difference = point_m - v.m;

                float dist = length(difference);

                if(dist == 0.0f) {
                    gjk_step.search_direction = vec3(0.0f);
                    
                    // finish
                    
                    auto ps = get_closest_points(simplex.vertices);
                    c_event.point_a = ps[0];
                    c_event.point_b = ps[1];
                    c_event.point_m = ps[0] - ps[1];
                    
                    return {};
                }
            }

            if(glm::dot(point_m, direction) <= limit) {
                gjk_step.search_direction = vec3(0.0f);
                
                simplex.vertices.push_back(simplex_vertex{point_m, point_a, point_b});

                if(simplex.vertices.size() == 4) {
                    int n = simplex.contains(vec3(0, 0, 0), iterations > iter_limit);
                    if(n != -1) {
                        gjk_step.erase_index = n;
                        simplex.vertices.erase(simplex.vertices.begin() + n);
                    }
                }

                c_event.gjk.push_back(gjk_step);
                
                auto ps = get_closest_points(simplex.vertices);
                c_event.point_a = ps[0];
                c_event.point_b = ps[1];
                c_event.point_m = ps[0] - ps[1];
                
                return {};
            }
            
            c_event.gjk.push_back(gjk_step);

            simplex.vertices.push_back(simplex_vertex{point_m, point_a, point_b});

            if(size == 0) {
                direction = -glm::normalize(point_m);

                gjk_step.search_direction = direction;
                gjk_step.search_origin = point_m;
            } else if(size == 1) {
                vec3 line_direction = normalize(simplex.vertices[0].m - simplex.vertices[1].m);
                vec3 rel_origin_pos = -simplex.vertices[1].m;

                vec3 closest_point = line_direction * glm::dot(rel_origin_pos, line_direction) + simplex.vertices[1].m;
                direction = glm::normalize(-closest_point);
                
                gjk_step.search_direction = direction;
                gjk_step.search_origin = closest_point;
            } else if(size == 2) {
                vec3 center = (simplex.vertices[0].m + simplex.vertices[1].m + simplex.vertices[2].m) / 3.0f;
                vec3 normal = glm::normalize(glm::cross(simplex.vertices[0].m - simplex.vertices[2].m, simplex.vertices[1].m - simplex.vertices[2].m));
                if(glm::dot(normal, -center) <= 0.0f) normal = -normal;
                d = glm::dot(normal, -center);

                direction = normal;
                
                gjk_step.search_direction = direction;
                gjk_step.search_origin = center;
            }
        } else {
            int n = simplex.contains(vec3(0, 0, 0), iterations > iter_limit);
            if(n == -1) {
                bool loop_epa = true;

                vec3 weights;

                vec3 contact_point_a;
                vec3 contact_point_b;
                vec3 separation_vector;
                vec3 collision_normal;

                polytope p;
                p.from_simplex(simplex);

                iterations = 0;
                uint32_t pass = 0;

                while(true) {
                    ++iterations;
                    
                    polytope_return r = p.find_closest_face();

                    if(r.f) set = true;
                    
                    if(r.vertices.size() == 0) {
                        std::cout << "ERROR: ZERO\n";
                        return {};
                    }

                    direction = r.normal;

                    //

                    epa_step.search_direction = direction;

                    vec3 center = vec3(0.0f);
                    for(auto i : r.vertices) center += i.m;
                    center /= r.vertices.size();
                    epa_step.search_origin = center;

                    //
                
                    uint32_t ia, ib;
                    vec3 point_a = support(direction, a_vertices);
                    vec3 point_b = support(-direction, b_vertices);

                    vec3 point_m = point_a - point_b;

                    epa_step.input_point = point_m;

                    float dist = dot(point_m, r.normal);
                    
                    if(iterations > iter_limit) {
                        std::cout << "limit EPA";

                        epa_step.erase_triangles.clear();
                        c_event.epa.push_back(epa_step);

                        return {};
                    }

                    // end epa

                    float limit_2 = 0.01f;

                    if(abs(dist - dot(r.vertices[0].m, r.normal)) < limit_2) {
                        ++pass;
                        
                        epa_step.erase_triangles.clear();
                        c_event.epa.push_back(epa_step);

                        //

                        contact_point_a = r.vertices[0].a * r.weights.x + r.vertices[1].a * r.weights.y + r.vertices[2].a * r.weights.z;
                        contact_point_b = r.vertices[0].b * r.weights.x + r.vertices[1].b * r.weights.y + r.vertices[2].b * r.weights.z;

                        c_event.point_a = contact_point_a;
                        c_event.point_b = contact_point_b;
                        c_event.point_m = contact_point_a - contact_point_b;
                        c_event.data = {r.vertices[0].m, r.vertices[1].m, r.vertices[2].m};

                        vec3 main_dir = a_rel_pos - b_rel_pos;
                        vec3 collision_normal;
                        bool line = false;
                        float sep = length(contact_point_a - contact_point_b);
                        
                        if(r.vertices[2].a != r.vertices[0].a && r.vertices[2].a != r.vertices[1].a && r.vertices[0].a != r.vertices[1].a) { // triangle to vertex, triangle is a
                            collision_normal = normalize(cross(r.vertices[0].a - r.vertices[2].a, r.vertices[1].a - r.vertices[2].a));

                            tag.type = axiom::collision_type::FACE;
                            tag.vid_a[0] = ivec3(r.vertices[0].a * 64.0f);
                            tag.vid_a[1] = ivec3(r.vertices[1].a * 64.0f);
                            tag.vid_a[2] = ivec3(r.vertices[2].a * 64.0f);
                            tag.vid_b[0] = ivec3(r.vertices[0].b * 64.0f);
                        } else if(r.vertices[2].b != r.vertices[0].b && r.vertices[2].b != r.vertices[1].b && r.vertices[0].b != r.vertices[1].b) { // triangle to vertex, triangle is b
                            collision_normal = normalize(cross(r.vertices[0].b - r.vertices[2].b, r.vertices[1].b - r.vertices[2].b));

                            tag.type =  axiom::collision_type::EDGE;
                            tag.vid_a[0] = ivec3(r.vertices[0].a * 64.0f);
                            tag.vid_b[0] = ivec3(r.vertices[0].b * 64.0f);
                            tag.vid_b[1] = ivec3(r.vertices[1].b * 64.0f);
                            tag.vid_b[2] = ivec3(r.vertices[2].b * 64.0f);
                        } else { // line to line
                            vec3 a0;
                            vec3 a1;
                            vec3 b0;
                            vec3 b1;

                            if(r.vertices[0].a != r.vertices[1].a) {
                                a0 = r.vertices[0].a;
                                a1 = r.vertices[1].a;
                            } else {
                                a0 = r.vertices[0].a;
                                a1 = r.vertices[2].a;
                            }

                            if(r.vertices[0].b != r.vertices[1].b) {
                                b0 = r.vertices[0].b;
                                b1 = r.vertices[1].b;
                            } else {
                                b0 = r.vertices[0].b;
                                b1 = r.vertices[2].b;
                            }

                            collision_normal = normalize(cross(a0 - a1, b0 - b1));

                            line = true;

                            tag.type =  axiom::collision_type::VERTEX;
                            tag.vid_a[0] = ivec3(a0 * 64.0f);
                            tag.vid_a[1] = ivec3(a1 * 64.0f);
                            tag.vid_b[0] = ivec3(b0 * 64.0f);
                            tag.vid_b[1] = ivec3(b1 * 64.0f);
                        }
                        
                        std::vector<return_point> return_points;
                        if(dot(collision_normal, main_dir) < 0.0f) collision_normal = -collision_normal;
                        
                        if(ca.faces.size() == 1) {
                            if(dot(collision_normal, ca.faces[0].normal) > 0.1f) collision_normal = -collision_normal;
                        } else if(cb.faces.size() == 1) {
                            if(dot(collision_normal, -cb.faces[0].normal) > 0.1f) collision_normal = -collision_normal;
                        }
                        
                        shape_face* af = get_normal(ca, ta.orientation, -collision_normal);
                        shape_face* bf = get_normal(cb, tb.orientation, collision_normal);

                        float threshold = cos(30.0f * axiom::pi / 180.0f);
                        float dot_a = 0.0f;// 
                        float dot_b = 0.0f;//
                        if(af) dot_a = dot(af->normal, -collision_normal);
                        if(bf) dot_b = dot(bf->normal, collision_normal);
                        
                        //collision_normal = glm::normalize(contact_point_b - contact_point_a);
                        
                        if(ca.faces.size() == 0 || cb.faces.size() == 0 || af == nullptr || bf == nullptr) {
                            collision_normal = glm::normalize(contact_point_b - contact_point_a);
                            return_points.push_back(return_point(contact_point_a, contact_point_b, collision_normal));
                        } else {
                            shape_face& a_face = *af;
                            shape_face& b_face = *bf;

                            vec3 a_normal = ta.orientation * ca.orientation * a_face.normal;
                            vec3 b_normal = tb.orientation * cb.orientation * b_face.normal;

                            std::vector<vec2> a_verts;
                            std::vector<vec2> b_verts;

                            mat3 rot_mat = rotate_to(collision_normal, vec3(0, 0, 1));
                            vec3 a_c = rot_mat * a_vertices[af->vertices[0]].center;
                            vec3 b_c = rot_mat * b_vertices[bf->vertices[0]].center;

                            for(uint32_t i : a_face.vertices) {
                                a_verts.push_back((rot_mat * a_vertices[i].center).xy());
                            }
                            for(uint32_t i : b_face.vertices) {
                                b_verts.push_back((rot_mat * b_vertices[i].center).xy());
                            }

                            a_normal = rot_mat * a_normal;
                            b_normal = rot_mat * b_normal;
                            
                            rot_mat = transpose(rot_mat);

                            if((a_verts.size() <= 2 && b_verts.size() <= 2) || (a_verts.size() <= 1 || b_verts.size() <= 1)) return_points.push_back(return_point(contact_point_a, contact_point_b, collision_normal));
                            else {
                                std::vector<vec2> vertices_c;

                                vec2 a_center = vec2(0.0f);
                                for(vec2 v : a_verts) {
                                    a_center += v.xy();
                                }
                                a_center /= a_verts.size();

                                vec2 b_center = vec2(0.0f);
                                for(vec2 v : b_verts) {
                                    b_center += v.xy();
                                }
                                b_center /= b_verts.size();

                                vec2 center;
                                vec2 vv = vec2(0.0f, 1.0f);

                                bool flip = false;
                                
                                for(vec2 v : b_verts) {
                                    vertices_c.push_back(v);
                                }

                                std::function<void()> sutherland_hodgman = [&]() {
                                    vec2 center = vec2(0.0f);
                                    int num_verts = 0;
                                    for(int i = 0; i < a_verts.size(); ++i) {
                                        center += a_verts[i];
                                        ++num_verts;
                                    }
                                    center /= num_verts;

                                    for(int i = 0; i < a_verts.size(); ++i) {
                                        vec2 C = a_verts[i];
                                        vec2 D = a_verts[(i + 1) % a_verts.size()];
                                        std::vector<vec2> new_c;
                                        vec3 c = cross(vec3(D - C, 0), vec3(center - C, 0));

                                        for(int j = 0; j < vertices_c.size(); ++j) {  
                                            vec2 A = vertices_c[j];
                                            vec2 B = vertices_c[(j + 1) % vertices_c.size()];

                                            vec2 d = normalize(D - C);
                                            d = vec2(-d.y, d.x);

                                            vec2 b = vec2(B - A);
                                            vec2 a = vec2(C - A);

                                            float m = dot(d, b);
                                            float x = dot(d, a) / m;

                                            vec2 E;
                                            bool has_E = false;

                                            if(x != NAN) {
                                                if(x > 0 && x < 1) {
                                                    E = vec2(A.xy() + b * x);
                                                    has_E = true;
                                                }
                                            }

                                            if(dot(cross(vec3(D - C, 0), vec3(A - C, 0)), c) > 0) {
                                                if(has_E) {
                                                    new_c.push_back(A);
                                                    new_c.push_back(E);
                                                } else {
                                                    new_c.push_back(A);
                                                }
                                            } else {
                                                if(has_E) {
                                                    new_c.push_back(E);
                                                }
                                            }
                                        }

                                        vertices_c = new_c;
                                    }
                                };

                                sutherland_hodgman();

                                if(vertices_c.size() == 0) return_points.push_back(return_point(a_c, b_c, collision_normal));

                                // FLAG

                                for(vec2 v : vertices_c) { 
                                    vec3 va = vec3(v, 0);
                                    float a_z = dot(a_c - va, a_normal) / a_normal.z;
                                    va.z = a_z;
                                    //va.z = a_c.z;

                                    vec3 vb = vec3(v, 0);
                                    float b_z = dot(b_c - vb, b_normal) / b_normal.z;
                                    vb.z = b_z;
                                    //vb.z = b_c.z;

                                    return_points.push_back(return_point(rot_mat * va, rot_mat * vb, collision_normal));
                                }
                            }
                        }

                        for(return_point& rp : return_points) {
                            c_event.manifold_a.push_back(rp.a);
                            c_event.manifold_b.push_back(rp.b);
                            c_event.collision_normal.push_back(collision_normal);
                            
                            rp.a = rp.a;
                            rp.b = rp.b + vec3(ta.position - tb.position);
                            rp.normal = collision_normal;
                        }

                        /*
                        if(set) {
                            pv.pos = pvec3(pnum(0xC3500001E20BE2, 0xEC), pnum(-0x752FFFFEDEC2A3, 0xFE), pnum(0x9C400000909141, 0x8D));
                            Physics_system& ps = ecs.get_system<Physics_system>();
                            ps.visualizer = pv;
                        }
                        */
                        return return_points;
                    } else {
                        std::vector<uint> removed = p.expand({point_m, point_a, point_b});
                        
                        epa_step.erase_triangles = removed;
                        c_event.epa.push_back(epa_step);
                    }
                }
            } else {
                gjk_step.erase_index = n;

                simplex.vertices.erase(simplex.vertices.begin() + n);

                vec3 center = (simplex.vertices[0].m + simplex.vertices[1].m + simplex.vertices[2].m) / 3.0f;
                vec3 normal = glm::normalize(glm::cross(simplex.vertices[0].m - simplex.vertices[2].m, simplex.vertices[1].m - simplex.vertices[2].m));
                if(glm::dot(normal, -center) <= 0.0f) normal = -normal;

                direction = normal;

                gjk_step.search_origin = center;
                gjk_step.search_direction = normal;
            }
        }
    }
}

std::vector<return_point> collide(transform3d& ta, collider3d& ca, transform3d& tb, collider3d& cb, std::vector<collision_event>& c_events) {
    std::vector<return_point> ret;

    return_tag tag;

    if(ca.bvh.nodes.size()) {
        if(cb.bvh.nodes.size()) {
            std::vector<uint64_t> pairs = traverse_bvh(ta, ca.bvh, tb, cb.bvh);

            for(uint64_t pair : pairs) {
                uint32_t a = pair & 0xFFFFFFFF;
                uint32_t b = pair >> 32;

                collision_shape3d& sa = ca.shapes[a];
                collision_shape3d& sb = cb.shapes[b];

                collision_event c_event;

                std::vector<return_point> r = collide(ta, sa, tb, sb, tag, c_event);

                c_event.shape_a = a;
                c_event.shape_b = b;
                c_event.transform_a = ta;
                c_event.transform_b = tb;
                c_event.finished = r.size() != 0;

                c_events.push_back(c_event);

                ret.insert(ret.end(), r.begin(), r.end());
            }
        } else {
            std::vector<return_tag> tags;
            std::vector<std::vector<return_point>> points;
            
            for(collision_shape3d& sb : cb.shapes) {
                std::vector<uint32_t> shapes = traverse_bvh(ta, ca.bvh, tb, sb.bounding_box);

                for(uint32_t shape : shapes) {
                    collision_shape3d& sa = ca.shapes[shape];

                    collision_event c_event;

                    std::vector<return_point> r = collide(ta, sa, tb, sb, tag, c_event);
                    
                    c_event.shape_a = shape;
                    c_event.shape_b = 0;
                    c_event.transform_a = ta;
                    c_event.transform_b = tb;
                    c_event.finished = r.size() != 0;

                    c_events.push_back(c_event);

                    tags.push_back(tag);
                    points.push_back(r);
                }
            }

            // faces first

            std::unordered_set<ivec3, hash_coord> set;

            for(uint32_t i = 0; i < tags.size(); ++i) {
                return_tag& tag = tags[i];
                if(tag.type == axiom::collision_type::FACE) {
                    ret.insert(ret.end(), points[i].begin(), points[i].end());

                    set.insert(tag.vid_a[0]);
                    set.insert(tag.vid_a[1]);
                    set.insert(tag.vid_a[2]);
                }
            }

            // then edges
            
            for(uint32_t i = 0; i < tags.size(); ++i) {
                return_tag& tag = tags[i];
                if(tag.type == axiom::collision_type::EDGE) {
                    if(!(set.contains(tag.vid_a[0]) && set.contains(tag.vid_a[1]))) {
                        ret.insert(ret.end(), points[i].begin(), points[i].end());
                    
                        set.insert(tag.vid_a[0]);
                        set.insert(tag.vid_a[1]);
                    }
                }
            }

            // and finally vertices
            
            for(uint32_t i = 0; i < tags.size(); ++i) {
                return_tag& tag = tags[i];
                if(tag.type == axiom::collision_type::VERTEX) {
                    if(!set.contains(tag.vid_a[0])) {
                        ret.insert(ret.end(), points[i].begin(), points[i].end());
                    
                        set.insert(tag.vid_a[0]);
                    }
                }
            }
        }
    } else if(cb.bvh.nodes.size()) {
        std::vector<return_tag> tags;
        std::vector<std::vector<return_point>> points;
        
        for(collision_shape3d& sa : ca.shapes) {
            std::vector<uint32_t> shapes = traverse_bvh(tb, cb.bvh, ta, sa.bounding_box);

            for(uint32_t shape : shapes) {
                collision_shape3d& sb = cb.shapes[shape];

                collision_event c_event;

                std::vector<return_point> r = collide(ta, sa, tb, sb, tag, c_event);
                
                c_event.shape_a = 0;
                c_event.shape_b = shape;
                c_event.transform_a = ta;
                c_event.transform_b = tb;
                c_event.finished = r.size() != 0;

                c_events.push_back(c_event);

                tags.push_back(tag);
                points.push_back(r);
            }
        }

        // faces first

        std::unordered_set<ivec3, hash_coord> set;

        for(uint32_t i = 0; i < tags.size(); ++i) {
            return_tag& tag = tags[i];
            if(tag.type == axiom::collision_type::VERTEX) {
                ret.insert(ret.end(), points[i].begin(), points[i].end());

                set.insert(tag.vid_b[0]);
                set.insert(tag.vid_b[1]);
                set.insert(tag.vid_b[2]);
            }
        }

        // then edges
        
        for(uint32_t i = 0; i < tags.size(); ++i) {
            return_tag& tag = tags[i];
            if(tag.type == axiom::collision_type::EDGE) {
                if(!(set.contains(tag.vid_b[0]) && set.contains(tag.vid_b[1]))) {
                    ret.insert(ret.end(), points[i].begin(), points[i].end());
                
                    set.insert(tag.vid_b[0]);
                    set.insert(tag.vid_b[1]);
                }
            }
        }

        // and finally vertices
        
        for(uint32_t i = 0; i < tags.size(); ++i) {
            return_tag& tag = tags[i];
            if(tag.type == axiom::collision_type::FACE) {
                if(!set.contains(tag.vid_b[0])) {
                    ret.insert(ret.end(), points[i].begin(), points[i].end());
                    
                    set.insert(tag.vid_b[0]);
                }
            }
        }
    } else {
        for(collision_shape3d& sa : ca.shapes) {
            for(collision_shape3d& sb : cb.shapes) {
                collision_event c_event;

                std::vector<return_point> r = collide(ta, sa, tb, sb, tag, c_event);

                c_event.shape_a = 0;
                c_event.shape_b = 0;
                c_event.transform_a = ta;
                c_event.transform_b = tb;
                c_event.finished = r.size() != 0;

                c_events.push_back(c_event);
                
                ret.insert(ret.end(), r.begin(), r.end());
            }
        }
    }

    return ret;
}

bool gjk(collision_shape3d& ca, transform3d& ta, collision_shape3d& cb, transform3d& tb) {
    std::vector<vertex_element3d> a_vertices = ca.elements;
    std::vector<vertex_element3d> b_vertices = cb.elements;

    float limit = 0.001f;
    uint32_t iter_limit = 128;
    
    transform3d tta = ta;
    tta.orientation = tta.orientation * ca.orientation;
    tta.position += tta.orientation * ca.position;
    transform3d ttb = tb;
    ttb.orientation = ttb.orientation * cb.orientation;
    ttb.position += ttb.orientation * cb.position;

    vec3 a_rel_pos = transform_vertices(a_vertices, tta, ta.position);
    vec3 b_rel_pos = transform_vertices(b_vertices, ttb, ta.position);

    axiom::simplex simplex;

    vec3 direction = glm::normalize(vec3(a_rel_pos - b_rel_pos));

    vec3 offset = vec3(direction.y, -direction.z, direction.x);

    if(abs(glm::dot(offset, direction)) > 0.95) {
        offset = vec3(direction.z, -direction.y, direction.x);
    }

    direction = glm::normalize(direction + offset * 0.1f);

    int iterations = 0;

    bool loop = true;

    auto get_normal = [&](collision_shape3d& c, mat3 ori, vec3 dir) -> shape_face* {
        float dd = -FLT_MAX;
        vec3 vec = dir;
        int32_t id = -1;

        uint32_t i = 0;
        for(shape_face& t : c.faces) {
            vec3 normal = ori * t.normal;

            float d = dot(normal, dir);

            if(d > dd) {
                vec = normal;
                dd = d;
                id = i;
            }

            ++i;
        }

        if(id >= 0) return &c.faces[id];
        else return nullptr;
    };

    while(loop) {
        ++iterations;
        
        int size = simplex.vertices.size();
        if(size < 4) {
            if(iterations > iter_limit) return false;

            if(glm::isnan(direction.x)) {
                std::cout << "NAN DIRECTION\n";
                direction = vec3(1, 0, 0);
            }

            vec3 point_a = support(direction, a_vertices);
            vec3 point_b = support(-direction, b_vertices);

            vec3 point_m = point_a - point_b;

            for(simplex_vertex& v : simplex.vertices) {
                vec3 difference = point_m - v.m;

                float dist = dot(difference, direction);

                if(dist <= limit) return false;
            }

            if(glm::dot(point_m, direction) <= limit) return false;

            simplex.vertices.push_back(simplex_vertex{point_m, point_a, point_b});

            if(size == 0) {
                direction = -glm::normalize(point_m);
            } else if(size == 1) {
                vec3 line_direction = normalize(simplex.vertices[0].m - simplex.vertices[1].m);
                vec3 rel_origin_pos = -simplex.vertices[1].m;

                vec3 closest_point = line_direction * glm::dot(rel_origin_pos, line_direction) + simplex.vertices[1].m;
                direction = glm::normalize(-closest_point);
            } else if(size == 2) {
                vec3 center = (simplex.vertices[0].m + simplex.vertices[1].m + simplex.vertices[2].m) / 3.0f;
                vec3 normal = glm::normalize(glm::cross(simplex.vertices[0].m - simplex.vertices[2].m, simplex.vertices[1].m - simplex.vertices[2].m));
                if(glm::dot(normal, -center) <= 0.0f) normal = -normal;

                direction = normal;
            }
        } else {
            int n = simplex.contains(vec3(0, 0, 0), iterations > iter_limit);
            if(n == -1) {
                return true;
            } else {
                simplex.vertices.erase(simplex.vertices.begin() + n);

                vec3 center = (simplex.vertices[0].m + simplex.vertices[1].m + simplex.vertices[2].m) / 3.0f;
                vec3 normal = glm::normalize(glm::cross(simplex.vertices[0].m - simplex.vertices[2].m, simplex.vertices[1].m - simplex.vertices[2].m));
                if(glm::dot(normal, -center) <= 0.0f) normal = -normal;

                direction = normal;
            }
        }
    }
}

std::vector<uint> gjk_bvh(collider3d& ca, transform3d& ta, collider3d& cb, collision_shape3d& ccb, transform3d& tb) {
    std::vector<uint32_t> ret;

    std::vector<uint32_t> colliders = traverse_bvh(ta, ca.bvh, tb, ccb.bounding_box);

    for(uint32_t c : colliders) {
        collision_shape3d& cca = ca.shapes[c];

        return_tag tag;
        bool b = gjk(cca, ta, ccb, tb);

        if(b) ret.push_back(c);
    }

    return ret;
}

}