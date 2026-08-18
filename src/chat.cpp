#include <chat.hpp>

#include <consts.hpp>

void generate_placeholder() {
    json root;
    root["messages"] = json::array();

    //
    
    axiom::random32 rand(0xFF55FF66);

    ulong timestamp = axiom::get_timestamp();
    ulong start_timestamp = timestamp - 365.0f * 86400.0f * 1000000.0f;

    std::vector<std::string> names = {
        "Averie",
        "Averie",
        "Averie",
        "Averie",
        "Averie",
        "Averie",
        "Averie",
        "Averie",

        //

        "Addie",
        "Addie",
        "Addie",
        "Addie",
        "Addie",
        "Addie",
        "Addie",
        "Addie",

        //

        "ADMIN",
        "ADMIN",
        "ADMIN",
        "ADMIN",

        //

        "Luna",
        "Nova",
        "Kael",
        "Mira",
        "Rowan",
        "Lyra12",
        "RigelOrion",
        "Violet",
        "Atlas",
        "Ember",
        "Jasper",
        "Willow",
        "Phoenix",
        "xXSkyeXx",
        "Echo",
        "Aria",
        "Finn",
        "Cora",
        "Riven",
        "NovaByte86",
        "PixelFox3",
        "Solaris",
        "Zenith",
        "Nyx",
        "Rune",
        "Vale",
        "Axel",
        "Maris",
        "Cypher50",
        "Elara"
    };

    std::vector<std::string> messages = {
        "Hey, are you online? I was wondering if you wanted to explore the new area together because I found a really interesting path that I don't think many people have discovered yet.",
        "I just finished the new update and there are so many small changes that I didn't notice at first. The new interface feels much cleaner and the animations are really nice.",
        "Want to explore the new area together? I heard there are some rare resources hidden near the mountains, but we should probably bring some supplies before we go.",
        "That was a really close fight. I thought we were going to lose when the boss started using that final attack, but everyone worked together and we barely survived.",
        "I found a secret room behind the waterfall. It had some old decorations, a few mysterious items, and a note that looked like it was left there years ago.",
        "Can you help me with this quest? I have been stuck on this part for a while because the enemies keep spawning faster than I can defeat them.",
        "The weather looks amazing today. It would be nice to find somewhere peaceful to build a small base and just watch the sunset.",
        "I finally got the item I was looking for! It took several hours of searching, but it was completely worth it because it fits perfectly with my current setup.",
        "Does anyone want to join the party? We could probably finish the dungeon much faster if we had a few more people helping with the harder sections.",
        "I'll be there in five minutes. I just need to finish organizing my inventory because somehow I managed to fill every single storage slot again.",
        "That was the funniest thing I've seen all day. I was not expecting the game physics to completely break like that, but somehow it made the moment even better.",
        "I think we should build our base here. The location is close to resources, has a nice view, and would be easy to defend if anything attacks us.",
        "Have you tried the new feature yet? I think it has a lot of potential, but there are still a few things that could be improved.",
        "The server is running really smoothly today. I remember when we first started playing and everything was lagging constantly whenever too many people joined.",
        "I saved you some resources from my last trip. I wasn't sure what you needed, so I grabbed a little bit of everything just in case.",
        "Let's meet at the northern gate around sunset. From there we can decide where to go next and make a plan before we start exploring.",
        "I need to take a quick break. My hands are getting tired from gathering materials for so long, but I'll be back soon.",
        "Did you see what happened earlier? The entire area changed after the event started, and I think there might be more secrets hidden nearby.",
        "This place has such a cool atmosphere. The lighting, music, and small details make it feel like someone put a lot of effort into designing it.",
        "I finished organizing the inventory. It took longer than expected because I kept finding old items that I forgot I had collected.",
        "Your idea actually worked! I wasn't sure the strategy would succeed, but it ended up being much better than what we were doing before.",
        "Let's try a different strategy this time. The last attempt was close, but I think we can make a few changes that will improve our chances.",
        "I can't believe we actually won. That was probably one of the hardest challenges we've completed so far.",
        "Do you remember where we found that? I want to go back there later because I think there might have been something else we missed.",
        "I'll send you the coordinates. It should be easy to find, but make sure you bring enough supplies because the journey takes longer than it looks.",
        "The update notes look interesting. I'm especially curious about the changes to the crafting system because it could completely change how people play.",
        "That animation looks really polished. Small details like that make the whole world feel much more alive and enjoyable.",
        "I think we are ready to continue. Everyone has their equipment prepared, and we should have enough resources for the next part.",
        "Thanks for helping me out. I know that took a lot of time, and I really appreciate you sticking around until we finished.",
        "Something feels different today. I can't really explain it, but the world feels more alive than usual.",
        "I have a new idea I want to test. It might not work perfectly at first, but I think it could lead to something really interesting.",
        "See you again soon! Hopefully next time we can finish the rest of the adventure and discover what happens next."
    };

    int num_messages = 1000;
    ivec2 num_per_person = {1, 2};

    int counter = 0;
    int person_num = 0;
    std::string name;
    ulong time;
    for(int i = 0; i < num_messages; ++i) {
        float frac = float(i) / num_messages;

        if(counter > person_num) {
            person_num = floor(num_per_person.x + (num_per_person.y - num_per_person.x) * (rand() * 0.5f + 0.5f)); 

            counter = 0;

            name = names[floor((rand() * 0.5f + 0.5f) * names.size())];

            time = start_timestamp + float(timestamp - start_timestamp) * frac;
        }

        message m;
        m.sender = name;

        uint n = floor((rand() * 0.5f + 0.5f) * 3) + 1;
        std::string str;
        for(int i = 0; i < n; ++i) {
            str += messages[floor((rand() * 0.5f + 0.5f) * messages.size())];
            if(i < n - 1) str += "\n";
        }
        m.message = str;

        m.timestamp = time;
        m.index = i;

        ++counter;

        //

        json mes;
        mes["sender"] = m.sender;
        mes["text"] = m.message;
        mes["timestamp"] = m.timestamp;

        root["messages"].push_back(mes);
    }
    
    axiom::write_text_to_file(output_root + "json/messages.json", root.dump(4));
}

