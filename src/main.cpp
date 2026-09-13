#include <include/core.hpp>
#include <include/render.hpp>
#include <include/ui.hpp>

void render_billboards(uint camera, std::vector<vec3> origins, std::vector<vec4> textures, std::vector<vec4> colors, std::vector<vec2> sizes, ivec2 framebuffer_size, axiom::texture& texture) {
    static axiom::vertices vertices;
    if(!vertices.initialized) vertices.init();

    axiom::transform3d camera_transform = axiom::ecs.get_component<axiom::transform3d>(camera);
    axiom::camera3d& camera_cam = axiom::ecs.get_component<axiom::camera3d>(camera);
    mat4 view = axiom::get_view(camera_cam, camera_transform);
    mat4 proj = axiom::get_proj(camera_cam);

    std::vector<axiom::texture_vertex3d> tvs;

    axiom::transform3d transform;
    transform.position = vec3(0.0f);
    transform.orientation = glm::identity<mat3>();

    mat4 model = axiom::get_model(transform, camera_transform);

    mat4 inv_proj = glm::inverse(proj);

    uint i = 0;
    for(vec3 vvv : origins) {
        vec4 pos = view * model * vec4(vvv, 1.0f);

        //if(pos.z < 0.0f) {
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
            std::vector<axiom::texture_vertex3d> vs;
            vs.push_back(axiom::texture_vertex3d(vec3(-1.0f, -1.0f, 0.0f), vec2(0.0f, 0.0f), color, vec3(0.0f)));
            vs.push_back(axiom::texture_vertex3d(vec3(1.0f, -1.0f, 0.0f), vec2(1.0f, 0.0f), color, vec3(0.0f)));
            vs.push_back(axiom::texture_vertex3d(vec3(-1.0f, 1.0f, 0.0f), vec2(0.0f, 1.0f), color, vec3(0.0f)));
            vs.push_back(axiom::texture_vertex3d(vec3(1.0f, 1.0f, 0.0f), vec2(1.0f, 1.0f), color, vec3(0.0f)));

            vs = {vs[0], vs[1], vs[3], vs[0], vs[3], vs[2]};

            for(auto& v : vs) {
                v.position = vvv + transpose(mat3(view)) * (v.position * vec3(size, 1.0f));

                v.texture = v.texture * tex_range.zw() + tex_range.xy();
            }

            tvs.insert(tvs.end(), vs.begin(), vs.end());
        //}
        ++i;
    }
    
    transform.orientation = glm::identity<mat3>();
    model = axiom::get_model(transform, camera_transform);

    vertices.vertex_buffer_data(tvs.data(), tvs.size(), sizeof(axiom::texture_vertex3d), GL_STREAM_DRAW);
    vertices.add_vertex_attribute(0, 3, GL_FLOAT, false, sizeof(axiom::texture_vertex3d), 0);
    vertices.add_vertex_attribute(1, 2, GL_FLOAT, false, sizeof(axiom::texture_vertex3d), sizeof(float) * 3);
    vertices.add_vertex_attribute(2, 4, GL_FLOAT, false, sizeof(axiom::texture_vertex3d), sizeof(float) * 5);
    vertices.add_vertex_attribute(3, 3, GL_FLOAT, false, sizeof(axiom::texture_vertex3d), sizeof(float) * 9);

    axiom::shader& texture_shader = axiom::get_shader("texture3d");
    vec3 light_dir = vec3(0.0f, 0.0f, 0.0f);
    
    texture_shader.use();
    texture.bind(0);
    vertices.bind();

    axiom::push_uniform(0, &model);
    axiom::push_uniform(1, &view);
    axiom::push_uniform(2, &proj);
    //glUniform1f(3, msystem.light_contrast);

    vertices.draw_vertices_triangles();
}

