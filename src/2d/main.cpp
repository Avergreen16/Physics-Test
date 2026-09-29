#include <include/core.hpp>
#include <include/render.hpp>
#include <include/ui.hpp>
#include <include/physics2d.hpp>

void render_billboards(uint camera, std::vector<vec2> origins, std::vector<vec4> textures, std::vector<vec4> colors, std::vector<vec2> sizes, ivec2 framebuffer_size, axiom::texture& texture) {
    static axiom::vertices vertices;
    if (!vertices.initialized)
        vertices.init();

    axiom::transform2d camera_transform = axiom::ecs.get_component<axiom::transform2d>(camera);
    axiom::camera2d& camera_cam = axiom::ecs.get_component<axiom::camera2d>(camera);
    mat4 view = axiom::get_view(camera_cam, camera_transform);
    mat4 proj = axiom::get_proj(camera_cam);

    std::vector<axiom::texture_vertex2d> tvs;

    mat4 inv_proj = glm::inverse(proj);

    uint i = 0;
    for(vec2 vvv : origins) {
        vec4 pos = view * vec4(vvv, 0.0f, 1.0f);

        // if(pos.z < 0.0f) {
        pos = proj * pos;
        pos /= pos.w;

        vec2 size = sizes[i] / vec2(framebuffer_size);

        vec4 p = vec4(size, pos.z, 1.0f);
        p = inv_proj * p;
        p /= p.w;

        size = glm::abs(p.xy());

        //

        vec4 tex_range = textures[i];
        vec4 color = colors[i];

        //
        std::vector<axiom::texture_vertex2d> vs;
        vs.push_back(axiom::texture_vertex2d(vec2(-1.0f, -1.0f), vec2(0.0f, 0.0f), color));
        vs.push_back(axiom::texture_vertex2d(vec2(1.0f, -1.0f), vec2(1.0f, 0.0f), color));
        vs.push_back(axiom::texture_vertex2d(vec2(-1.0f, 1.0f), vec2(0.0f, 1.0f), color));
        vs.push_back(axiom::texture_vertex2d(vec2(1.0f, 1.0f), vec2(1.0f, 1.0f), color));

        vs = {vs[0], vs[1], vs[3], vs[0], vs[3], vs[2]};

        for(auto& v : vs) {
            v.position = vvv + transpose(mat2(view)) * (v.position * size);

            v.texture = v.texture * tex_range.zw() + tex_range.xy();
        }

        tvs.insert(tvs.end(), vs.begin(), vs.end());
        //}
        ++i;
    }

    vertices.vertex_buffer_data(tvs.data(), tvs.size(), sizeof(axiom::texture_vertex2d), GL_STREAM_DRAW);
    vertices.add_vertex_attribute(0, 2, GL_FLOAT, false, sizeof(axiom::texture_vertex2d), 0);
    vertices.add_vertex_attribute(1, 2, GL_FLOAT, false, sizeof(axiom::texture_vertex2d), sizeof(float) * 2);
    vertices.add_vertex_attribute(2, 4, GL_FLOAT, false, sizeof(axiom::texture_vertex2d), sizeof(float) * 4);

    axiom::shader& texture_shader = axiom::get_shader("texture2d");
    vec3 light_dir = vec3(0.0f, 0.0f, 0.0f);

    texture_shader.use();
    texture.bind(0);
    vertices.bind();

    mat4 model = glm::identity<mat4>();

    axiom::push_uniform(0, &model);
    axiom::push_uniform(1, &view);
    axiom::push_uniform(2, &proj);
    // glUniform1f(3, msystem.light_contrast);

    vertices.draw_vertices_triangles();
}

auto base_render = [](uint camera_entity, axiom::framebuffer& framebuffer) {
    axiom::transform2d& camera_transform = axiom::ecs.get_component<axiom::transform2d>(camera_entity);
    axiom::camera2d& camera = axiom::ecs.get_component<axiom::camera2d>(camera_entity);

    //

    mat4 view = axiom::get_view(camera, camera_transform);
    mat4 proj = axiom::get_proj(camera);

    float light_contrast = 0.75f;

    // render shape
    
    std::vector<vec2> origins;
    std::vector<vec4> textures;
    std::vector<vec4> colors;
    std::vector<vec2> sizes;

    glDisable(GL_DEPTH_TEST);

    auto& collector_color = axiom::ecs.collectors["color_mesh2d"];

    for(uint entity : collector_color.entities) {
        axiom::transform2d& transform = axiom::ecs.get_component<axiom::transform2d>(entity);
        axiom::color_mesh2d& mesh = axiom::ecs.get_component<axiom::color_mesh2d>(entity);

        mat4 model = axiom::get_model(transform);
        axiom::shader& color_shader = axiom::get_shader("color2d");

        origins.push_back(transform.position);
        textures.push_back(vec4(3, 0, 5, 5));
        colors.push_back(mesh.border[0].color);
        sizes.push_back(vec2(5, 5));

        //

        color_shader.use();

        glUniformMatrix4fv(0, 1, false, &model[0][0]);
        glUniformMatrix4fv(1, 1, false, &view[0][0]);
        glUniformMatrix4fv(2, 1, false, &proj[0][0]);

        mesh.v_tris->draw_vertices_triangles();
        mesh.v_lines->draw_vertices_lines();
    }

    /*
    auto& physics_system = axiom::ecs.get_system<axiom::physics_system2d>();

    for(auto& [id, points] : physics_system.collision_table) {
        for(auto& point : points) {
            axiom::transform2d& ta = axiom::ecs.get_component<axiom::transform2d>(point.a);
            vec2 pa = ta.position + ta.orientation * point.pa; 
            
            origins.push_back(pa);
            textures.push_back(vec4(8, 0, 5, 5));
            colors.push_back(vec4(1.0f, 0.25f, 0.25f, 1.0f));
            sizes.push_back(vec2(5, 5));
            
            vec2 pb = point.pb;
            if(point.b != 0xFFFFFFFF) {
                axiom::transform2d& tb = axiom::ecs.get_component<axiom::transform2d>(point.b);
                pb = tb.position + tb.orientation * point.pb;
            }
            
            origins.push_back(pb);
            textures.push_back(vec4(8, 0, 5, 5));
            colors.push_back(vec4(0.25f, 1.0f, 1.0f, 1.0f));
            sizes.push_back(vec2(5, 5));
        }
    }
    */

    render_billboards(camera_entity, origins, textures, colors, sizes, framebuffer.size, axiom::get_texture("ui"));
};

