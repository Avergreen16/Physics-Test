#include <graphicsh.hpp>

#include <window.hpp>
#include <math.hpp>
#include <ecs.hpp>
#include <render.hpp>
#include <ui.hpp>
#include <platform.hpp>
#include <utilities.hpp>

#include <iostream>
#include <consts.hpp>

struct main_system : axiom::system {
    axiom::window* win;
    axiom::vertices vertices;

    std::unordered_map<std::string, axiom::texture_asset> texture_assets;
    std::unordered_map<std::string, axiom::text_asset> text_assets;
    std::unordered_map<std::string, axiom::texture> textures;
    std::unordered_map<std::string, axiom::shader> shaders;

    std::vector<std::unique_ptr<axiom::render_target>> targets;

    uint frames = 0;

    float light_altitude = 50.0f;
    float light_azimuth = 35.0f;

    main_system(axiom::window* win_) {
        win = win_;

        axiom::text_asset vert;
        axiom::text_asset frag;
        axiom::texture_asset texasset;

        vert = axiom::text_asset::load(resource_root + "shaders/ui.vert");
        frag = axiom::text_asset::load(resource_root + "shaders/ui.frag");
        shaders.emplace("ui", std::move(axiom::shader(vert, frag)));

        vert = axiom::text_asset::load(resource_root + "shaders/grid3d.vert");
        frag = axiom::text_asset::load(resource_root + "shaders/grid3d.frag");
        shaders.emplace("grid3d", std::move(axiom::shader(vert, frag)));
        
        vert = axiom::text_asset::load(resource_root + "shaders/color3d.vert");
        frag = axiom::text_asset::load(resource_root + "shaders/color3d.frag");
        shaders.emplace("color3d", std::move(axiom::shader(vert, frag)));

        vert = axiom::text_asset::load(resource_root + "shaders/texture3d.vert");
        frag = axiom::text_asset::load(resource_root + "shaders/texture3d.frag");
        shaders.emplace("textures3d", std::move(axiom::shader(vert, frag)));

        //

        texasset = axiom::texture_asset::load(resource_root + "textures/ui.png");
        textures.emplace("ui", std::move(axiom::texture(texasset, axiom::texture_format::RGBA8)));

        vertices.init();
    }