void render_lines(uint camera, std::vector<vec3> points, std::vector<vec4> colors) {
    static axiom::vertices vertices;
    if(!vertices.initialized) vertices.init();

    axiom::transform3d camera_transform = axiom::ecs.get_component<axiom::transform3d>(camera);
    axiom::camera3d& camera_cam = axiom::ecs.get_component<axiom::camera3d>(camera);
    mat4 view = axiom::get_view(camera_cam, camera_transform);
    mat4 proj = axiom::get_proj(camera_cam);

    std::vector<axiom::color_vertex3d> cvs;

    axiom::transform3d transform;
    transform.position = vec3(0.0f);
    transform.orientation = glm::identity<mat3>();

    mat4 model = axiom::get_model(transform, camera_transform);

    mat4 inv_proj = glm::inverse(proj);

    uint i = 0;
    for(int i = 0; i < points.size(); ++i) {
        axiom::color_vertex3d vertex;
        vertex.position = points[i];
        vertex.color = colors[i];

        cvs.push_back(vertex);
    }
    
    transform.orientation = glm::identity<mat3>();
    model = axiom::get_model(transform, camera_transform);

    vertices.vertex_buffer_data(cvs.data(), cvs.size(), sizeof(axiom::color_vertex3d), GL_STREAM_DRAW);
    vertices.add_vertex_attribute(0, 3, GL_FLOAT, false, sizeof(axiom::color_vertex3d), 0);
    vertices.add_vertex_attribute(1, 4, GL_FLOAT, false, sizeof(axiom::color_vertex3d), sizeof(float) * 3);
    vertices.add_vertex_attribute(2, 3, GL_FLOAT, false, sizeof(axiom::color_vertex3d), sizeof(float) * 7);

    axiom::shader& shader = axiom::get_shader("color3d");
    vec3 light_dir = vec3(0.0f, 0.0f, 0.0f);
    
    shader.use();
    vertices.bind();

    axiom::push_uniform(0, &model);
    axiom::push_uniform(1, &view);
    axiom::push_uniform(2, &proj);
    //glUniform1f(3, msystem.light_contrast);

    vertices.draw_vertices_lines();
}

auto base_render = [](uint camera_entity) {
    axiom::transform3d& camera_transform = axiom::ecs.get_component<axiom::transform3d>(camera_entity);
    axiom::camera3d& camera = axiom::ecs.get_component<axiom::camera3d>(camera_entity);
};

void render_grid(uint camera_entity, axiom::framebuffer& framebuffer) {
    axiom::transform3d& camera_transform = axiom::ecs.get_component<axiom::transform3d>(camera_entity);
    axiom::camera3d& camera = axiom::ecs.get_component<axiom::camera3d>(camera_entity);

    auto& vertices = axiom::get_vertices();
    
    std::vector<vec2> vs = {
        vec2(-1.0f, -1.0f),
        vec2(1.0f, -1.0f),
        vec2(-1.0f, 1.0f),
        vec2(1.0f, 1.0f)
    };

    vs = {vs[0], vs[1], vs[3], vs[0], vs[3], vs[2]};

    vertices.vertex_buffer_data(vs.data(), vs.size(), sizeof(vec2), GL_STATIC_DRAW);
    vertices.add_vertex_attribute(0, 2, GL_FLOAT, false, sizeof(vec2), 0);
    
    axiom::transform3d grid_transform;
    grid_transform.position = vec3(0.0f);
    grid_transform.orientation = glm::identity<mat3>();
    
    mat4 model = axiom::get_model(grid_transform, camera_transform);
    mat4 view = axiom::get_view(camera, camera_transform);
    mat4 proj = axiom::get_proj(camera);

    axiom::get_shader("grid3d").use();

    framebuffer.textures[framebuffer.depth_texture].bind(0);
    vertices.bind();

    axiom::push_uniform(0, &model);
    axiom::push_uniform(1, &view);
    axiom::push_uniform(2, &proj);
    axiom::push_uniform(3, vec3(axiom::max_float));

    vertices.draw_vertices_triangles();
}

struct clip_space {
    vec4 range;
    float radius;
    uint parent = 0xFFFFFFFF;
};
static_assert(sizeof(clip_space) == 24);
static_assert(offsetof(clip_space, range) == 0);
static_assert(offsetof(clip_space, radius) == 16);
static_assert(offsetof(clip_space, parent) == 20);