void render_grid(uint camera_entity, axiom::framebuffer& framebuffer) {
    axiom::transform2d& camera_transform = axiom::ecs.get_component<axiom::transform2d>(camera_entity);
    axiom::camera2d& camera = axiom::ecs.get_component<axiom::camera2d>(camera_entity);

    auto& vertices = axiom::get_vertices();

    std::vector<vec2> vs = {
        vec2(-1.0f, -1.0f),
        vec2(1.0f, -1.0f),
        vec2(-1.0f, 1.0f),
        vec2(1.0f, 1.0f)};

    vs = {vs[0], vs[1], vs[3], vs[0], vs[3], vs[2]};

    vertices.vertex_buffer_data(vs.data(), vs.size(), sizeof(vec2), GL_STATIC_DRAW);
    vertices.add_vertex_attribute(0, 2, GL_FLOAT, false, sizeof(vec2), 0);

    mat4 view = axiom::get_view(camera, camera_transform);
    mat4 proj = axiom::get_proj(camera);

    axiom::get_shader("grid2d").use();

    framebuffer.textures[framebuffer.depth_texture].bind(0);
    vertices.bind();

    axiom::push_uniform(0, &view);
    axiom::push_uniform(1, &proj);

    vertices.draw_vertices_triangles();
}


uint create_capsule(vec2 size, uint num_vertices, vec2 position, mat2 orientation, vec3 color, float mass) {
    std::vector<axiom::vertex_element2d> elements;

    for(int i = 0; i < num_vertices; ++i) {
        float angle = float(i) / (num_vertices - 1) * axiom::pi;

        vec2 pos = vec2(cos(angle), sin(angle));

        vec2 pa = pos * size.x + vec2(0.0f, size.y * 0.5f - size.x);
        vec2 pb = -pos * size.x - vec2(0.0f, size.y * 0.5f - size.x);

        elements.push_back(axiom::vertex_element2d(pa));
        elements.push_back(axiom::vertex_element2d(pb));
    }

    //

    axiom::transform2d transform = {
        position, orientation
    };

    uint entity = axiom::ecs.insert_entity();
    axiom::collider2d collider;
    axiom::collision_shape2d shape;
    shape.mass = mass;

    shape.vertices = elements;
    axiom::create_faces(shape);

    collider.shapes = {
        shape   
    };

    std::vector<vec2> border;
    std::vector<vec2> area;

    vec2 v = axiom::physics_system2d::calculate_inertia(collider);
    transform.position += transform.orientation * v;
    axiom::create_mesh(collider, &border, &area);

    axiom::color_mesh2d mesh;

    for(vec2 v : border) {
        axiom::color_vertex2d vv;
        vv.position = v;
        vv.color = vec4(color, 1.0f);

        mesh.border.push_back(vv);
    }
    
    for(vec2 v : area) {
        axiom::color_vertex2d vv;
        vv.position = v;
        vv.color = vec4(color, 0.25f);

        mesh.area.push_back(vv);
    }

    mesh.v_lines = std::shared_ptr<axiom::vertices>(new axiom::vertices);
    mesh.v_lines->init();
    mesh.v_lines->vertex_buffer_data(mesh.border.data(), mesh.border.size(), sizeof(axiom::color_vertex2d), GL_STATIC_DRAW);
    mesh.v_lines->add_vertex_attribute(0, 2, GL_FLOAT, false, sizeof(axiom::color_vertex2d), 0);
    mesh.v_lines->add_vertex_attribute(1, 4, GL_FLOAT, false, sizeof(axiom::color_vertex2d), sizeof(float) * 2);
    
    mesh.v_tris = std::shared_ptr<axiom::vertices>(new axiom::vertices);
    mesh.v_tris->init();
    mesh.v_tris->vertex_buffer_data(mesh.area.data(), mesh.area.size(), sizeof(axiom::color_vertex2d), GL_STATIC_DRAW);
    mesh.v_tris->add_vertex_attribute(0, 2, GL_FLOAT, false, sizeof(axiom::color_vertex2d), 0);
    mesh.v_tris->add_vertex_attribute(1, 4, GL_FLOAT, false, sizeof(axiom::color_vertex2d), sizeof(float) * 2);

    axiom::ecs.insert_component(entity, transform);
    axiom::ecs.insert_component(entity, collider);
    axiom::ecs.insert_component(entity, mesh);

    return entity;
}