void chat_system::call() {
    if(!enabled) return;

    //

    auto& ui_system = axiom::global_core.ecs->get_system<axiom::ui_system>();

    std::vector<ulong> existing_messages;
    for(auto [message_id, widget_id] : message_map) existing_messages.push_back(message_id);

    std::sort(existing_messages.begin(), existing_messages.end());

    //

    auto& root_w = ui_system.widgets[root_id];
    axiom::scroll_widget* scroll_w = dynamic_cast<axiom::scroll_widget*>(ui_system.widgets[root_w->parent].get());

    if(scroll_w->capture_scroll) return;

    bool found_before = std::find(root_w->children.begin(), root_w->children.end(), scroll_w->anchor_widget) != root_w->children.end();

    //

    float bottom;
    float top;
    float start_height;
    
    float buf = 100.0f;
    float extents = 800.0f;

    float bottom_target = scroll_w->size.y + extents;
    float top_target = extents;

    float start_scroll = scroll_w->scroll_pos;
    float start_size = root_w->size.y;
    
    //std::cout << "TOP ADD\n";

    float delta_scroll = 0.0f;

    bottom = scroll_w->scroll_pos + root_w->size.y;
    top = -scroll_w->scroll_pos;

    std::vector<float> vvs = {bottom_target, top_target, bottom, top};

    uint removed = 0;
    uint added = 0;
    uint readded = 0;

    if(top < top_target) { // add top
        float delta;
        int message_id;
        if(existing_messages.size()) message_id = existing_messages[0];
        else message_id = messages.size();

        while(true) {
            message_id -= 1;
            if(message_id < 0) break;

            message& mes = messages[message_id];
            
            //

            auto headers = pop_headers();

            vec3 color = player_color;
            if(mes.sender == "Averie") color = self_color;
            else if(mes.sender == "Addie") color = addie_color;
            else if(mes.sender == "ADMIN") color = admin_color;

            insert_message(root_id, mes.sender, mes.message, mes.timestamp, color, message_id, 0);
            ++added;

            push_headers(headers);

            if(scroll_w->anchor_mode == 1) scroll_w->anchor_mode = 0;
            for(int i = 0; i < 4; ++i) ui_system.measure(scroll_w->self);

            //

            bottom = scroll_w->scroll_pos + root_w->size.y;
            top = -scroll_w->scroll_pos;

            if(top > top_target) break;
        }
    }
    
    //std::cout << "TOP REMOVE\n";
    
    bottom = scroll_w->scroll_pos + root_w->size.y;
    top = -scroll_w->scroll_pos;

    if(top > top_target + buf) { // remove top
        float delta;
        int message_index = 0;

        while(true) {
            if(!existing_messages.size()) break;
            
            int message_id = existing_messages[message_index];
            ulong widget_id = message_map[message_id];

            auto& widget = ui_system.widgets[widget_id];

            //

            auto headers = pop_headers();

            remove_message(root_id, widget_id, message_id);
            ++removed;
            existing_messages.erase(existing_messages.begin());

            push_headers(headers);
            
            if(scroll_w->anchor_mode == 1) scroll_w->anchor_mode = 0;
            for(int i = 0; i < 4; ++i) ui_system.measure(scroll_w->self);

            //
            
            bottom = scroll_w->scroll_pos + root_w->size.y;
            top = -scroll_w->scroll_pos;

            if(top < top_target + buf) {
                message& mes = messages[message_id];
                
                auto headers = pop_headers();

                vec3 color = player_color;
                if(mes.sender == "Averie") color = self_color;
                else if(mes.sender == "Addie") color = addie_color;
                else if(mes.sender == "ADMIN") color = admin_color;

                insert_message(root_id, mes.sender, mes.message, mes.timestamp, color, message_id, 0);
                ++readded;

                push_headers(headers);

                if(scroll_w->anchor_mode == 1) scroll_w->anchor_mode = 0;
                for(int i = 0; i < 4; ++i) ui_system.measure(scroll_w->self);

                break;
            }
        }
    }

    //std::cout << "BOTTOM ADD\n";

    bottom = scroll_w->scroll_pos + root_w->size.y;
    top = -scroll_w->scroll_pos;

    if(bottom < bottom_target) { // add bottom
        float delta;
        int message_id;
        if(existing_messages.size()) message_id = existing_messages.back();
        else message_id = messages.size() - 1;

        while(true) {
            message_id += 1;
            if(message_id >= messages.size()) break;

            message& mes = messages[message_id];
            
            //

            auto headers = pop_headers();

            vec3 color = player_color;
            if(mes.sender == "Averie") color = self_color;
            else if(mes.sender == "Addie") color = addie_color;
                else if(mes.sender == "ADMIN") color = admin_color;

            insert_message(root_id, mes.sender, mes.message, mes.timestamp, color, message_id, root_w->children.size());
            ++added;
            existing_messages.push_back(message_id);

            push_headers(headers);

            //

            if(scroll_w->anchor_mode == 2) scroll_w->anchor_mode = 0;
            for(int i = 0; i < 4; ++i) ui_system.measure(scroll_w->self);
            
            bottom = scroll_w->scroll_pos + root_w->size.y;
            top = -scroll_w->scroll_pos;

            if(bottom > bottom_target) break;
        }   
    }
    
    //std::cout << "BOTTOM REMOVE\n";

    bottom = scroll_w->scroll_pos + root_w->size.y;
    top = -scroll_w->scroll_pos;

    if(bottom > bottom_target + buf) { // remove bottom
        float delta;

        while(true) {
            if(existing_messages.size() == 0) break;

            int message_id = existing_messages.back();

            ulong widget_id = message_map[message_id];

            auto& widget = ui_system.widgets[widget_id];

            auto headers = pop_headers();

            remove_message(root_id, widget_id, message_id);
            ++removed;

            existing_messages.erase(existing_messages.end() - 1);

            push_headers(headers);
            
            if(scroll_w->anchor_mode == 2) scroll_w->anchor_mode = 0;
            for(int i = 0; i < 4; ++i) ui_system.measure(scroll_w->self);

            //
            
            bottom = scroll_w->scroll_pos + root_w->size.y;
            top = -scroll_w->scroll_pos;
            
            if(bottom < bottom_target + buf) {
                message& mes = messages[message_id];
                
                auto headers = pop_headers();

                vec3 color = player_color;
                if(mes.sender == "Averie") color = self_color;
                else if(mes.sender == "Addie") color = addie_color;
                else if(mes.sender == "ADMIN") color = admin_color;

                insert_message(root_id, mes.sender, mes.message, mes.timestamp, color, message_id, root_w->children.size());
                ++readded;
                existing_messages.push_back(message_id);

                push_headers(headers);
                
                if(scroll_w->anchor_mode == 2) scroll_w->anchor_mode = 0;
                for(int i = 0; i < 4; ++i) ui_system.measure(scroll_w->self);

                break;
            }
        }
    }
}

