#include <physics-3d/mesh.hpp>
#include <physics-3d/collider.hpp>
#include <include/utilities.hpp>

#include <iostream>

namespace axiom {

struct mesh_face {
    int i0;
    int i1;
    int i2;
};

void create_mesh(std::vector<vertex_element3d> elements, std::vector<output_vertex>* vertices, std::vector<uint>* indices) {
    output_vertex va;
    output_vertex vb;
    output_vertex vc;

    vec3 center = vec3(0.0f);
    for(auto& element : elements) center += element.center;
    center /= elements.size();

    va.position = support(vec3(1.0f, 0.0f, 0.0f), elements, va.element);
    vb.position = support(normalize(center - va.position), elements, vb.element);

    vec3 dir = normalize(va.position - vb.position);
    vec3 d = vb.position + dir * dot(dir, center - vb.position);
    vec3 dd = dir;

    dir = center - d;
    if(length(dir) < 0.01) dir += normalize(vec3(-dd.y, dd.x, 0.0f)) * 0.02f;
    
    dir = normalize(dir);

    vc.position = support(dir, elements, vc.element);

    std::vector<output_vertex> points = {va, vb, vc};

    std::vector<mesh_face> faces = {mesh_face(0, 1, 2), mesh_face(2, 1, 0)};
    
    while(true) {
        std::vector<mesh_face> new_faces;

        std::vector<uint> to_erase;

        float threshold = 0.0025f;

        int index = 0;

        for(mesh_face& face : faces) {
            vec3 normal = normalize(cross(points[face.i0].position - points[face.i2].position, points[face.i1].position - points[face.i2].position));

            output_vertex s;
            s.position = support(normal, elements, s.element);

            //

            float t = glm::max(glm::max(glm::length(elements[s.element].radii), glm::length(elements[points[face.i0].element].radii)) * threshold, threshold);

            if(dot(s.position, normal) > dot(points[face.i0].position, normal) + t) {
                std::vector<uint> to_erase2 = {};

                int index2 = index;

                for(uint i = 0; i < faces.size(); ++i) {
                    mesh_face& face = faces[i];
                    
                    vec3 nn = normalize(cross(points[face.i0].position - points[face.i2].position, points[face.i1].position - points[face.i2].position));
                    
                    if(dot(s.position, nn) > dot(points[face.i0].position, nn)) {
                        to_erase2.push_back(i);
                    }
                }

                std::unordered_set<uint> indices;
                std::unordered_map<uint, uint> edges;
                for(int i : to_erase2) {
                    mesh_face& face = faces[i];

                    uint a = face.i0;
                    uint b = face.i1;
                    uint c = face.i2;
                    
                    indices.insert(a);
                    indices.insert(b);
                    indices.insert(c);

                    uint e0 = ((glm::max(a, b) << 16) | glm::min(a, b));
                    uint e1 = ((glm::max(b, c) << 16) | glm::min(b, c));
                    uint e2 = ((glm::max(c, a) << 16) | glm::min(c, a));

                    if(edges.contains(e0)) ++edges[e0];
                    else edges[e0] = 1;
                    
                    if(edges.contains(e1)) ++edges[e1];
                    else edges[e1] = 1;
                    
                    if(edges.contains(e2)) ++edges[e2];
                    else edges[e2] = 1;
                }

                vec3 center = vec3(0.0f);
                for(uint i : indices) center += points[i].position;
                center /= indices.size();

                vec3 dir = s.position - center;

                for(auto [e, n] : edges) {
                    if(n == 1) {
                        uint e0 = e & 0xfFFF;
                        uint e1 = e >> 16;

                        vec3 nn = normalize(cross(points[e0].position - s.position, points[e1].position - s.position));
                        if(dot(nn, dir) < 0.0f) std::swap(e0, e1);
                        
                        new_faces.push_back(mesh_face(e0, e1, points.size()));
                    }
                }

                to_erase.insert(to_erase.end(), to_erase2.begin(), to_erase2.end());

                points.push_back(s);

                break;
            }

            ++index;
        }

        if(index == faces.size()) break;

        for(auto iter = to_erase.rbegin(); iter != to_erase.rend(); ++iter) {
            faces.erase(faces.begin() + *iter);
        }
        faces.insert(faces.end(), new_faces.begin(), new_faces.end());
    }

    for(auto point : points) {
        vertices->push_back(point);
    }
    
    for(mesh_face& face : faces) {
        indices->push_back(face.i0);
        indices->push_back(face.i1);
        indices->push_back(face.i2);
    }
}

}