uint create_square(vec2 size, vec2 position, mat2 orientation, vec3 color, float mass) {
    std::vector<axiom::vertex_element2d> elements = {
        axiom::vertex_element2d{vec2(-1.0f, -1.0f) * size * 0.5f},
        axiom::vertex_element2d{vec2(1.0f, -1.0f) * size * 0.5f},
        axiom::vertex_element2d{vec2(-1.0f, 1.0f) * size * 0.5f},
        axiom::vertex_element2d{vec2(1.0f, 1.0f) * size * 0.5f},
    };

    axiom::transform2d transform = {
        position, orientation
    };

    uint entity = axiom::ecs.insert_entity();
    axiom::collider2d collider;
    axiom::collision_shape2d shape;
    shape.mass = mass;

    shape.vertices = elements;
    axiom::create_faces(shape);

    collider.shapes = {
        shape   
    };

    std::vector<vec2> border;
    std::vector<vec2> area;

    vec2 v = axiom::physics_system2d::calculate_inertia(collider);
    transform.position += transform.orientation * v;
    axiom::create_mesh(collider, &border, &area);

    axiom::color_mesh2d mesh;

    for(vec2 v : border) {
        axiom::color_vertex2d vv;
        vv.position = v;
        vv.color = vec4(color, 1.0f);

        mesh.border.push_back(vv);
    }
    
    for(vec2 v : area) {
        axiom::color_vertex2d vv;
        vv.position = v;
        vv.color = vec4(color, 0.25f);

        mesh.area.push_back(vv);
    }

    mesh.v_lines = std::shared_ptr<axiom::vertices>(new axiom::vertices);
    mesh.v_lines->init();
    mesh.v_lines->vertex_buffer_data(mesh.border.data(), mesh.border.size(), sizeof(axiom::color_vertex2d), GL_STATIC_DRAW);
    mesh.v_lines->add_vertex_attribute(0, 2, GL_FLOAT, false, sizeof(axiom::color_vertex2d), 0);
    mesh.v_lines->add_vertex_attribute(1, 4, GL_FLOAT, false, sizeof(axiom::color_vertex2d), sizeof(float) * 2);
    
    mesh.v_tris = std::shared_ptr<axiom::vertices>(new axiom::vertices);
    mesh.v_tris->init();
    mesh.v_tris->vertex_buffer_data(mesh.area.data(), mesh.area.size(), sizeof(axiom::color_vertex2d), GL_STATIC_DRAW);
    mesh.v_tris->add_vertex_attribute(0, 2, GL_FLOAT, false, sizeof(axiom::color_vertex2d), 0);
    mesh.v_tris->add_vertex_attribute(1, 4, GL_FLOAT, false, sizeof(axiom::color_vertex2d), sizeof(float) * 2);

    axiom::ecs.insert_component(entity, transform);
    axiom::ecs.insert_component(entity, collider);
    axiom::ecs.insert_component(entity, mesh);

    return entity;
}

void create_polygon(float rad, int num_vertices, vec2 position, mat2 orientation, vec3 color, float mass) {
    std::vector<axiom::vertex_element2d> elements;
    for(int i = 0; i < num_vertices; ++i) {
        float angle = float(i + 0.5f) / num_vertices * 2.0f * axiom::pi;

        vec2 pos = vec2(cos(angle), sin(angle)) * rad;

        elements.push_back(axiom::vertex_element2d(pos));
    }

    axiom::transform2d transform = {
        position, orientation
    };

    uint entity = axiom::ecs.insert_entity();
    axiom::collider2d collider;
    axiom::collision_shape2d shape;
    shape.mass = mass;

    shape.vertices = elements;
    axiom::create_faces(shape);

    collider.shapes = {
        shape   
    };

    std::vector<vec2> border;
    std::vector<vec2> area;

    vec2 v = axiom::physics_system2d::calculate_inertia(collider);
    transform.position += transform.orientation * v;
    axiom::create_mesh(collider, &border, &area);

    axiom::color_mesh2d mesh;

    for(vec2 v : border) {
        axiom::color_vertex2d vv;
        vv.position = v;
        vv.color = vec4(color, 1.0f);

        mesh.border.push_back(vv);
    }
    
    for(vec2 v : area) {
        axiom::color_vertex2d vv;
        vv.position = v;
        vv.color = vec4(color, 0.25f);

        mesh.area.push_back(vv);
    }

    mesh.v_lines = std::shared_ptr<axiom::vertices>(new axiom::vertices);
    mesh.v_lines->init();
    mesh.v_lines->vertex_buffer_data(mesh.border.data(), mesh.border.size(), sizeof(axiom::color_vertex2d), GL_STATIC_DRAW);
    mesh.v_lines->add_vertex_attribute(0, 2, GL_FLOAT, false, sizeof(axiom::color_vertex2d), 0);
    mesh.v_lines->add_vertex_attribute(1, 4, GL_FLOAT, false, sizeof(axiom::color_vertex2d), sizeof(float) * 2);
    
    mesh.v_tris = std::shared_ptr<axiom::vertices>(new axiom::vertices);
    mesh.v_tris->init();
    mesh.v_tris->vertex_buffer_data(mesh.area.data(), mesh.area.size(), sizeof(axiom::color_vertex2d), GL_STATIC_DRAW);
    mesh.v_tris->add_vertex_attribute(0, 2, GL_FLOAT, false, sizeof(axiom::color_vertex2d), 0);
    mesh.v_tris->add_vertex_attribute(1, 4, GL_FLOAT, false, sizeof(axiom::color_vertex2d), sizeof(float) * 2);

    axiom::ecs.insert_component(entity, transform);
    axiom::ecs.insert_component(entity, collider);
    axiom::ecs.insert_component(entity, mesh);
}