void chat_system::rebuild() {
    rebuild_flag = false;

    auto header_map = pop_headers();

    push_headers(header_map);
}

void chat_system::enable(ulong root_widget) {
    root_id = root_widget;
    enabled = true;
    
    for(auto& mes : json_files[0]["messages"]) {
        std::string sender = mes["sender"];
        std::string content = mes["text"];
        ulong timestamp = mes["timestamp"];

        message message_;
        message_.message = content;
        message_.sender = sender;
        message_.timestamp = timestamp;

        messages.push_back(message_);
    }

    push_headers({});
}

void chat_system::chat_system::disable() {
    axiom::ui_system& ui_system = axiom::global_core.ecs->get_system<axiom::ui_system>();
    
    axiom::widget* column_root = ui_system.widgets[root_id].get();
    axiom::scroll_widget* scroll_root = dynamic_cast<axiom::scroll_widget*>(ui_system.widgets[column_root->parent].get());
    
    auto children = ui_system.get_children(scroll_root->self);
    children.push_back(scroll_root->self);
    
    ui_system.erase(children);

    messages.clear();
    message_map.clear();
    
    enabled = false;
}

chat_system::chat_system() {
    json root_json;

    std::ifstream stream(output_root + "json/messages.json");
    stream >> root_json;

    json_files.push_back(std::move(root_json));
}