    void call() {
        std::vector<axiom::fb_tex_params> params = {
            axiom::fb_tex_params(axiom::texture_format::RGBA8, axiom::texture_attachment::COLOR1, 0),
            axiom::fb_tex_params(axiom::texture_format::DEPTH32, axiom::texture_attachment::DEPTH)
        };
        static axiom::framebuffer main_fb(ivec2(100, 100), params);

        main_fb.resize(win->viewport_size);

        // input
        ++frames;

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        if(win->pressed_buttons.contains(axiom::input_code::KEY_F6)) { // screenshot
            ivec2 size = win->viewport_size;

            std::vector<byte> pixels(size.x * size.y * 4);

            glReadPixels(
                0, 0,
                size.x, size.y,
                GL_RGBA,
                GL_UNSIGNED_BYTE,
                pixels.data()
            );

            axiom::texture_asset asset = axiom::texture_asset::load(pixels, size, 4);

            ulong timestamp = axiom::get_timestamp();
            asset.save(output_root + "screenshot" + std::to_string(timestamp) + ".png");
        }

        if(win->pressed_buttons.contains(axiom::input_code::KEY_F11)) { // fullscreen
            if(win->is_fullscreen()) win->make_windowed();
            else {
                win->make_fullscreen();
            }
        }

        main_fb.bind();
        glViewport(0, 0, win->viewport_size.x, win->viewport_size.y);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        //
        
        glEnable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_GEQUAL);
        glClearDepth(0.0f);
        glDepthRange(0, 1);
        glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);
        glDisable(GL_DEPTH_CLAMP);
        glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA, GL_ONE);

        //main_fb.bind();
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, win->viewport_size.x, win->viewport_size.y);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shaders["ui"].use();
        textures["font_axiom_default"].bind(0);
        textures["ui"].bind(1);

        axiom::ui_system& ui_system = ecs->get_system<axiom::ui_system>();
        for(int i = 0; i < ui_system.target_textures.size(); ++i) {
            ui_system.target_textures[i]->bind(i + 2);
        }

        mat3 view_matrix = glm::translate(glm::identity<mat3>(), vec2(-1.0f, -1.0f)) * glm::scale(glm::identity<mat3>(), vec2(2.0f / win->viewport_size.x, 2.0f / win->viewport_size.y));

        mat3 trans_matrix = glm::identity<mat3>();

        vertices.vertex_buffer_data(ui_system.vertices.data(), ui_system.vertices.size(), sizeof(axiom::ui_vertex), GL_STREAM_DRAW);

        vertices.add_vertex_attribute(0, 3, GL_FLOAT, false, sizeof(axiom::ui_vertex), 0);
        vertices.add_vertex_attribute(1, 2, GL_FLOAT, false, sizeof(axiom::ui_vertex), sizeof(float) * 3);
        vertices.add_vertex_attribute(2, 4, GL_FLOAT, false, sizeof(axiom::ui_vertex), sizeof(float) * 5);
        vertices.add_vertex_attribute(3, 4, GL_FLOAT, false, sizeof(axiom::ui_vertex), sizeof(float) * 9);
        vertices.add_vertex_attribute(4, 1, GL_UNSIGNED_INT, false, sizeof(axiom::ui_vertex), sizeof(float) * 13);

        vertices.bind();

        glUniformMatrix3fv(0, 1, false, &view_matrix[0][0]);
        glUniformMatrix3fv(1, 1, false, &trans_matrix[0][0]);

        vertices.draw_vertices(GL_TRIANGLES);

        //

        {
            std::vector<axiom::texture_vertex3d> vs = {
                axiom::texture_vertex3d(vec3(-1.0f, -1.0f, 0.5f), vec2(0.0f, 0.0f), vec4(1.0f), vec3(0.0f)),
                axiom::texture_vertex3d(vec3(1.0f, -1.0f, 0.5f), vec2(win->viewport_size.x, 0.0f), vec4(1.0f), vec3(0.0f)),
                axiom::texture_vertex3d(vec3(-1.0f, 1.0f, 0.5f), vec2(0.0f, win->viewport_size.y), vec4(1.0f), vec3(0.0f)),
                axiom::texture_vertex3d(vec3(1.0f, 1.0f, 0.5f), vec2(win->viewport_size.x, win->viewport_size.y), vec4(1.0f), vec3(0.0f)),
            };
            vs = {vs[0], vs[1], vs[3], vs[0], vs[3], vs[2]};

            shaders["texture3d"].use();
            main_fb.textures[0].bind(0);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            axiom::ui_system& ui_system = ecs->get_system<axiom::ui_system>();
            for(int i = 0; i < ui_system.target_textures.size(); ++i) {
                ui_system.target_textures[i]->bind(i + 2);
            }

            mat4 model_matrix = glm::identity<mat4>();
            mat4 view_matrix = glm::identity<mat4>();
            mat4 proj_matrix = glm::identity<mat4>();

            vertices.vertex_buffer_data(vs.data(), vs.size(), sizeof(axiom::texture_vertex3d), GL_STREAM_DRAW);

            vertices.add_vertex_attribute(0, 3, GL_FLOAT, false, sizeof(axiom::texture_vertex3d), 0);
            vertices.add_vertex_attribute(1, 2, GL_FLOAT, false, sizeof(axiom::texture_vertex3d), sizeof(float) * 3);
            vertices.add_vertex_attribute(2, 4, GL_FLOAT, false, sizeof(axiom::texture_vertex3d), sizeof(float) * 5);
            vertices.add_vertex_attribute(3, 3, GL_FLOAT, false, sizeof(axiom::texture_vertex3d), sizeof(float) * 9);

            vertices.bind();

            glUniformMatrix4fv(0, 1, false, &model_matrix[0][0]);
            glUniformMatrix4fv(1, 1, false, &view_matrix[0][0]);
            glUniformMatrix4fv(2, 1, false, &proj_matrix[0][0]);

            vertices.draw_vertices(GL_TRIANGLES);
        }

        glfwSwapBuffers(win->window_handle);
    }
};

void create_ui() {
    auto* ui_system = &axiom::global_core.ecs->get_system<axiom::ui_system>();
    auto* msystem = &axiom::global_core.ecs->get_system<main_system>();

    axiom::screen_widget::insert("Axiom", axiom::color_magenta, msystem->win);
    
    axiom::panel_widget::insert();
    axiom::scroll_widget::insert(8.0f, true);

    ui_system->buffer(vec4(6.0f));

    axiom::grid_widget::insert(3);
    axiom::text_widget::insert("hello world", axiom::text_alignment::LEFT, true);
    axiom::spacer_widget::insert(vec2(0.0f), vec2(axiom::max_float, 0.0f));
    axiom::slider_widget::insert(vec2(192, 16), 8, axiom::color_magenta, vec2(0.0f, 360.0f), 0.0f, 1.0f, "", 
        [](axiom::slider_widget& w) {
            w.text[0]->string = axiom::to_base(w.current_value, 10, 3);
        }
    );
};

int main(int argc, char* argv[]) {
    axiom::window win = axiom::window(ivec2(64, 64), ivec2(512, 512), 6, "axiom test");

    //

    axiom::font_asset default_font = axiom::font_asset::load(resource_root + "fonts/axiom_default.bdf");
    axiom::texture font_tex = std::move(axiom::texture(default_font.texture, axiom::texture_format::RGBA8));

    //

    win.hide_cursor();

    axiom::ecs ecs;
    ecs.make_active();
    
    axiom::ui_system ui_system(&win);
    ui_system.font_assets = {&default_font};
    ecs.register_system(ui_system);

    main_system msystem(&win);
    msystem.textures.emplace("font_axiom_default", std::move(font_tex));
    ecs.register_system(msystem);

    create_ui();

    win.on_resize = [&win]() {
        win.clear_events();

        axiom::get_system<axiom::ui_system>().call();
        axiom::get_system<main_system>().call();
    };

    while(!win.should_close) {
        axiom::prof.start_frame();

        win.poll_events();

        ecs.do_frame();

        axiom::prof.end_frame();
    }
}