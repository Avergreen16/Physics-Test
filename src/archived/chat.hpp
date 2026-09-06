#pragma once
#include <nlohmann/json.hpp>
#include <utilities.hpp>
#include <math.hpp>
#include <ecs.hpp>
#include <ui.hpp>

using json = nlohmann::json;

void generate_placeholder();

struct message {
    std::string sender;
    std::string message;
    ulong timestamp;
    ulong index;
};

struct chat_system : axiom::system {
    std::vector<message> messages;
    std::unordered_map<ulong, ulong> message_map;

    ivec2 message_range = ivec2(0, 0);

    //

    ulong root_id;
    bool enabled = false;

    vec3 player_color = vec3(0.0625f);
    vec3 self_color = axiom::color_blue;
    vec3 addie_color = axiom::color_purple;
    vec3 admin_color = axiom::color_red;

    bool rebuild_flag = false;
    
    std::vector<json> json_files;
    
    std::unordered_map<ulong, ulong> pop_headers();
    void push_headers(std::unordered_map<ulong, ulong> headers);

    void insert_message(ulong message_root, std::string sender, std::string message, ulong timestamp, vec3 color, ulong map_index, int index = -1);
    void remove_message(ulong message_root, ulong id, ulong map_index);

    void rebuild();

    void call();

    void enable(ulong root_widget);

    void disable();

    chat_system();
};