std::unordered_map<ulong, ulong> chat_system::pop_headers() {
    auto& ui_system = axiom::global_core.ecs->get_system<axiom::ui_system>();
    auto& root = ui_system.widgets[root_id];

    std::vector<int> remove;
    std::unordered_map<ulong, ulong> headers;

    for(int i = 0; i < root->children.size(); ++i) {
        ulong current_id = root->children[i];
        bool rem = !dynamic_cast<axiom::message_widget*>(ui_system.widgets[current_id].get());
        if(rem) {
            if(i + 1 < root->children.size()) {
                ulong next_id = root->children[i + 1];
                bool n = dynamic_cast<axiom::message_widget*>(ui_system.widgets[next_id].get());

                if(n) {
                    headers.emplace(next_id, current_id);
                } else {
                    ui_system.widgets.erase(current_id);
                }
            }
            remove.push_back(i);
        }
    }

    std::sort(remove.begin(), remove.end());

    for(int i = remove.size() - 1; i >= 0; --i) {
        //ui_system.widgets.erase(root->children[remove[i]]);
        root->children.erase(root->children.begin() + remove[i]);
    }

    return headers;
}

void chat_system::push_headers(std::unordered_map<ulong, ulong> headers) {
    auto& ui_system = axiom::global_core.ecs->get_system<axiom::ui_system>();
    auto& root = ui_system.widgets[root_id];

    ui_system.input_set(root_id);
    
    ulong time_delta = 2.0f * 60.0f * 1000000.0f;
    
    std::vector<glm::vec<2, ulong>> to_insert;

    std::string sender = "";
    ulong timestamp = 0;
    ulong prev = axiom::NULL_WIDGET;
    
    bool inserted_prev = false;
    for(int i = 0; i < root->children.size(); ++i) {
        ulong current_id = root->children[i];

        bool d = dynamic_cast<axiom::message_widget*>(ui_system.widgets[current_id].get());
        if(!d) continue;

        axiom::message_widget* message = dynamic_cast<axiom::message_widget*>(ui_system.widgets[current_id].get());

        //

        message->tail_settings = 0;
        message->tail_size = 0.0f;

        if(message->sender != sender) {
            ulong label;
            if(headers.contains(current_id) && !message->inserted) {
                label = headers[current_id];
            } else {
                if(message->sender == "Averie") {
                    ui_system.position(axiom::position_mode::TOP_RIGHT);
                    ui_system.buffer(vec4(4.0f, 4.0f, 4.0f, 24.0f));

                    //axiom::get_date_time_string(message->timestamp) // std::to_string(message->index)
                    
                    std::string L = "[" + axiom::get_date_time_string(message->timestamp) + "] " + message->sender + " <";
                    label = axiom::text_widget::insert(L, axiom::text_alignment::RIGHT);
                } else {
                    ui_system.position(axiom::position_mode::TOP_LEFT);
                    ui_system.buffer(vec4(4.0f, 4.0f, 4.0f, 24.0f));

                    //

                    std::string L = "> " + message->sender + " [" + axiom::get_date_time_string(message->timestamp) + "]"; 
                    label = axiom::text_widget::insert(L, axiom::text_alignment::LEFT);
                }

                root->children.erase(root->children.end() - 1, root->children.end());
            }

            to_insert.push_back({i, label});
            
            if(prev != axiom::NULL_WIDGET) {
                axiom::message_widget* message = dynamic_cast<axiom::message_widget*>(ui_system.widgets[prev].get());

                message->tail_size = 8.0f;
                if(message->sender == "Averie") message->tail_settings = 2;
                else message->tail_settings = 1;
            }
        } else if(message->timestamp > timestamp + time_delta) {
            ulong label;
            if(headers.contains(current_id) && !message->inserted) {
                label = headers[current_id];
            } else {
                if(message->sender == "Averie") {
                    ui_system.position(axiom::position_mode::TOP_RIGHT);
                    ui_system.buffer(vec4(4.0f, 4.0f, 4.0f, 24.0f));
                    
                    //

                    std::string L = "[" + axiom::get_date_time_string(message->timestamp) + "] " + message->sender + " <"; 
                    label = axiom::text_widget::insert(L, axiom::text_alignment::RIGHT);
                } else {
                    ui_system.position(axiom::position_mode::TOP_LEFT);
                    ui_system.buffer(vec4(4.0f, 4.0f, 4.0f, 24.0f));

                    //

                    std::string L = "> " + message->sender + " [" + axiom::get_date_time_string(message->timestamp) + "]"; 
                    label = axiom::text_widget::insert(L, axiom::text_alignment::LEFT);
                }

                root->children.erase(root->children.end() - 1, root->children.end());
            }

            to_insert.push_back({i, label});

            if(prev != axiom::NULL_WIDGET) {
                axiom::message_widget* message = dynamic_cast<axiom::message_widget*>(ui_system.widgets[prev].get());

                message->tail_size = 8.0f;
                if(message->sender == "Averie") message->tail_settings = 2;
                else message->tail_settings = 1;
            }
        } else if(message->inserted) {
            if(headers.contains(current_id)) {
                ulong label = headers[current_id];
                
                to_insert.push_back({i, label});
            } else inserted_prev = false;
        } else inserted_prev = false;

        sender = message->sender;
        timestamp = message->timestamp;
        prev = root->children[i];

        if(i == root->children.size() - 1) {
            if(prev != axiom::NULL_WIDGET) {
                axiom::message_widget* message = dynamic_cast<axiom::message_widget*>(ui_system.widgets[prev].get());

                message->tail_size = 8.0f;
                if(message->sender == "Averie") message->tail_settings = 2;
                else message->tail_settings = 1;
            }
        }
    }

    std::unordered_set<ulong> del_map;

    ulong inserted = 0;
    for(auto& vec : to_insert) {
        root->children.insert(root->children.begin() + (vec.x + inserted), vec.y);
        del_map.emplace(vec.y);
        ++inserted;
    }
    
    ui_system.buffer(vec4(0.0f));
    ulong spacer = axiom::spacer_widget::insert(vec2(0.0f, 0.0f), vec2(FLT_MAX, 0.0f), false);
    root->children.erase(root->children.end() - 1, root->children.end());
    root->children.insert(root->children.begin(), spacer);

    uint erased = 0;

    for(auto [k, i] : headers) {
        if(!del_map.contains(i)) {
            ui_system.widgets.erase(i);
            ++erased;
        }
    }
}

