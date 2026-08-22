#pragma once

#include <physics-3d/bounding_box.hpp>
#include <physics-3d/collider.hpp>

#include <iostream>

namespace axiom {
    
bounding_box3d create_bounding_box(std::vector<vertex_element3d>& elements) {
    bounding_box3d bb;

    bb.minimum = vec3(
        support(vec3(-1.0f, 0.0f, 0.0f), elements).x,
        support(vec3(0.0f, -1.0f, 0.0f), elements).y,
        support(vec3(0.0f, 0.0f, -1.0f), elements).z
    );

    bb.maximum = vec3(
        support(vec3(1.0f, 0.0f, 0.0f), elements).x,
        support(vec3(0.0f, 1.0f, 0.0f), elements).y,
        support(vec3(0.0f, 0.0f, 1.0f), elements).z
    );
    
    vec3 size = bb.maximum - bb.minimum;
    bb.volume = size.x * size.y * size.z;

    return bb;
}

bounding_box3d create_bounding_box(std::vector<vertex_element3d> elements, vec3 offset_pos, mat3 offset_ori) {
    for(auto& element : elements) {
        element.center = offset_ori * element.center + offset_pos;
        element.orientation = offset_ori * element.orientation;
    }

    return create_bounding_box(elements);
}

void create_bounding_box(collider3d& collider) {
    bounding_box3d total_bb;

    collider.init_bb = true;

    total_bb.minimum = vec3(FLT_MAX);
    total_bb.maximum = vec3(-FLT_MAX);

    for(auto& shape : collider.shapes) {
        shape.bounding_box = create_bounding_box(shape.elements, shape.position, shape.orientation);

        total_bb.maximum = glm::max(total_bb.maximum, shape.bounding_box.maximum);
        total_bb.minimum = glm::min(total_bb.minimum, shape.bounding_box.minimum);
    }
    
    vec3 size = total_bb.maximum - total_bb.minimum;
    total_bb.volume = size.x * size.y * size.z;

    
    collider.bounding_box = total_bb;
}

bounding_box3d transform_bounding_box(bounding_box3d b, vec3 pos, mat3 ori) {
    vec3 center = (b.minimum + b.maximum) * 0.5f;
    vec3 extents = b.maximum - center;

    center = ori * center + pos;
    extents = mat3(abs(ori[0]), abs(ori[1]), abs(ori[2])) * extents;

    b.minimum = center - extents;
    b.maximum = center + extents;

    return b;
};

void create_bvh(collider3d& collider, ivec3 v) {
    create_bounding_box(collider);

    bvh3d bvh;

    bvh_node3d root;
    
    for(int i = 0; i < collider.shapes.size(); ++i) root.children.push_back(i);

    uint N = collider.shapes.size();

    bvh.nodes.push_back(root);

    uint ca;
    uint cb;

    bool f = true;

    auto split = [&](bvh_node3d& node) {
        uint index = 0;

        bounding_box3d centers;

        for(int i : node.children) {
            collision_shape3d& shape = collider.shapes[i];
            vec3 center = (shape.bounding_box.minimum + shape.bounding_box.maximum) * 0.5f;

            node.bounding_box.minimum = min(node.bounding_box.minimum, shape.bounding_box.minimum);
            node.bounding_box.maximum = glm::max(node.bounding_box.maximum, shape.bounding_box.maximum);

            centers.minimum = min(centers.minimum, center);
            centers.maximum = glm::max(centers.maximum, center);
        }

        if(node.split) {
            vec3 size = centers.maximum - centers.minimum;
            vec3 center = (centers.minimum + centers.maximum) * 0.5f;

            bvh_node3d child_a;
            bvh_node3d child_b;

            int ii = 0;
            if(size.y > size.x && size.y > size.z) ii = 1;
            else if(size.z > size.x && size.z > size.y) ii = 2;
            
            for(int i : node.children) {
                collision_shape3d& shape = collider.shapes[i];

                float c = (shape.bounding_box.minimum[ii] + shape.bounding_box.maximum[ii]) * 0.5f;
                if(c < center[ii]) child_a.children.push_back(i);
                else child_b.children.push_back(i);
            }

            if(child_a.children.size() == 0)  {
                uint split = child_b.children.size() / 2;
                
                child_a.children = std::vector<uint>(child_b.children.begin() + split, child_b.children.end());
                child_b.children.resize(split);
            } else if(child_b.children.size() == 0) {
                uint split = child_b.children.size() / 2;

                child_b.children = std::vector<uint>(child_a.children.begin() + split, child_a.children.end());
                child_a.children.resize(split);
            }
               
                
            if(child_a.children.size() == 1) child_a.split = false;
            if(child_b.children.size() == 1) child_b.split = false;
            
            ca = bvh.nodes.size();
            cb = bvh.nodes.size() + 1;
            
            node.children = {ca, cb};

            bvh.nodes.push_back(child_a);
            bvh.nodes.push_back(child_b);
            
            return true;
        } else return false;
    };

    std::vector<uint> open_nodes = {0};
    std::vector<uint> new_open_nodes = {};

    while(true) {
        if(open_nodes.size() == 0) break;

        for(uint n : open_nodes) {
            if(split(bvh.nodes[n])) {
                new_open_nodes.push_back(ca);
                new_open_nodes.push_back(cb);
            }
        }

        open_nodes = std::move(new_open_nodes);
        new_open_nodes.clear();
    }

    collider.bvh = bvh;
}

std::vector<uint> traverse_bvh(transform3d& ta, bvh3d& ba, transform3d& tb, bounding_box3d& bb) {
    std::vector<uint> front_buffer = {0};
    std::vector<uint> back_buffer;
    std::vector<uint> shapes;

    while(true) {
        if(front_buffer.size() == 0) break;

        for(uint i : front_buffer) {
            bvh_node3d& node = ba.nodes[i];

            if(collide(ta, node.bounding_box, tb, bb)) {
                if(node.children.size() > 1) {
                    back_buffer.push_back(node.children[0]);
                    back_buffer.push_back(node.children[1]);
                } else shapes.push_back(node.children[0]);
            }
        }

        front_buffer = back_buffer;
        back_buffer.clear();
    }

    return shapes;
}

std::vector<ulong> traverse_bvh(transform3d& ta, bvh3d& ba, transform3d& tb, bvh3d& bb) {
    std::vector<ulong> front_buffer = {0};
    std::vector<ulong> back_buffer;
    std::vector<ulong> shape_pairs;

    while(true) {
        if(front_buffer.size() == 0) break;

        for(ulong i : front_buffer) {
            uint ai = i & 0xFFFFFFFF;
            uint bi = i >> 32;

            bvh_node3d& node_a = ba.nodes[ai];
            bvh_node3d& node_b = bb.nodes[bi];

            if(collide(ta, node_a.bounding_box, tb, node_b.bounding_box)) {
                if(node_a.children.size() == 1) {
                    if(node_b.children.size() == 1) {
                        shape_pairs.push_back(ulong(node_a.children[0]) | (ulong(node_b.children[0]) << 32));
                    } else {
                        back_buffer.push_back(ulong(ai) | (ulong(node_b.children[0]) << 32));
                        back_buffer.push_back(ulong(ai) | (ulong(node_b.children[1]) << 32));
                    }
                } else if(node_b.children.size() == 1) {
                    back_buffer.push_back(ulong(node_a.children[0]) | (ulong(bi) << 32));
                    back_buffer.push_back(ulong(node_a.children[1]) | (ulong(bi) << 32));
                } else {
                    back_buffer.push_back(ulong(node_a.children[0]) | (ulong(node_b.children[0]) << 32));
                    back_buffer.push_back(ulong(node_a.children[1]) | (ulong(node_b.children[0]) << 32));
                    back_buffer.push_back(ulong(node_a.children[0]) | (ulong(node_b.children[1]) << 32));
                    back_buffer.push_back(ulong(node_a.children[1]) | (ulong(node_b.children[1]) << 32));
                }
            }
        }

        front_buffer = back_buffer;
        back_buffer.clear();
    }

    return shape_pairs;
}

bool collide(transform3d& ta, bounding_box3d& a, transform3d& tb, bounding_box3d& b) {
    if(a.volume < b.volume) {
        vec3 pos = glm::transpose(tb.orientation) * vec3(ta.position - tb.position);
        mat3 ori = glm::transpose(tb.orientation) * ta.orientation;

        bounding_box3d na = transform_bounding_box(a, pos, ori);

        bool xb = na.minimum.x < b.maximum.x && b.minimum.x < na.maximum.x;
        bool yb = na.minimum.y < b.maximum.y && b.minimum.y < na.maximum.y;
        bool zb = na.minimum.z < b.maximum.z && b.minimum.z < na.maximum.z;

        return xb && yb && zb;
    } else {
        vec3 pos = glm::transpose(ta.orientation) * vec3(tb.position - ta.position);
        mat3 ori = glm::transpose(ta.orientation) * tb.orientation;

        bounding_box3d nb = transform_bounding_box(b, pos, ori);

        bool xb = a.minimum.x < nb.maximum.x && nb.minimum.x < a.maximum.x;
        bool yb = a.minimum.y < nb.maximum.y && nb.minimum.y < a.maximum.y;
        bool zb = a.minimum.z < nb.maximum.z && nb.minimum.z < a.maximum.z;

        return xb && yb && zb;
    }
}


bool collide(bounding_box3d& a, bounding_box3d& b) {
    bool xb = a.minimum.x < b.maximum.x && b.minimum.x < a.maximum.x;
    bool yb = a.minimum.y < b.maximum.y && b.minimum.y < a.maximum.y;
    bool zb = a.minimum.z < b.maximum.z && b.minimum.z < a.maximum.z;

    return xb && yb && zb;
}

}