int main(int argc, char **argv) {
    axiom::window window(ivec2(256), ivec2(512), 0, "Axiom", false);

    axiom::ui_init(&window);
    axiom::render_init(&window);
    axiom::physics2d_init();
    //axiom::physics3d_init();

    axiom::render_system& render_system = axiom::ecs.get_system<axiom::render_system>();
    render_system.targets.reserve(5);

    axiom::physics_system2d& physics_system = axiom::ecs.get_system<axiom::physics_system2d>();

    //axiom::ecs.get_system<axiom::ui_system>().font_handler.process_ttf("res/Oxanium-Medium.ttf", "test");

    // create collectors
    axiom::signature sig;
    axiom::collector col;

    sig = axiom::update_signature<axiom::transform2d>();
    axiom::update_signature<axiom::color_mesh2d>(sig);
    col = axiom::collector(sig);
    axiom::ecs.create_collector("color_mesh2d", col);

    //

    // create and initialize camera
    uint camera_entity = axiom::ecs.insert_entity();

    axiom::camera2d cam;
    cam.aspect = vec2(1.0f, 1.0f);
    cam.zoom = 1.0f;

    axiom::transform2d transform;
    transform.position = vec2(0.0f, 0.0f);
    transform.orientation = glm::identity<mat2>();

    axiom::ecs.insert_component(camera_entity, cam);
    axiom::ecs.insert_component(camera_entity, transform);

    // create target callback and pass in camera

    axiom::random32 rand(0xFF1);

    float angle = rand();
    mat2 ori = mat2{
        cos(angle), -sin(angle),
        sin(angle), cos(angle)
    };
    ivec2 num_squares = {32, 32};
    vec2 size = vec2(0.5f);
    vec2 center_pos = vec2(0.0f, 36.0f);
    vec2 sep = size + 0.125f;

    create_square(vec2(128.0f, 0.5f), vec2(0.0f, 0.25f), glm::identity<mat2>(), vec3(1.0f), 0.0f);
    create_square(vec2(0.5, 24.0), vec2(63.75, 12.5), glm::identity<mat2>(), vec3(1.0f), 0.0f);
    create_square(vec2(0.5, 24.0), vec2(-63.75, 12.5), glm::identity<mat2>(), vec3(1.0f), 0.0f);

    for(int y = 0; y < num_squares.y; ++y) {
        for(int x = 0; x < num_squares.x; ++x) {
            vec2 pos = vec2(x, y) - vec2(num_squares) * 0.5f;
            pos *= sep;
            pos = ori * pos;
            pos += center_pos;

            uint num_vertices = rand.next() % 4 + 3;

            vec3 color;

            if(num_vertices == 3) {
                color = axiom::hsv_color(rand() * 0.125f + 0.875f, 0.65f, 1.0f);
            } else if(num_vertices == 4) {
                color = axiom::hsv_color(rand() * 0.125f + 5.875f, 0.65f, 1.0f);
            } else if(num_vertices == 5) {
                color = axiom::hsv_color(rand() * 0.125f + 0.3f, 0.65f, 1.0f);
            } else if(num_vertices == 6) {
                color = axiom::hsv_color(rand() * 0.125f + 3.0f, 0.65f, 1.0f);
            }

            create_polygon(axiom::sqrt2 * size.x * 0.5f, num_vertices, pos, ori, color, 1.0f);

            //create_square(size, pos, ori, vec3(1.0f), 1.0f);
        }
    }

    uint prev = 0.0f;
    uint prev_e = 0;

    for(int i = 0; i < 16; ++i) {
        uint e = create_capsule(vec2(0.125f, 1.25f), 8, vec2(32.0f, 4.0f) + ori * vec2(0.0f, i), ori, axiom::hsv_color(rand() * 0.125f + 2.25f, 0.65f, 1.0f), 1.0f);

        if(i != 0) {
            auto& collider = axiom::ecs.get_component<axiom::collider2d>(e);
            auto& prev_collider = axiom::ecs.get_component<axiom::collider2d>(prev_e);

            collider.non_colliding.emplace(e);
            collider.non_colliding.emplace(prev_e);

            axiom::constraint2d cc;
            cc.a = prev_e;
            cc.b = e;

            axiom::pos_constraint pc;
            pc.a = vec2(0.0f, 0.5f);
            pc.b = vec2(0.0f, -0.5f);
            pc.vs = {vec2(1, 0), vec2(0, 1)};
            pc.is_hold = true;

            cc.pos.push_back(pc);

            physics_system.constraints.push_back(cc);
        }

        prev_e = e;
    }


    //

    static bool do_render_grid = true;

    auto target_callback = [&window, camera_entity](axiom::render_target& target) {
        target.framebuffer.bind();
        target.framebuffer.clear(vec4(0.0f, 0.0f, 0.0f, 1.0f));

        axiom::camera2d& camera = axiom::ecs.get_component<axiom::camera2d>(camera_entity);
        camera.aspect = vec2(target.size) / float(glm::min(target.size.x, target.size.y));

        //render_skybox(camera_entity, target.framebuffer);
        if(do_render_grid) render_grid(camera_entity, target.framebuffer);
        base_render(camera_entity, target.framebuffer);
    };

    auto widget_callback = [camera_entity, &window](axiom::render_widget *self) {
        static vec2 cursor_pos = vec2(0.0f);
        static bool capture = false;
        static uint constraint = 0xFFFFFFFF;
        
        
        axiom::physics_system2d& physics = axiom::ecs.get_system<axiom::physics_system2d>();
        axiom::ui_system& ui_system = axiom::ecs.get_system<axiom::ui_system>();
        
        uint camera = *axiom::ecs.collectors["camera"].entities.begin();

        axiom::transform2d& camera_transform = axiom::ecs.get_component<axiom::transform2d>(camera);
        axiom::camera2d& camera_cam = axiom::ecs.get_component<axiom::camera2d>(camera);
        
        cursor_pos = ui_system.window->cursor_pos;
        cursor_pos = (cursor_pos - (vec2)self->position - (0.5f * (vec2)self->size)) / (0.5f * (vec2)self->size);
        
        mat4 view = axiom::get_view(camera_cam, camera_transform);
        mat4 proj = axiom::get_proj(camera_cam);

        mat4 inv_view = glm::inverse(view);
        mat4 inv_proj = glm::inverse(proj);

        cursor_pos = inv_view * inv_proj * vec4(cursor_pos, 0.0f, 1.0f);
        
        //std::cout << ui_system.text_cursor << "\n";
        if(constraint != 0xFFFFFFFF || ui_system.text_cursor) capture = false;
        if(ui_system.click_capture == self->self) {
            if(ui_system.window->pressed_buttons.contains(axiom::input_code::MOUSE_LEFT) && !ui_system.text_cursor) {
                capture = true;
            }

            //

            if(capture) {
                vec2 delta = ui_system.window->cursor_delta / (0.5f * (vec2)self->size);

                vec2 world_delta = mat4(mat3(inv_view)) * inv_proj * vec4(delta, 0.0f, 1.0f);

                camera_transform.position -= world_delta;
            }

            // update constraint
            if(constraint != 0xFFFFFFFF) {
                axiom::constraint2d& cc = physics.constraints[constraint];
                cc.pos[0].b = cursor_pos;
            }

            if(ui_system.window->pressed_buttons.contains(axiom::input_code::MOUSE_LEFT) && ui_system.window->input_map[axiom::input_code::KEY_LEFT_SHIFT]) {
                if(constraint == 0xFFFFFFFF) {
                    for(uint entity : physics.collectors[0].entities) {
                        axiom::transform2d& transform = axiom::ecs.get_component<axiom::transform2d>(entity);
                        axiom::collider2d& collider = axiom::ecs.get_component<axiom::collider2d>(entity);

                        vec2 rel_point = glm::transpose(transform.orientation) * (cursor_pos - transform.position);

                        bool collide = false;

                        for(axiom::collision_shape2d& cs : collider.shapes) {
                            vec2 rel_point2 = transpose(cs.orientation) * (rel_point - cs.position);

                            collide |= axiom::physics_system2d::collision_point(cs.vertices, rel_point2);

                            if(collide) break;
                        }

                        if(collide) {
                            constraint = physics.constraints.size();

                            //

                            axiom::constraint2d cc;
                            cc.a = entity;

                            axiom::pos_constraint pc;
                            pc.a = rel_point;
                            pc.b = cursor_pos;
                            pc.vs = {vec2(1, 0), vec2(0, 1)};
                            pc.is_hold = true;

                            cc.pos.push_back(pc);

                            physics.constraints.push_back(cc);

                            break;
                        }
                    }
                }
            }
        }

        if(!ui_system.window->input_map[axiom::input_code::MOUSE_LEFT]) {
            if(constraint != 0xFFFFFFFF) {
                physics.constraints.erase(physics.constraints.begin() + constraint);
                constraint = 0xFFFFFFFF;
                capture = false;
            }
        }

        if(ui_system.hover_capture == self->self) {
            float zoom_delta = glm::pow(1.25f, ui_system.window->scroll_delta);
            if(zoom_delta != 1.0f) {
                vec2 offset = camera_transform.position - cursor_pos;
                offset /= zoom_delta;

                camera_transform.position = offset + cursor_pos;
                
                camera_cam.zoom *= zoom_delta;
            }
        }
    };

    //

    std::vector<axiom::texture_format> format;
    std::vector<axiom::texture_attachment> attachment;

    format = {axiom::texture_format::RGBA8, axiom::texture_format::DEPTHF};
    attachment = {axiom::texture_attachment::COLOR0, axiom::texture_attachment::DEPTH};
    axiom::render_target* rt = axiom::render_target::create(
        target_callback,
        ivec2(512),
        format,
        attachment, {}
    );
    
    static axiom::ui_system& ui_system = axiom::ecs.get_system<axiom::ui_system>(); 
    
    {
        axiom::screen_widget::insert("Axiom", axiom::color_red, &window);
        axiom::render_widget::insert(rt, 0, widget_callback);
        //

        ui_system.buffer(vec4(4.0f));
        axiom::column_widget::insert();
        axiom::match_widget::insert(vec4(0.35f, 0.35f, 0.35f, 0.35f), true);
        
        axiom::text_widget::insert(
            "", axiom::text_alignment::LEFT, false,
            [](std::string prev) {
                static double elapsed_time = 0.0f;
                static int frames = 0;

                elapsed_time += axiom::ecs.delta_time;
                ++frames;

                if(elapsed_time > 1.0f) {
                    float fps = frames / elapsed_time;
                    frames = 0;
                    elapsed_time = 0.0;

                    return "FPS: " + axiom::to_base(fps, 10, 3);
                } else {
                    return prev;
                }
            }
        );

        axiom::text_widget::insert(
            "", axiom::text_alignment::LEFT, false,
            [camera_entity](std::string prev) {
                axiom::transform2d& camera_transform = axiom::ecs.get_component<axiom::transform2d>(camera_entity);

                return "position: " + axiom::to_base(camera_transform.position.x, 10, 3) + " " + axiom::to_base(camera_transform.position.y, 10, 3);
            }
        );

        ui_system.input_step();

        //

        ui_system.position(axiom::position_mode::TOP_RIGHT);
        ui_system.buffer(vec4(6.0f));
        axiom::column_widget::insert();
        axiom::row_widget::insert();

        axiom::button_widget::insert(vec2(32.0f), axiom::color_purple, vec4(0, 116, 12, 12), 
            [](axiom::button_widget& self) {
                static bool update = true;
                static bool psym = false;

                auto* physics = &axiom::ecs.get_system<axiom::physics_system2d>();
                auto& parent_widget = ui_system.widgets[self.parent];

                if(self.pressed || ui_system.window->pressed_buttons.contains(axiom::input_code::KEY_F5)) {
                    physics->sim_active = !physics->sim_active;
                    update = true;
                }

                if(update) {
                    update = false;

                    if(physics->sim_active) {
                        auto prev_children = parent_widget->children;
                        parent_widget->children = {self.self};
                        for(ulong child : prev_children) {
                            if(find(parent_widget->children.begin(), parent_widget->children.end(), child) == parent_widget->children.end()) {
                                ui_system.widgets.erase(child);
                            }
                        }

                        self.icon = vec4(0, 116, 12, 12);
                    } else {
                        ui_system.input_set(self.parent);
                        ui_system.buffer(vec4(6.0f));

                        //
                        
                        ulong continue_button = axiom::button_widget::insert(vec2(32.0f), axiom::color_purple, vec4(36, 116, 12, 12), 
                            [physics](axiom::button_widget& self) {
                                if(self.held) {
                                    physics->sim_active = true;
                                } else {
                                    physics->sim_active = false;
                                }
                            }
                        );

                        parent_widget->children.pop_back();
                        parent_widget->children.insert(parent_widget->children.begin(), continue_button);
                        
                        //

                        ulong step_button = axiom::button_widget::insert(vec2(32.0f), axiom::color_purple, vec4(24, 116, 12, 12), 
                            [physics](axiom::button_widget& self) {
                                if(self.pressed) {
                                    physics->physics_loop();
                                }
                            }
                        );

                        parent_widget->children.pop_back();
                        parent_widget->children.insert(parent_widget->children.begin(), step_button);
                        
                        self.icon = vec4(12, 116, 12, 12);
                    }
                }
            }
        );

        ui_system.input_step(2);

        //

        ui_system.input_attach(1);
        ui_system.buffer(vec4(4.0f));
        ui_system.position(axiom::position_mode::CENTER_LEFT);
        axiom::row_widget::insert();
        axiom::button_widget::insert(vec2(40, 16), axiom::color_red, "File", 
            [&](axiom::button_widget& w) {
                if(w.pressed) {
                    static axiom::menu_node node = {
                        .children = {
                            {
                                "Settings", 
                                {},
                                []() {
                                    ivec2 size = uvec2(500, 200);
                                    ivec2 pos = (ui_system.window->size - size) / 2;

                                    ui_system.input_reset();
                                    ui_system.position(axiom::position_mode::TOP_LEFT);
                                    axiom::window_widget::insert("Settings", size, pos, axiom::color_red);
                                    axiom::panel_widget::insert();
                                    axiom::scroll_widget::insert(6.0f, true);
                                    ui_system.buffer(vec4(6.0f));

                                    axiom::grid_widget::insert(3);
                                    ui_system.position(axiom::position_mode::CENTER_LEFT);

                                    axiom::text_widget::insert("Substeps Per Frame", axiom::text_alignment::LEFT, false);
                                    axiom::spacer_widget::insert(vec2(0.0f), vec2(axiom::max_float));
                                    axiom::slider_widget::insert(vec2(256.0f, 16.0f), 8.0f, axiom::color_red, vec2(1, 32), 1.0f, 0.0f, "", 
                                        [](axiom::slider_widget& self) {
                                            if(self.pressed) {
                                                axiom::ecs.get_system<axiom::physics_system2d>().substeps = self.current_value;
                                            } else {
                                                self.current_value = axiom::ecs.get_system<axiom::physics_system2d>().substeps;
                                            }
                                            
                                            self.text[0]->string = axiom::to_base(int64_t(self.current_value), 10);
                                        }
                                    );

                                    axiom::text_widget::insert("Iterations Per Substep", axiom::text_alignment::LEFT, false);
                                    axiom::spacer_widget::insert(vec2(0.0f), vec2(axiom::max_float));
                                    axiom::slider_widget::insert(vec2(256.0f, 16.0f), 8.0f, axiom::color_red, vec2(1, 32), 1.0f, 0.0f, "", 
                                        [](axiom::slider_widget& self) {
                                            if(self.pressed) {
                                                axiom::ecs.get_system<axiom::physics_system2d>().iterations = self.current_value;
                                            } else {
                                                self.current_value = axiom::ecs.get_system<axiom::physics_system2d>().iterations;
                                            }
                                            
                                            self.text[0]->string = axiom::to_base(int64_t(self.current_value), 10);
                                        }
                                    );
                                    
                                    axiom::text_widget::insert("Physics FPS", axiom::text_alignment::LEFT, false);
                                    axiom::spacer_widget::insert(vec2(0.0f), vec2(axiom::max_float));
                                    axiom::slider_widget::insert(vec2(256.0f, 16.0f), 8.0f, axiom::color_red, vec2(8, 128), 0.0f, 0.0f, "", 
                                        [](axiom::slider_widget& self) {
                                            if(self.pressed) {
                                                axiom::ecs.get_system<axiom::physics_system2d>().fps = self.current_value;
                                            } else {
                                                self.current_value = axiom::ecs.get_system<axiom::physics_system2d>().fps;
                                            }
                                            
                                            self.text[0]->string = axiom::to_base(self.current_value, 10, 3);
                                        }
                                    );

                                    /*
                                    axiom::scroll_widget::insert(6.0f, true);
                                    ui_system.buffer(vec4(6.0f));

                                    axiom::grid_widget::insert(3);
                                    ui_system.position(axiom::position_mode::CENTER_LEFT);

                                    axiom::text_widget::insert("Light Altitude", axiom::text_alignment::LEFT, false);
                                    axiom::spacer_widget::insert(vec2(0.0f), vec2(axiom::max_float));
                                    axiom::slider_widget::insert(vec2(256.0f, 16.0f), 8.0f, axiom::color_red, vec2(-90.0f, 90.0f), 0.0f, 35.0f, "", 
                                        [](axiom::slider_widget& self) {
                                            if(self.pressed) {
                                                axiom::ecs.get_system<axiom::render_system>().shadow_renderers[0].altitude = self.current_value;
                                            } else {
                                                self.current_value = axiom::ecs.get_system<axiom::render_system>().shadow_renderers[0].altitude;
                                            }
                                            
                                            self.text[0]->string = axiom::to_base(self.current_value, 10, 3);
                                        }
                                    );

                                    axiom::text_widget::insert("Light Azimuth", axiom::text_alignment::LEFT, false);
                                    axiom::spacer_widget::insert(vec2(0.0f), vec2(axiom::max_float));
                                    axiom::slider_widget::insert(vec2(256.0f, 16.0f), 8.0f, axiom::color_red, vec2(-180.0f, 180.0f), 0.0f, 0.0f, "", 
                                        [](axiom::slider_widget& self) {
                                            if(self.pressed) {
                                                axiom::ecs.get_system<axiom::render_system>().shadow_renderers[0].azimuth = self.current_value;
                                            } else {
                                                self.current_value = axiom::ecs.get_system<axiom::render_system>().shadow_renderers[0].azimuth;
                                            }
                                            
                                            self.text[0]->string = axiom::to_base(self.current_value, 10, 3);
                                        }
                                    );
                                    
                                    axiom::text_widget::insert("Light Contrast", axiom::text_alignment::LEFT, false);
                                    axiom::spacer_widget::insert(vec2(0.0f), vec2(axiom::max_float));
                                    axiom::slider_widget::insert(vec2(256.0f, 16.0f), 8.0f, axiom::color_red, vec2(0.0f, 1.0f), 0.0f, 1.0f, "", 
                                        [](axiom::slider_widget& self) {
                                            if(self.pressed) {
                                                axiom::ecs.get_system<axiom::render_system>().shadow_renderers[0].contrast = self.current_value;
                                            } else {
                                                self.current_value = axiom::ecs.get_system<axiom::render_system>().shadow_renderers[0].contrast;
                                            }

                                            self.text[0]->string = axiom::to_base(self.current_value, 10, 3);
                                        }
                                    );
                                    
                                    
                                    axiom::text_widget::insert("Pixel Size", axiom::text_alignment::LEFT, false);
                                    axiom::spacer_widget::insert(vec2(0.0f), vec2(axiom::max_float));
                                    axiom::slider_widget::insert(vec2(256.0f, 16.0f), 8.0f, axiom::color_red, vec2(1.0f, 64.0), 0.0f, 16.0f, "", 
                                        [](axiom::slider_widget& self) {
                                            if(self.pressed) {
                                                axiom::ecs.get_system<axiom::render_system>().shadow_renderers[0].base_pixel_size = 1.0f / self.current_value;
                                            } else {
                                                self.current_value = 1.0f / axiom::ecs.get_system<axiom::render_system>().shadow_renderers[0].base_pixel_size;
                                            }

                                            self.text[0]->string = axiom::to_base(self.current_value, 10, 3);
                                        }
                                    );
                                    
                                    axiom::text_widget::insert("Cascade Scale", axiom::text_alignment::LEFT, false);
                                    axiom::spacer_widget::insert(vec2(0.0f), vec2(axiom::max_float));
                                    axiom::slider_widget::insert(vec2(256.0f, 16.0f), 8.0f, axiom::color_red, vec2(2.0f, 16.0f), 0.0f, 8.0f, "", 
                                        [](axiom::slider_widget& self) {
                                            if(self.pressed) {
                                                axiom::ecs.get_system<axiom::render_system>().shadow_renderers[0].cascade_factor = self.current_value;
                                            } else {
                                                self.current_value = axiom::ecs.get_system<axiom::render_system>().shadow_renderers[0].cascade_factor;
                                            }

                                            self.text[0]->string = axiom::to_base(self.current_value, 10, 3);
                                        }
                                    );
                                    
                                    axiom::text_widget::insert("Blend Radius", axiom::text_alignment::LEFT, false);
                                    axiom::spacer_widget::insert(vec2(0.0f), vec2(axiom::max_float));
                                    axiom::slider_widget::insert(vec2(256.0f, 16.0f), 8.0f, axiom::color_red, vec2(0.0f, 3.0f), 0.0f, 1.0f, "", 
                                        [](axiom::slider_widget& self) {
                                            if(self.pressed) {
                                                axiom::ecs.get_system<axiom::render_system>().shadow_renderers[0].blend_radius = self.current_value;
                                            } else {
                                                self.current_value = axiom::ecs.get_system<axiom::render_system>().shadow_renderers[0].blend_radius;
                                            }

                                            self.text[0]->string = axiom::to_base(self.current_value, 10, 3);
                                        }
                                    );
                                    
                                    axiom::text_widget::insert("Grid", axiom::text_alignment::LEFT, false);
                                    axiom::spacer_widget::insert(vec2(0.0f), vec2(axiom::max_float));
                                    ui_system.position(axiom::position_mode::CENTER);
                                    axiom::checkbox_widget::insert(vec2(16.0f), axiom::color_red, false, 
                                        [](axiom::checkbox_widget& self) {
                                            if(self.checked) do_render_grid = true;
                                            else do_render_grid = false;
                                        }
                                    );
                                    */
                                }
                            },
                            {
                                "Lipsum", 
                                {},
                                []() {
                                    std::string lipsum = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Sed pulvinar sit amet urna eget commodo. Morbi pulvinar ac mauris ut tempus. Mauris aliquet ultrices nulla. In feugiat rutrum pulvinar. Nulla dictum nisl nec auctor venenatis. Etiam vel pretium metus, quis auctor tortor. Quisque quam metus, scelerisque id sagittis ac, iaculis vitae leo. Sed dapibus purus dolor, et vestibulum velit sagittis et. Aliquam vel gravida lectus, eget viverra velit. Cras bibendum, risus in bibendum volutpat, tortor dui cursus augue, quis vulputate ante nibh a ante. Nam varius arcu ac felis vestibulum suscipit. Aliquam mauris justo, placerat sit amet tincidunt eu, auctor eu nunc. Donec suscipit arcu et risus sagittis porttitor. Praesent tellus mauris, semper quis dictum sit amet, tristique in nisl. Aenean nec metus feugiat neque porttitor vulputate vel vitae sem.\nQuisque vulputate imperdiet magna ac porttitor. Vivamus eget neque sed purus tempor placerat pharetra vitae felis. Class aptent taciti sociosqu ad litora torquent per conubia nostra, per inceptos himenaeos. Nullam dui justo, tempus ut neque sed, finibus vestibulum eros. Morbi egestas risus non justo rhoncus blandit. Nulla facilisis a lacus vitae rutrum. Praesent facilisis ligula et lacus semper tincidunt. Mauris at urna justo. Vivamus ornare molestie turpis vulputate auctor.\nSuspendisse pellentesque, urna consectetur suscipit dapibus, leo felis scelerisque sapien, nec elementum risus risus ac ipsum. Cras interdum massa neque. In lacinia volutpat ex at pretium. Pellentesque eleifend eu elit eu fermentum. Ut eros erat, viverra vel feugiat vitae, pulvinar id dui. Sed mattis lorem ac sapien eleifend, vitae finibus turpis suscipit. Mauris viverra nunc non eros efficitur, non efficitur odio porttitor. Aenean sed eros vitae tortor hendrerit pretium at a tellus. Nulla vel accumsan justo. Etiam dignissim ac justo nec pharetra. Pellentesque habitant morbi tristique senectus et netus et malesuada fames ac turpis egestas. Aliquam maximus aliquam tempus. Aenean et dui ullamcorper, consectetur arcu non, condimentum est. Vivamus in neque sit amet dolor feugiat sollicitudin at quis felis. In hac habitasse platea dictumst.";

                                    ui_system.input_reset();
                                    ui_system.position(axiom::position_mode::TOP_LEFT);
                                    axiom::window_widget::insert("Axiom", ivec2(200, 200), ivec2(400, 100), axiom::color_red);
                                    axiom::panel_widget::insert();
                                    axiom::scroll_widget::insert(6.0f, true);
                                    ui_system.buffer(vec4(6.0f));
                                    axiom::column_widget::insert();

                                    axiom::text_widget::insert(lipsum, axiom::text_alignment::LEFT);
                                }
                            }
                        }
                    };

                    ui_system.input_reset();
                    ui_system.position(axiom::position_mode::TOP_LEFT);
                    axiom::menu_widget::insert(w.position, 0.5f, axiom::color_red * 0.9f, 160, 20, 80, &node, {});
                }
            }
        );
        axiom::button_widget::insert(vec2(40, 16), axiom::color_red, "Edit");
        axiom::button_widget::insert(vec2(50, 16), axiom::color_red, "Debug");
        axiom::button_widget::insert(vec2(100, 16), axiom::color_red, "Super Secret");
        ui_system.position(axiom::position_mode::TOP_LEFT);
        ui_system.buffer(vec4(0.0f));
    }

    {
        
    }

    //

    auto main_target_callback = [](axiom::render_target& target) {
        axiom::ui_system& ui_system = axiom::ecs.get_system<axiom::ui_system>();
        auto& vertices = axiom::get_vertices();

        static axiom::storage_buffer storage_buffer;
        if (!storage_buffer.initialized)
            storage_buffer.init();

        struct cspace {
            axiom::clip_space space;
            float padding[2];
        };
        static_assert(sizeof(cspace) == 32);

        std::vector<cspace> spaces;
        for(auto& cs : ui_system.clip_spaces) {
            spaces.push_back({cs});
        }
        storage_buffer.buffer_data(spaces.data(), spaces.size() * sizeof(cspace), GL_STREAM_DRAW);

        //

        vertices.vertex_buffer_data(ui_system.vertices.data(), ui_system.vertices.size(), sizeof(axiom::ui_vertex), GL_STREAM_DRAW);

        vertices.add_vertex_attribute(0, 3, GL_FLOAT, false, sizeof(axiom::ui_vertex), 0);
        vertices.add_vertex_attribute(1, 2, GL_FLOAT, false, sizeof(axiom::ui_vertex), sizeof(float) * 3);
        vertices.add_vertex_attribute(2, 4, GL_FLOAT, false, sizeof(axiom::ui_vertex), sizeof(float) * 5);
        vertices.add_vertex_attribute(3, 1, GL_UNSIGNED_INT, false, sizeof(axiom::ui_vertex), sizeof(float) * 9);
        vertices.add_vertex_attribute(4, 1, GL_UNSIGNED_INT, false, sizeof(axiom::ui_vertex), sizeof(float) * 10);

        //

        mat3 view_matrix = glm::translate(glm::identity<mat3>(), vec2(-1.0f, -1.0f)) * glm::scale(glm::identity<mat3>(), vec2(2.0f / ui_system.window->size.x, 2.0f / ui_system.window->size.y));
        mat3 trans_matrix = glm::identity<mat3>();

        axiom::get_shader("ui").use();

        ui_system.font_handler.texture.bind(0);
        axiom::get_texture("ui").bind(1);
        for(int i = 0; i < ui_system.target_textures.size(); ++i) {
            ui_system.target_textures[i]->bind(i + 2);
        }

        storage_buffer.bind(0);

        axiom::push_uniform(0, &view_matrix);
        axiom::push_uniform(1, &trans_matrix);

        vertices.draw_vertices_triangles();

        //

        target.framebuffer.textures[1].bind(0);
        target.framebuffer.textures[2].bind(1);

        axiom::get_shader("ui_composite").use();
        glDrawArrays(GL_TRIANGLES, 0, 6);
    };

    format = {axiom::texture_format::RGBA8, axiom::texture_format::RGBA8, axiom::texture_format::RGBA8, axiom::texture_format::DEPTHF};
    attachment = {axiom::texture_attachment::COLOR0, axiom::texture_attachment::COLOR1, axiom::texture_attachment::COLOR2, axiom::texture_attachment::DEPTH};
    axiom::render_target* main_target = axiom::render_target::create(
        main_target_callback,
        ivec2(512),
        format,
        attachment, {}
    );
    window.attach(main_target);

    while (!window.should_close) {
        window.poll_events();

        axiom::ecs.frame();
    }
}

//