void chat_system::insert_message(ulong message_root, std::string sender, std::string message, ulong timestamp, vec3 color, ulong map_index, int index) {
    auto& ui_system = axiom::global_core.ecs->get_system<axiom::ui_system>();
    
    auto& root = ui_system.widgets[message_root];

    ui_system.input_set(message_root);
    ui_system.buffer(vec4(4.0f));

    if(sender == "Averie") ui_system.position(axiom::position_mode::TOP_RIGHT);
    else ui_system.position(axiom::position_mode::TOP_LEFT);

    //

    ulong w = axiom::message_widget::insert(sender, timestamp, message, axiom::text_alignment::LEFT, vec2(160, 10000), color, vec2(8.0f), 0);
    
    auto ww = dynamic_cast<axiom::message_widget*>(ui_system.widgets[w].get());
    ww->index = map_index;
    
    message_map.emplace(map_index, w);

    if(index != -1) {
        root->children.erase(root->children.end() - 1);
        root->children.insert(root->children.begin() + index, w);
    }

    rebuild_flag = true;
}

void chat_system::remove_message(ulong message_root, ulong id, ulong map_index) {
    auto& ui_system = axiom::global_core.ecs->get_system<axiom::ui_system>();
    auto& root = ui_system.widgets[message_root];

    int index = 0;
    bool contains = true;
    while(true) {
        if(root->children[index] == id) break;

        ++index;
        if(index >= root->children.size()) {
            contains = false;
            break;
        }
    }

    message_map.erase(map_index);

    if(contains) {
        root->children.erase(root->children.begin() + index);

        rebuild_flag = true;
    }

    ui_system.widgets.erase(id);
}