int main(int argc, char** argv) {
    axiom::window window(ivec2(256), ivec2(512), 0, "Axiom");

    axiom::render_init(&window);  
    axiom::ui_init(&window);

    axiom::ttf_font font = axiom::process_ttf("res/Oxanium-Medium.ttf");
    axiom::ecs.get_system<axiom::ui_system>().fonts.push_back(std::move(font));

    // create and initialize camera
    
    uint camera_entity = axiom::ecs.insert_entity();

    axiom::camera3d cam;
    cam.fov = 90.0f;
    cam.near = 0.01f;
    cam.aspect = vec2(1.0f, 1.0f);
    axiom::ecs.insert_component(camera_entity, cam);

    axiom::transform3d transform;
    transform.position = vec3(0.0f, 0.0f, 8.0f);
    transform.orientation = glm::identity<mat3>();
    axiom::ecs.insert_component(camera_entity, transform);

    // create target callback and pass in camera
    
    auto target_callback = [&window, camera_entity](axiom::render_target& target) {
        target.framebuffer.bind();
        target.framebuffer.clear(vec4(0.0f, 0.0f, 0.0f, 1.0f));
        
        axiom::camera3d& camera = axiom::ecs.get_component<axiom::camera3d>(camera_entity);
        camera.aspect = target.size;

        base_render(camera_entity);

        //
        
        /*
        std::vector<vec3> origins;
        std::vector<vec4> textures;
        std::vector<vec4> colors;
        std::vector<vec2> sizes;

        for(auto& pt : font.glyphs[0].points) {
            origins.push_back(vec3(pt, 5.0f));
            textures.push_back(vec4(48, 96, 5, 5)),
            colors.push_back(vec4(1.0f, 0.25f, 0.25f, 1.0f));
            sizes.push_back(vec2(5.0f, 5.0f));
        }

        render_billboards(camera_entity, origins, textures, colors, sizes, target.size, axiom::get_texture("ui"));
        */
        
        render_grid(camera_entity, target.framebuffer);
    };

    auto widget_callback = [camera_entity, &window](axiom::render_widget* widget) {
        static bool movement_capture = false;
        static float movement_speed = 1.0f;
        static bool cursor_hidden = false;

        axiom::ui_system& ui_system = axiom::ecs.get_system<axiom::ui_system>();
        axiom::transform3d& camera_transform = axiom::ecs.get_component<axiom::transform3d>(camera_entity);
        axiom::camera3d& camera = axiom::ecs.get_component<axiom::camera3d>(camera_entity);
        
        if(ui_system.click_capture == widget->self) {
            if(movement_capture == false) {
                cursor_hidden = window.cursor_hidden;
                window.disable_cursor();

                movement_capture = true;
            }
        } else {
            if(movement_capture) {
                window.show_cursor();
                if(cursor_hidden) window.hide_cursor();

                movement_capture = false;
            }
        }

        if(ui_system.hover_capture == widget->self) {
            if(ui_system.window->scroll_delta != 0.0f) {
                movement_speed *= pow(2, ui_system.window->scroll_delta * 0.5f);
            }
        }

        if(ui_system.click_capture == widget->self) {
            if(movement_capture) {
                glm::vec3 raw_movement = {0, 0, 0};
                float rotate_value = 0.0f;

                vec3 rotate = vec3(0.0f);
                rotate.x = -ui_system.window->cursor_delta.x;
                rotate.y = -ui_system.window->cursor_delta.y;

                if(ui_system.window->input_map[axiom::input_code::KEY_Q]) {
                    rotate.z -= 1;
                }
                if(ui_system.window->input_map[axiom::input_code::KEY_E]) {
                    rotate.z += 1;
                }

                //

                if(ui_system.window->input_map[axiom::input_code::KEY_A]) {
                    raw_movement.x -= 1;
                }
                if(ui_system.window->input_map[axiom::input_code::KEY_D]) {
                    raw_movement.x += 1;
                } 
                if(ui_system.window->input_map[axiom::input_code::KEY_S]) {
                    raw_movement.z += 1;
                }
                if(ui_system.window->input_map[axiom::input_code::KEY_W]) {
                    raw_movement.z -= 1;
                }
                if(ui_system.window->input_map[axiom::input_code::KEY_SPACE]) {
                    raw_movement.y += 1;
                }
                if(ui_system.window->input_map[axiom::input_code::KEY_LEFT_SHIFT]) {
                    raw_movement.y -= 1;
                }

                //

                float len = length(raw_movement);
                if(len != 0.0f) raw_movement = glm::normalize(raw_movement);
                
                glm::vec3 translation_vec = camera_transform.orientation * raw_movement;
                
                vec3 dir = -camera_transform.orientation[2];
                vec3 u = camera_transform.orientation[1];
                glm::mat3 rotate_y_mat = (mat3)glm::rotate(float(2 * axiom::pi * (1.0 / 1024) * rotate.y), glm::normalize(glm::cross(u, dir)));
                glm::mat3 rotate_x_mat = (mat3)glm::rotate(float(2 * axiom::pi * (1.0 / 1024) * rotate.x), u);
                glm::mat3 rotate_z_mat = (mat3)glm::rotate(float(2 * axiom::pi * (1.0 / 128) * rotate.z * (axiom::ecs.delta_time * 60)), dir);

                camera_transform.orientation = rotate_z_mat * rotate_x_mat * rotate_y_mat * camera_transform.orientation;
                camera_transform.position += translation_vec * (float)axiom::ecs.delta_time * movement_speed;
            }
        }
    };

    //

    std::vector<axiom::texture_format> format;
    std::vector<axiom::texture_attachment> attachment;

    axiom::ui_system& ui_system = axiom::ecs.get_system<axiom::ui_system>();

    {
        format = {axiom::texture_format::RGBA8, axiom::texture_format::RGBA8, axiom::texture_format::RGBA8, axiom::texture_format::DEPTHF};
        attachment = {axiom::texture_attachment::COLOR0, axiom::texture_attachment::COLOR1, axiom::texture_attachment::COLOR2, axiom::texture_attachment::DEPTH};
        static axiom::render_target render_target = axiom::render_target::create(
            target_callback,
            ivec2(512),
            format,
            attachment,
            {}
        );

        axiom::screen_widget::insert("AXIOM", axiom::color_magenta, &window);
        axiom::render_widget::insert(&render_target, 0, widget_callback);
    }

    //

    /*
    {
        ui_system.input_reset();
        axiom::window_widget::insert("AXIOM", ivec2(300, 100), ivec2(100), axiom::color_purple);
        axiom::panel_widget::insert();
        axiom::scroll_widget::insert(2.0f, false);

        ui_system.buffer(vec4(6));
        axiom::grid_widget::insert(3);

        ui_system.position(axiom::position_mode::CENTER_LEFT);

        axiom::text_widget::insert("Glyph Index", axiom::text_alignment::LEFT, false);
        axiom::spacer_widget::insert(vec2(0, 0), vec2(axiom::max_float, 0));
        axiom::slider_widget::insert(vec2(512.0f, 10.0f), 4, axiom::color_magenta, vec2(0, font.glyphs.size()), 1, 0, "", 
            [](axiom::slider_widget& widget) {
                widget.text[0]->string = axiom::to_base(int64_t(widget.current_value), 10);

                glyph_index = widget.current_value;
            }
        );
        
        axiom::text_widget::insert("texture res X", axiom::text_alignment::LEFT, false);
        axiom::spacer_widget::insert(vec2(0, 0), vec2(axiom::max_float, 0));
        axiom::slider_widget::insert(vec2(128.0f, 10.0f), 4, axiom::color_magenta, vec2(4, 40), 1, 16, "", 
            [](axiom::slider_widget& widget) {
                widget.text[0]->string = axiom::to_base(int64_t(widget.current_value), 10);

                glyph_size.x = widget.current_value;
            }
        );
        
        axiom::text_widget::insert("texture res Y", axiom::text_alignment::LEFT, false);
        axiom::spacer_widget::insert(vec2(0, 0), vec2(axiom::max_float, 0));
        axiom::slider_widget::insert(vec2(128.0f, 10.0f), 4, axiom::color_magenta, vec2(4, 40), 1, 16, "", 
            [](axiom::slider_widget& widget) {
                widget.text[0]->string = axiom::to_base(int64_t(widget.current_value), 10);

                glyph_size.y = widget.current_value;
            }
        );
    }
    */

    //

    /*
    {
        auto target_callback = [&window, camera_entity](axiom::render_target& target) {
            static axiom::texture texture;

            target.framebuffer.bind();
            target.framebuffer.clear(vec4(1.0f, 1.0f, 1.0f, 1.0f));
            
            std::vector<vec4> vs = {
                vec4(-1.0f, -1.0f, 0.0f, 0.0f),
                vec4(1.0f, -1.0f, 1.0f, 0.0f),
                vec4(1.0f, 1.0f, 1.0f, 1.0f),
                vec4(-1.0f, -1.0f, 0.0f, 0.0f),
                vec4(1.0f, 1.0f, 1.0f, 1.0f),
                vec4(-1.0f, 1.0f, 0.0f, 1.0f),
            };

            vec2 size = gsize;
            vec2 offset = vec2(ivec2(size) % 2) + vec2(ivec2(target.size) % 2) * 0.5f;
            for(vec4& v : vs) {
                v.x = v.x * size.x + offset.x;
                v.y = v.y * size.y + offset.y;

                v.x /= target.size.x;
                v.y /= target.size.y;
            }

            auto& vertices = axiom::get_vertices();

            vertices.vertex_buffer_data(vs.data(), vs.size(), sizeof(vec4), GL_STREAM_DRAW);
            vertices.add_vertex_attribute(0, 2, GL_FLOAT, false, sizeof(vec4), 0);
            vertices.add_vertex_attribute(1, 2, GL_FLOAT, false, sizeof(vec4), sizeof(float) * 2);

            texture.bind(0);

            axiom::get_shader("glyph").use();
            vertices.draw_vertices_triangles();
        };

        std::vector<axiom::texture_format> format = {axiom::texture_format::RGBA8};
        std::vector<axiom::texture_attachment> attachment = {axiom::texture_attachment::COLOR0};
        static axiom::render_target target = axiom::render_target::create(
            target_callback,
            ivec2(512),
            format,
            attachment,
            {}
        );

        ui_system.input_reset();
        axiom::window_widget::insert("AXIOM", ivec2(200, 200), ivec2(400, 100), axiom::color_purple);
        axiom::render_widget::insert(&target, 0);
    }
    */

    {
        std::string lipsum = "Sed aliquet risus eu orci tristique ullamcorper. Nam sit amet leo eu enim dictum efficitur in eu orci. Vivamus quis nulla ac massa tincidunt volutpat. Vestibulum pellentesque mattis enim eget feugiat. Nullam eu tortor non dolor dignissim tristique at ut eros. Aliquam pretium at mi sit amet ornare. In mi massa, finibus eu quam id, malesuada sagittis mauris. Aliquam porttitor tellus ut dolor euismod laoreet. Aliquam viverra ut ipsum eu feugiat. Etiam sit amet porta massa. Cras vestibulum a sem vitae suscipit. Suspendisse eros nulla, volutpat vitae auctor pharetra, vestibulum quis sem. Maecenas malesuada nulla ac erat feugiat, dignissim condimentum quam fringilla. Aenean consequat metus eu leo cursus interdum.";

        ui_system.input_reset();
        axiom::window_widget::insert("AXIOM", ivec2(200, 200), ivec2(400, 100), axiom::color_purple);
        axiom::panel_widget::insert();
        axiom::scroll_widget::insert(6.0f, true);
        ui_system.buffer(vec4(4.0f));
        axiom::column_widget::insert();

        axiom::text_widget::insert(lipsum, axiom::text_alignment::LEFT);
    }

    //
    
    auto main_target_callback = [](axiom::render_target& target) {
        axiom::ui_system& ui_system = axiom::ecs.get_system<axiom::ui_system>();
        auto& vertices = axiom::get_vertices();

        static axiom::storage_buffer storage_buffer;
        if(!storage_buffer.initialized) storage_buffer.init();
        std::vector<clip_space> clip_spaces = {
            clip_space(vec4(0.0f, 0.0f, target.size), 10.0f, 0xFFFFFFFF),
        };
        storage_buffer.buffer_data(clip_spaces.data(), clip_spaces.size() * sizeof(clip_space), GL_STREAM_DRAW);

        //

        vertices.vertex_buffer_data(ui_system.vertices.data(), ui_system.vertices.size(), sizeof(axiom::ui_vertex), GL_STREAM_DRAW);

        vertices.add_vertex_attribute(0, 3, GL_FLOAT, false, sizeof(axiom::ui_vertex), 0);
        vertices.add_vertex_attribute(1, 2, GL_FLOAT, false, sizeof(axiom::ui_vertex), sizeof(float) * 3);
        vertices.add_vertex_attribute(2, 4, GL_FLOAT, false, sizeof(axiom::ui_vertex), sizeof(float) * 5);
        vertices.add_vertex_attribute(3, 4, GL_FLOAT, false, sizeof(axiom::ui_vertex), sizeof(float) * 9);
        vertices.add_vertex_attribute(4, 1, GL_UNSIGNED_INT, false, sizeof(axiom::ui_vertex), sizeof(float) * 13);

        //

        mat3 view_matrix = glm::translate(glm::identity<mat3>(), vec2(-1.0f, -1.0f)) * glm::scale(glm::identity<mat3>(), vec2(2.0f / ui_system.window->size.x, 2.0f / ui_system.window->size.y));
        mat3 trans_matrix = glm::identity<mat3>();

        axiom::get_shader("ui").use();

        ui_system.fonts[0].texture.bind(0);
        axiom::get_texture("ui").bind(1);
        for(int i = 0; i < ui_system.target_textures.size(); ++i) {
            ui_system.target_textures[i]->bind(i + 2);
        }

        storage_buffer.bind(0);

        axiom::push_uniform(0, &view_matrix);
        axiom::push_uniform(1, &trans_matrix);

        vertices.draw_vertices_triangles();
    };
    
    format = {axiom::texture_format::RGBA8, axiom::texture_format::DEPTHF};
    attachment = {axiom::texture_attachment::COLOR0, axiom::texture_attachment::DEPTH};
    axiom::render_target main_target = axiom::render_target::create(
        main_target_callback,
        ivec2(512),
        format,
        attachment,
        {}
    );
    window.attach(&main_target);

    while(!window.should_close) {
        window.poll_events();

        main_target.call();

        axiom::ecs.frame();
    }
}