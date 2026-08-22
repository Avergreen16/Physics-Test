#include <physics-3d/debugger.hpp>

namespace axiom {

void physics_debugger::insert_frame(ulong id, debugger_frame frame) {
    frame_list.push_back(id);
    frames.emplace(id, frame);

    while(frame_list.size() > frame_buffer) {
        frames.erase(frame_list.front());
        frame_list.erase(frame_list.begin());
    }
}

std::vector<uint> physics_debugger::search_collision_collider(ulong frame, uint collider) {
    std::vector<uint> indices;

    if(frames.contains(frame)) {
        uint i = 0;
        for(collision_event& event : frames[frame].collision_events) {
            if(event.collider_a == collider || event.collider_b == collider) {
                indices.push_back(i);
            }  

            ++i;
        }
    }
    
    return indices;
}

std::vector<uint> physics_debugger::search_collision_collider(ulong frame, uint collider_a, uint collider_b) {
    std::vector<uint> indices;

    if(frames.contains(frame)) {
        uint i = 0;
        for(collision_event& event : frames[frame].collision_events) {
            if(event.collider_a == collider_a && event.collider_b == collider_b || event.collider_a == collider_b || event.collider_b == collider_a) {
                indices.push_back(i);
            }  

            ++i;
        }
    }

    return indices;
}

}