#include <graphicsh.hpp>

#include <window.hpp>
#include <math.hpp>
#include <ecs.hpp>
#include <render.hpp>
#include <ui.hpp>
#include <platform.hpp>
#include <utilities.hpp>
#include <scene.hpp>
#include <physics-2d.hpp>
#include <physics-3d.hpp>

#include <nlohmann/json.hpp>

#include <chat.hpp>
#include <world_gen.hpp>

#include <iostream>

#include <consts.hpp>

bool render_grid = true;
uint render_points_shape = axiom::NULL_ENTITY;

mat3 random_orientation(axiom::random32& rand) {
    glm::quat q = {rand() * 2.0f - 1.0f, rand() * 2.0f - 1.0f, rand() * 2.0f - 1.0f, rand() * 2.0f - 1.0f};

    q = glm::normalize(q);

    return mat3(q);
}

struct world_params {
    uint seed;

    vec3 radii;
    vec3 position;
    mat3 orientation;

    uint num_chunks;
    uint num_tiles;
};

mat4 get_matrix(vec3 y, vec3 z, vec3 origin);
void make_world(world_params params);

struct main_system : axiom::system {
    axiom::window* win;
    axiom::vertices vertices;

    std::unordered_map<std::string, axiom::texture_asset> texture_assets;
    std::unordered_map<std::string, axiom::text_asset> text_assets;
    std::unordered_map<std::string, axiom::texture> textures;
    std::unordered_map<std::string, axiom::shader> shaders;

    std::vector<std::unique_ptr<axiom::render_target>> targets;

    uint frames = 0;

    float light_altitude = 0.0f;
    float light_azimuth = 0.0f;

    main_system(axiom::window* win_) {
        win = win_;

        axiom::text_asset vert;
        axiom::text_asset frag;
        axiom::texture_asset texasset;

        vert = axiom::text_asset::load(resource_root + "shaders/ui.vert");
        frag = axiom::text_asset::load(resource_root + "shaders/ui.frag");
        shaders.emplace("ui", std::move(axiom::shader(vert, frag)));

        vert = axiom::text_asset::load(resource_root + "shaders/grid.vert");
        frag = axiom::text_asset::load(resource_root + "shaders/grid.frag");
        shaders.emplace("grid", std::move(axiom::shader(vert, frag)));
        
        vert = axiom::text_asset::load(resource_root + "shaders/grid3d.vert");
        frag = axiom::text_asset::load(resource_root + "shaders/grid3d.frag");
        shaders.emplace("grid3d", std::move(axiom::shader(vert, frag)));
        
        vert = axiom::text_asset::load(resource_root + "shaders/color.vert");
        frag = axiom::text_asset::load(resource_root + "shaders/color.frag");
        shaders.emplace("color", std::move(axiom::shader(vert, frag)));
        
        vert = axiom::text_asset::load(resource_root + "shaders/color3d.vert");
        frag = axiom::text_asset::load(resource_root + "shaders/color3d.frag");
        shaders.emplace("color3d", std::move(axiom::shader(vert, frag)));

        vert = axiom::text_asset::load(resource_root + "shaders/texture3d.vert");
        frag = axiom::text_asset::load(resource_root + "shaders/texture3d.frag");
        shaders.emplace("texture3d", std::move(axiom::shader(vert, frag)));
        
        vert = axiom::text_asset::load(resource_root + "shaders/shadow.vert");
        frag = axiom::text_asset::load(resource_root + "shaders/shadow.frag");
        shaders.emplace("shadow", std::move(axiom::shader(vert, frag)));
        
        vert = axiom::text_asset::load(resource_root + "shaders/test.vert");
        frag = axiom::text_asset::load(resource_root + "shaders/test.frag");
        shaders.emplace("test", std::move(axiom::shader(vert, frag)));
        
        vert = axiom::text_asset::load(resource_root + "shaders/texture_range3d.vert");
        frag = axiom::text_asset::load(resource_root + "shaders/texture_range3d.frag");
        shaders.emplace("texture_range3d", std::move(axiom::shader(vert, frag)));

        //

        texasset = axiom::texture_asset::load(resource_root + "textures/ui.png");
        textures.emplace("ui", std::move(axiom::texture(texasset, axiom::texture_format::RGBA8)));

        texasset = axiom::texture_asset::load(resource_root + "textures/tilesheet.png");
        textures.emplace("tilesheet", std::move(axiom::texture(texasset, axiom::texture_format::RGBA8)));
        
        texasset = axiom::texture_asset::load(resource_root + "textures/test.png");
        textures.emplace("test", std::move(axiom::texture(texasset, axiom::texture_format::RGBA8)));

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

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
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

        main_fb.bind();
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
        
        /*
        struct ui_vertex {
            vec3 pos;
            vec2 tex_pos;
            vec4 color = vec4(1.0f);
            vec4 range = vec4(-FLT_MAX, -FLT_MAX, FLT_MAX, FLT_MAX);
            uint data = 0;
        };
        */

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

axiom::shader& get_shader(std::string name) {
    return axiom::get_system<main_system>().shaders[name];
}

axiom::texture& get_texture(std::string name) {
    return axiom::get_system<main_system>().textures[name];
}

void get_bounding_box(std::vector<vec3> vertices, vec3& min, vec3& max) {
    min = vec3(axiom::max_float);
    max = vec3(-axiom::max_float);

    for(vec3 v : vertices) {
        min = glm::min(min, v);
        max = glm::max(max, v);
    }
}

std::vector<vec3> get_vertices(std::vector<vec3> directions, float near, float far) {
    return {
        directions[0] * near / -directions[0].z,
        directions[1] * near / -directions[1].z,
        directions[2] * near / -directions[2].z,
        directions[3] * near / -directions[3].z,

        directions[0] * far / -directions[0].z,
        directions[1] * far / -directions[1].z,
        directions[2] * far / -directions[2].z,
        directions[3] * far / -directions[3].z,
    };
}

struct shadow_map {
    mat4 model = glm::identity<mat3>();
    mat4 view;
    mat4 proj;

    axiom::framebuffer framebuffer;
};

void render_billboards(uint camera, std::vector<vec3> origins, std::vector<vec4> textures, std::vector<vec4> colors, std::vector<vec2> sizes, ivec2 framebuffer_size, axiom::texture& texture) {
    static axiom::vertices vertices;
    if(!vertices.initialized) vertices.init();

    axiom::transform3d camera_transform = axiom::get_component<axiom::transform3d>(camera);
    axiom::camera3d& camera_cam = axiom::get_component<axiom::camera3d>(camera);
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

        if(pos.z < 0.0f) {
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
        }
        ++i;
    }
    
    transform.orientation = glm::identity<mat3>();
    model = axiom::get_model(transform, camera_transform);

    vertices.vertex_buffer_data(tvs.data(), tvs.size(), sizeof(axiom::texture_vertex3d), GL_STREAM_DRAW);
    vertices.add_vertex_attribute(0, 3, GL_FLOAT, false, sizeof(axiom::texture_vertex3d), 0);
    vertices.add_vertex_attribute(1, 2, GL_FLOAT, false, sizeof(axiom::texture_vertex3d), sizeof(float) * 3);
    vertices.add_vertex_attribute(2, 4, GL_FLOAT, false, sizeof(axiom::texture_vertex3d), sizeof(float) * 5);
    vertices.add_vertex_attribute(3, 3, GL_FLOAT, false, sizeof(axiom::texture_vertex3d), sizeof(float) * 9);

    main_system& msystem = axiom::get_system<main_system>();

    axiom::shader& texture_shader = msystem.shaders["texture3d"];
    vec3 light_dir = vec3(0.0f, 0.0f, 0.0f);
    
    texture_shader.use();
    texture.bind(0);
    vertices.bind();

    glUniformMatrix4fv(0, 1, false, &model[0][0]);
    glUniformMatrix4fv(1, 1, false, &view[0][0]);
    glUniformMatrix4fv(2, 1, false, &proj[0][0]);
    glUniform3fv(3, 1, &light_dir.x);

    vertices.draw_vertices(GL_TRIANGLES);
}

void shadow_render(std::vector<shadow_map>& shadow_maps, uint camera) {
    static uint texture_width = 2048;
    static const float pixel_size = 1.0f / 48.0f;
    static const float vgrowth = 8.0f;
    static const int vmaps = shadow_maps.size();
    glEnable(GL_DEPTH_CLAMP);

    auto vfunc = [](int t) {
        return pixel_size * texture_width * pow(8, t);
    };

    axiom::transform3d camera_transform = axiom::get_component<axiom::transform3d>(camera);
    axiom::camera3d& camera_cam = axiom::get_component<axiom::camera3d>(camera);

    main_system& msystem = axiom::get_system<main_system>();
    mat3 orientation = glm::identity<mat3>();
    orientation = (mat3)glm::rotate(-(msystem.light_altitude - 90.0f) / 360.0f * 2.0f * axiom::pi, vec3(0.0f, 1.0f, 0.0f)) * orientation;
    orientation = (mat3)glm::rotate(msystem.light_azimuth / 360.0f * 2.0f * axiom::pi, vec3(0.0f, 0.0f, 1.0f)) * orientation;

    std::vector<vec3> frustum = {
        vec3(-1.0f, -1.0f, 0.5f),
        vec3(1.0f, -1.0f, 0.5f),
        vec3(-1.0f, 1.0f, 0.5f),
        vec3(1.0f, 1.0f, 0.5f),
    };

    mat4 inv_proj = glm::inverse(axiom::get_proj(camera_cam));
    for(vec3& v : frustum) {
        vec4 vv = inv_proj * vec4(v, 1.0f);
        vv /= vv.w;

        v = glm::normalize(vv.xyz());
    }

    for(auto& map : shadow_maps) map.framebuffer.resize(ivec2(texture_width));

    //

    float prev = camera_cam.near;

    for(int i = 0; i < shadow_maps.size(); ++i) {
        float width = 1.0f / 16 * texture_width * pow(8.0f, i);

        float texel_size = width / texture_width;
        float boundary = 1.0f;

        float size = width * 0.25f;
        float delta = size;

        while(true) {
            std::vector<vec3> vvs = get_vertices(frustum, prev, size);
            for(vec3& v : vvs) v = transpose(orientation) * camera_transform.orientation * v;

            vec3 min;
            vec3 max;

            get_bounding_box(vvs, min, max);

            vec3 s = max - min;

            if(glm::max(glm::max(s.x, s.y), s.z) > width - boundary) {
                delta *= 0.5f;
                size -= delta;
            } else {
                delta *= 0.5f;
                size += delta;
            }

            if(delta < texel_size * 4) break;
        }
        
        std::vector<vec3> vvs = get_vertices(frustum, camera_cam.near, size);
        for(vec3& v : vvs) v = transpose(orientation) * camera_transform.orientation * v;

        vec3 min;
        vec3 max;
        get_bounding_box(vvs, min, max);
        
        mat3 t = glm::identity<mat3>();//orientation; //
        vec3 vx = t[0];
        vec3 vy = t[1];
        vec3 vz = t[2];

        axiom::transform3d ncamera_transform = camera_transform;

        vec3 center = (min + max) * 0.5f + transpose(orientation) * camera_transform.position;
        
        float texel = texel_size * 4.0f;

        float cx = dot(center, vx);
        float cy = dot(center, vy);
        float cz = dot(center, vz);
        cx = round(cx / texel) * texel;
        cy = round(cy / texel) * texel;
        cz = round(cz / texel) * texel;

        center = cx * vx + cy * vy + cz * vz - transpose(orientation) * camera_transform.position;

        //std::cout << dot((center + ocenter) / texel_size, vx) << " " << dot((center + ocenter) / texel_size, vy) << "\n";



        //

        prev = size;

        shadow_maps[i].proj = axiom::get_ortho_proj_matrix(-width * 0.5f, width * 0.5f, -width * 0.5f, width * 0.5f, -width * 0.5f, width * 0.5f);

        shadow_maps[i].view = glm::translate(-center) * mat4(transpose(orientation));

        // render

        shadow_maps[i].framebuffer.bind();

        glEnable(GL_DEPTH_TEST);
    
        auto& collector = axiom::global_core.ecs->collectors["color_mesh3d"];

        for(uint entity : collector.entities) {
            axiom::transform3d& transform = axiom::get_component<axiom::transform3d>(entity);
            axiom::color_mesh3d& mesh = axiom::get_component<axiom::color_mesh3d>(entity);

            mat4 model = axiom::get_model(transform, camera_transform);
            axiom::shader& color_shader = msystem.shaders["color3d"];

            //
            
            color_shader.use();
            
            glUniformMatrix4fv(0, 1, false, &model[0][0]);
            glUniformMatrix4fv(1, 1, false, &shadow_maps[i].view[0][0]);
            glUniformMatrix4fv(2, 1, false, &shadow_maps[i].proj[0][0]);

            mesh.vertices->draw_vertices(GL_TRIANGLES);
        }
        
        auto& collector_texture = axiom::global_core.ecs->collectors["texture_mesh3d"];

        for(uint entity : collector_texture.entities) {
            axiom::transform3d& transform = axiom::get_component<axiom::transform3d>(entity);
            axiom::texture_mesh3d& mesh = axiom::get_component<axiom::texture_mesh3d>(entity);

            mat4 model = axiom::get_model(transform, camera_transform);
            axiom::shader& texture_shader = msystem.shaders["texture3d"];

            //
            
            texture_shader.use();
            mesh.texture->bind(0);

            glUniformMatrix4fv(0, 1, false, &model[0][0]);
            glUniformMatrix4fv(1, 1, false, &shadow_maps[i].view[0][0]);
            glUniformMatrix4fv(2, 1, false, &shadow_maps[i].proj[0][0]);

            mesh.vertices->draw_vertices(GL_TRIANGLES);
        }
        
        auto& collector_texture_range = axiom::global_core.ecs->collectors["texture_range_mesh3d"];

        for(uint entity : collector_texture_range.entities) {
            axiom::transform3d& transform = axiom::get_component<axiom::transform3d>(entity);
            axiom::texture_range_mesh3d& mesh = axiom::get_component<axiom::texture_range_mesh3d>(entity);

            mat4 model = axiom::get_model(transform, camera_transform);
            axiom::shader& texture_shader = msystem.shaders["texture_range3d"];

            //
            
            texture_shader.use();
            mesh.texture->bind(0);

            glUniformMatrix4fv(0, 1, false, &model[0][0]);
            glUniformMatrix4fv(1, 1, false, &shadow_maps[i].view[0][0]);
            glUniformMatrix4fv(2, 1, false, &shadow_maps[i].proj[0][0]);

            mesh.vertices->draw_vertices(GL_TRIANGLES);
        }
    }
    glDisable(GL_DEPTH_CLAMP);
}

void base_render(uint camera, axiom::render_target& f) {
    std::vector<axiom::fb_tex_params> params = {
        axiom::fb_tex_params(axiom::texture_format::RGBA8, axiom::texture_attachment::COLOR1, 1),
        axiom::fb_tex_params(axiom::texture_format::DEPTH32, axiom::texture_attachment::DEPTH)
    };

    static std::vector<shadow_map> shadow_maps = [&params]() {
        std::vector<shadow_map> v;
        v.reserve(5);
        
        v.emplace_back(shadow_map(glm::identity<mat4>(), glm::identity<mat4>(), glm::identity<mat4>(), axiom::framebuffer(ivec2(64), params)));
        v.emplace_back(shadow_map(glm::identity<mat4>(), glm::identity<mat4>(), glm::identity<mat4>(), axiom::framebuffer(ivec2(64), params)));
        v.emplace_back(shadow_map(glm::identity<mat4>(), glm::identity<mat4>(), glm::identity<mat4>(), axiom::framebuffer(ivec2(64), params)));
        v.emplace_back(shadow_map(glm::identity<mat4>(), glm::identity<mat4>(), glm::identity<mat4>(), axiom::framebuffer(ivec2(64), params)));
        v.emplace_back(shadow_map(glm::identity<mat4>(), glm::identity<mat4>(), glm::identity<mat4>(), axiom::framebuffer(ivec2(64), params)));

        return v;
    }();

    shadow_render(shadow_maps, camera);

    //

    f.framebuffer.bind();

    main_system& msystem = axiom::get_system<main_system>();
    axiom::ui_system& ui_system = axiom::get_system<axiom::ui_system>();

    vec3 background = axiom::hex_color(0x000000);
    glClearColor(background.x, background.y, background.z, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glBlendFuncSeparatei(
        0,
        GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
        GL_ONE, GL_ONE_MINUS_SRC_ALPHA
    );
    glBlendFuncSeparatei(
        1,
        GL_ONE, GL_ZERO,
        GL_ONE, GL_ZERO
    );
    glBlendFuncSeparatei(
        2,
        GL_ONE, GL_ZERO,
        GL_ONE, GL_ZERO
    );

    //

    glEnable(GL_CULL_FACE);
    
    static axiom::vertices vertices;
    if(!vertices.initialized) {
        vertices.init();
    }

    //

    axiom::transform3d& camera_transform = axiom::get_component<axiom::transform3d>(camera);
    axiom::camera3d& camera_cam = axiom::get_component<axiom::camera3d>(camera);

    camera_cam.aspect = vec2(f.size) / (float)glm::min(f.size.x, f.size.y);
    
    mat4 view = axiom::get_view(camera_cam, camera_transform);
    mat4 proj = axiom::get_proj(camera_cam);

    // render shape

    glEnable(GL_DEPTH_TEST);
    
    auto& collector_color = axiom::global_core.ecs->collectors["color_mesh3d"];

    for(uint entity : collector_color.entities) {
        axiom::transform3d& transform = axiom::get_component<axiom::transform3d>(entity);
        axiom::color_mesh3d& mesh = axiom::get_component<axiom::color_mesh3d>(entity);

        mat4 model = axiom::get_model(transform, camera_transform);
        axiom::shader& color_shader = msystem.shaders["color3d"];
        
        vec3 light_dir = normalize(vec3(1.0f, 1.0f, 1.0f));

        //
        
        color_shader.use();

        glUniformMatrix4fv(0, 1, false, &model[0][0]);
        glUniformMatrix4fv(1, 1, false, &view[0][0]);
        glUniformMatrix4fv(2, 1, false, &proj[0][0]);
        glUniform3fv(3, 1, &light_dir.x);

        mesh.vertices->draw_vertices(GL_TRIANGLES);
    }

    auto& collector_texture = axiom::global_core.ecs->collectors["texture_mesh3d"];

    for(uint entity : collector_texture.entities) {
        axiom::transform3d& transform = axiom::get_component<axiom::transform3d>(entity);
        axiom::texture_mesh3d& mesh = axiom::get_component<axiom::texture_mesh3d>(entity);

        mat4 model = axiom::get_model(transform, camera_transform);
        axiom::shader& texture_shader = msystem.shaders["texture3d"];
        
        vec3 light_dir = normalize(vec3(1.0f, 1.0f, 1.0f));

        //
        
        texture_shader.use();
        mesh.texture->bind(0);

        glUniformMatrix4fv(0, 1, false, &model[0][0]);
        glUniformMatrix4fv(1, 1, false, &view[0][0]);
        glUniformMatrix4fv(2, 1, false, &proj[0][0]);
        glUniform3fv(3, 1, &light_dir.x);

        mesh.vertices->draw_vertices(GL_TRIANGLES);
    }

    auto& collector_texture_range = axiom::global_core.ecs->collectors["texture_range_mesh3d"];

    for(uint entity : collector_texture_range.entities) {
        axiom::transform3d& transform = axiom::get_component<axiom::transform3d>(entity);
        axiom::texture_range_mesh3d& mesh = axiom::get_component<axiom::texture_range_mesh3d>(entity);

        mat4 model = axiom::get_model(transform, camera_transform);
        axiom::shader& texture_shader = msystem.shaders["texture_range3d"];

        //
        
        texture_shader.use();
        mesh.texture->bind(0);

        glUniformMatrix4fv(0, 1, false, &model[0][0]);
        glUniformMatrix4fv(1, 1, false, &view[0][0]);
        glUniformMatrix4fv(2, 1, false, &proj[0][0]);

        mesh.vertices->draw_vertices(GL_TRIANGLES);
    }

    //

    // flag

    glDisable(GL_DEPTH_TEST);

    //

    std::vector<vec2> vs = {
        vec2(-1.0f, -1.0f),
        vec2(1.0f, -1.0f),
        vec2(-1.0f, 1.0f),
        vec2(1.0f, 1.0f)
    };

    vs = {vs[0], vs[1], vs[3], vs[0], vs[3], vs[2]};

    vertices.vertex_buffer_data(vs.data(), vs.size(), sizeof(vec2), GL_STATIC_DRAW);
    vertices.add_vertex_attribute(0, 2, GL_FLOAT, false, sizeof(vec2), 0);

    msystem.shaders["shadow"].use();
    
    std::vector<mat4> models;
    std::vector<mat4> views;
    std::vector<mat4> projs;
    int i = 0;
    for(auto& smap : shadow_maps) {
        views.push_back(smap.view);
        projs.push_back(smap.proj);
        models.push_back(smap.model);

        smap.framebuffer.textures[0].bind(i + 3);
        smap.framebuffer.textures[1].bind(i + 8);
        ++i;
    }

    mat4 model = glm::identity<mat4>();

    glUniformMatrix4fv(0, 1, false, &model[0][0]);
    glUniformMatrix4fv(1, 1, false, &view[0][0]);
    glUniformMatrix4fv(2, 1, false, &proj[0][0]);
    
    glUniformMatrix4fv(3, 5, false, &models[0][0][0]);
    glUniformMatrix4fv(8, 5, false, &views[0][0][0]);
    glUniformMatrix4fv(13, 5, false, &projs[0][0][0]);

    f.framebuffer.textures[1].bind(0);
    f.framebuffer.textures[2].bind(1);
    f.framebuffer.textures[3].bind(2);

    vertices.draw_vertices(GL_TRIANGLES);
    
    glEnable(GL_DEPTH_TEST);

    // render debug points

    std::vector<vec3> normals;

    std::vector<vec3> position;
    std::vector<vec4> tex_range;
    std::vector<vec4> color;
    std::vector<vec2> size;

    auto& psystem3d = axiom::get_system<axiom::physics_system3d>();
    if(render_points_shape != axiom::NULL_ENTITY) {
        for(auto& c : psystem3d.collision_table) {
            if(c.first[0] == render_points_shape || c.first[1] == render_points_shape) {
                auto& manifold = c.second;

                axiom::transform3d ta;
                if(c.first[0] != axiom::NULL_ENTITY) ta = axiom::get_component<axiom::transform3d>(c.first[0]);
                else {
                    ta.orientation = glm::identity<mat3>();
                    ta.position = vec3(0.0f);
                }

                axiom::transform3d tb;
                if(c.first[1] != axiom::NULL_ENTITY) tb = axiom::get_component<axiom::transform3d>(c.first[1]);
                else {
                    tb.orientation = glm::identity<mat3>();
                    tb.position = vec3(0.0f);
                }

                for(auto& mf : manifold) {
                    for(auto& pt : mf.points) {
                        position.push_back(ta.position + ta.orientation * pt.point.a);
                        tex_range.push_back(vec4(3, 0, 5, 5));
                        color.push_back(vec4(1.0f, 0.5f, 0.5f, 1.0f));
                        size.push_back(vec2(5, 5));
                        
                        position.push_back(tb.position + tb.orientation * pt.point.b);
                        tex_range.push_back(vec4(3, 0, 5, 5));
                        color.push_back(vec4(0.5f, 1.0f, 1.0f, 1.0f));
                        size.push_back(vec2(5, 5));

                        normals.push_back(ta.position + ta.orientation * pt.point.a);
                        normals.push_back(ta.position + ta.orientation * pt.point.a + pt.normal * 0.125f);
                        
                        normals.push_back(tb.position + tb.orientation * pt.point.b);
                        normals.push_back(tb.position + tb.orientation * pt.point.b - pt.normal * 0.125f);
                    }
                }
            }
        }
    }

    glDisable(GL_DEPTH_TEST);
    render_billboards(camera, position, tex_range, color, size, f.size, msystem.textures["ui"]);

    //

    std::vector<axiom::color_vertex3d> cvs;

    for(int i = 0; i < normals.size(); ++i) {
        axiom::color_vertex3d cv;
        cv.position = normals[i];
        cv.color = vec4(axiom::hsv_color(1.0f, 0.75f, 1.0f), 1 - i % 2);
        cvs.push_back(cv);
    }

    vertices.vertex_buffer_data(cvs.data(), cvs.size(), sizeof(axiom::color_vertex3d), GL_STREAM_DRAW);
    vertices.add_vertex_attribute(0, 3, GL_FLOAT, false, sizeof(axiom::color_vertex3d), 0);
    vertices.add_vertex_attribute(1, 4, GL_FLOAT, false, sizeof(axiom::color_vertex3d), sizeof(float) * 3);
    vertices.add_vertex_attribute(2, 3, GL_FLOAT, false, sizeof(axiom::color_vertex3d), sizeof(float) * 7);
    
    msystem.shaders["color3d"].use();
    vertices.bind();

    axiom::transform3d tf;
    tf.position = vec3(0.0f);
    tf.orientation = glm::identity<mat3>();

    model = axiom::get_model(tf, camera_transform);

    glUniformMatrix4fv(0, 1, false, &model[0][0]);
    glUniformMatrix4fv(1, 1, false, &view[0][0]);
    glUniformMatrix4fv(2, 1, false, &proj[0][0]);

    vertices.draw_vertices(GL_LINES);
    glEnable(GL_DEPTH_TEST);


    // render grid

    if(render_grid) {
        vertices.vertex_buffer_data(vs.data(), vs.size(), sizeof(vec2), GL_STATIC_DRAW);
        vertices.add_vertex_attribute(0, 2, GL_FLOAT, false, sizeof(vec2), 0);

        //

        axiom::transform3d grid_transform;
        grid_transform.position = vec3(0.0f);
        grid_transform.orientation = glm::identity<mat3>();
        
        model = axiom::get_model(grid_transform, camera_transform);

        axiom::shader& grid_shader = msystem.shaders["grid3d"];

        grid_shader.use();
        vertices.bind();

        f.framebuffer.textures[3].bind(0);

        glUniformMatrix4fv(0, 1, false, &model[0][0]);
        glUniformMatrix4fv(1, 1, false, &view[0][0]);
        glUniformMatrix4fv(2, 1, false, &proj[0][0]);
        glUniform3f(3, axiom::max_float, axiom::max_float, axiom::max_float);

        vertices.draw_vertices(GL_TRIANGLES);

        for(uint camera_entity : axiom::global_core.ecs->collectors["camera"].entities) {
            if(camera_entity != camera) {
                axiom::transform3d transform = axiom::get_component<axiom::transform3d>(camera_entity);
                axiom::camera3d& cam = axiom::get_component<axiom::camera3d>(camera_entity);
                transform.orientation = transpose(mat3(view));

                mat4 model = axiom::get_model(transform, camera_transform);
                vec4 pos = view * model * vec4(0.0f, 0.0f, 0.0f, 1.0f);

                if(pos.z < 0.0f) {
                    pos = proj * pos;
                    pos /= pos.w;

                    vec2 size = vec2(16.0f) / vec2(f.size);

                    vec4 p = vec4(size, pos.z, 0.0f);
                    p /= p.w;

                    size = glm::abs(p.xy());

                    //

                    vec4 tex_range = vec4(96, 80, 16, 16);

                    //

                    std::vector<axiom::texture_vertex3d> vs;
                    vs.push_back(axiom::texture_vertex3d(vec3(-1.0f, -1.0f, 0.0f), vec2(0.0f, 0.0f), vec4(1.0f), vec3(0.0f)));
                    vs.push_back(axiom::texture_vertex3d(vec3(1.0f, -1.0f, 0.0f), vec2(1.0f, 0.0f), vec4(1.0f), vec3(0.0f)));
                    vs.push_back(axiom::texture_vertex3d(vec3(-1.0f, 1.0f, 0.0f), vec2(0.0f, 1.0f), vec4(1.0f), vec3(0.0f)));
                    vs.push_back(axiom::texture_vertex3d(vec3(1.0f, 1.0f, 0.0f), vec2(1.0f, 1.0f), vec4(1.0f), vec3(0.0f)));

                    vs = {vs[0], vs[1], vs[3], vs[0], vs[3], vs[2]};

                    for(auto& v : vs) {
                        v.position *= vec3(size, 1.0f);
                        v.texture = v.texture * tex_range.zw() + tex_range.xy();
                    }

                    //

                    vertices.vertex_buffer_data(vs.data(), vs.size(), sizeof(axiom::texture_vertex3d), GL_STREAM_DRAW);
                    vertices.add_vertex_attribute(0, 3, GL_FLOAT, false, sizeof(axiom::texture_vertex3d), 0);
                    vertices.add_vertex_attribute(1, 2, GL_FLOAT, false, sizeof(axiom::texture_vertex3d), sizeof(float) * 3);
                    vertices.add_vertex_attribute(2, 4, GL_FLOAT, false, sizeof(axiom::texture_vertex3d), sizeof(float) * 5);
                    vertices.add_vertex_attribute(3, 3, GL_FLOAT, false, sizeof(axiom::texture_vertex3d), sizeof(float) * 9);

                    axiom::shader& texture_shader = msystem.shaders["texture3d"];
                    axiom::texture& ui_texture = msystem.textures["ui"];
                    vec3 light_dir = vec3(0.0f, 0.0f, 0.0f);
                    
                    texture_shader.use();
                    ui_texture.bind(0);

                    glUniformMatrix4fv(0, 1, false, &model[0][0]);
                    glUniformMatrix4fv(1, 1, false, &view[0][0]);
                    glUniformMatrix4fv(2, 1, false, &proj[0][0]);
                    glUniform3fv(3, 1, &light_dir.x);

                    vertices.draw_vertices(GL_TRIANGLES);
                }
                
                transform = axiom::get_component<axiom::transform3d>(camera_entity);
                model = axiom::get_model(transform, camera_transform);
            }
        }
    }   
}

std::vector<axiom::shape_face> get_faces(std::vector<axiom::vertex_element3d>& elements) {
    std::vector<axiom::output_vertex> surface_vertices;
    std::vector<uint> surface_indices;
    axiom::create_mesh(elements, &surface_vertices, &surface_indices);

    vec3 center = vec3(0.0f);
    for(auto& element : elements) center += element.center;
    center /= elements.size();

    std::vector<axiom::shape_face> faces;

    for(int i = 0; i < surface_indices.size(); i += 3) {
        uint a = surface_indices[i];
        uint b = surface_indices[i + 1];
        uint c = surface_indices[i + 2];
        a = surface_vertices[a].element;
        b = surface_vertices[b].element;
        c = surface_vertices[c].element;

        vec3 va = elements[a].center;
        vec3 vb = elements[b].center;
        vec3 vc = elements[c].center;

        vec3 normal = glm::normalize(cross(va - vc, vb - vc));
        if(dot(normal, va - center) < 0.0f) normal = -normal;
        
        std::vector<uint> is = {a, b, c};
        std::sort(is.begin(), is.end());

        faces.push_back({is, normal});
    }

    for(int i = 0; i < faces.size(); ++i) {
        for(int j = i + 1; j < faces.size(); ++j) {
            axiom::shape_face face_i = faces[i];
            axiom::shape_face face_j = faces[j];

            if(dot(face_i.normal, face_j.normal) > 0.98f) {
                std::vector<uint> new_i;
                std::set_union(face_i.vertices.begin(), face_i.vertices.end(), face_j.vertices.begin(), face_j.vertices.end(), std::back_inserter(new_i));

                face_i.normal = normalize(face_i.normal + face_j.normal);
                face_i.vertices = new_i;
                faces[i] = face_i;

                faces.erase(faces.begin() + j);
                --j;
            }
        }
    }

    for(auto& face : faces) {
        vec3 center = vec3(0.0f);
        for(uint i : face.vertices) center += elements[i].center;
        center /= float(face.vertices.size());

        vec3 x = glm::normalize(elements[face.vertices[0]].center - center);
        vec3 y = glm::normalize(glm::cross(face.normal, x));

        std::sort(face.vertices.begin(), face.vertices.end(), 
            [center, norm = face.normal, x, y, &elements](const uint& a, const uint& b) {
                vec3 va = elements[a].center - center;
                vec3 vb = elements[b].center - center;

                float sa = dot(va, x);
                float sb = dot(vb, x);
                bool ba = sa >= 0.0f;
                bool bb = sb >= 0.0f;
                if(ba != bb) return sa < sb;
                
                return dot(norm, cross(va, vb)) < 0.0f;
            }
        );
    }

    return faces;
};

void create_collider(axiom::collider3d& collider, axiom::transform3d& transform, std::vector<axiom::vertex_element3d>& elements, float mass = 1.0f, bool is_static = false) {
    axiom::collision_shape3d shape;
    shape.elements = elements;
    collider.shapes = {shape};

    collider.is_static = is_static;
    collider.allow_rotation = true;

    vec3 offset = axiom::initialize_collider(collider, {mass});
    transform.position += transform.orientation * offset;
    
    axiom::create_bounding_box(collider);


    // get faces
    std::vector<axiom::shape_face> faces;

    if(collider.shapes[0].elements.size() >= 3) collider.shapes[0].faces = get_faces(collider.shapes[0].elements);
};


void create_mesh(axiom::color_mesh3d& mesh, std::vector<axiom::vertex_element3d>& elements, vec3 color) {
    std::vector<axiom::output_vertex> surface_vertices;
    std::vector<uint> surface_indices;
    axiom::create_mesh(elements, &surface_vertices, &surface_indices);

    std::unordered_map<uint, vec3> normals;

    for(uint i = 0; i < surface_indices.size(); i += 3) {
        uint i0 = surface_indices[i];
        uint i1 = surface_indices[i + 1];
        uint i2 = surface_indices[i + 2];

        axiom::output_vertex v0 = surface_vertices[i0];
        axiom::output_vertex v1 = surface_vertices[i1];
        axiom::output_vertex v2 = surface_vertices[i2];

        vec3 scaled_normal = cross(v0.position - v2.position, v1.position - v2.position);

        if(v0.element != v1.element || v1.element != v2.element || v2.element != v0.element) {
            i0 |= 0x80000000;
            i1 |= 0x80000000;
            i2 |= 0x80000000;
        }
        
        if(!normals.contains(i0)) normals[i0] = vec3(0.0f);
        if(!normals.contains(i1)) normals[i1] = vec3(0.0f);
        if(!normals.contains(i2)) normals[i2] = vec3(0.0f);

        if(!(elements[v0.element].radii.x + elements[v0.element].radii.y == 0.0f || 
        elements[v0.element].radii.y + elements[v0.element].radii.z == 0.0f || 
        elements[v0.element].radii.z + elements[v0.element].radii.x == 0.0f)) {
            normals[i0] += scaled_normal;
        }

        if(!(elements[v1.element].radii.x + elements[v1.element].radii.y == 0.0f || 
        elements[v1.element].radii.y + elements[v1.element].radii.z == 0.0f || 
        elements[v1.element].radii.z + elements[v1.element].radii.x == 0.0f)) {
            normals[i1] += scaled_normal;
        }

        if(!(elements[v2.element].radii.x + elements[v2.element].radii.y == 0.0f || 
        elements[v2.element].radii.y + elements[v2.element].radii.z == 0.0f || 
        elements[v2.element].radii.z + elements[v2.element].radii.x == 0.0f)) {
            normals[i2] += scaled_normal;
        }

        //
    };

    for(auto& [key, normal] : normals) {
        float len = length(normal);
        if(len != 0.0f) normal /= len;
    }

    for(uint i = 0; i < surface_indices.size(); i += 3) {
        uint i0 = surface_indices[i];
        uint i1 = surface_indices[i + 1];
        uint i2 = surface_indices[i + 2];

        axiom::output_vertex v0 = surface_vertices[i0];
        axiom::output_vertex v1 = surface_vertices[i1];
        axiom::output_vertex v2 = surface_vertices[i2];
        
        if(v0.element != v1.element || v0.element != v2.element || v1.element != v2.element) {
            i0 |= 0x80000000;
            i1 |= 0x80000000;
            i2 |= 0x80000000;
        }
        
        axiom::color_vertex3d vertex;
        vertex.color = vec4(color, 1.0);

        vec3 n0 = normals[i0];
        vec3 n1 = normals[i1];
        vec3 n2 = normals[i2];

        bool o0 = length(n0) == 0.0f;
        bool o1 = length(n1) == 0.0f;
        bool o2 = length(n2) == 0.0f;

        if(o0 || o1 || o2) {
            vec3 normal = normalize(cross(v0.position - v2.position, v1.position - v2.position));

            if(o0) n0 = normal;
            if(o1) n1 = normal;
            if(o2) n2 = normal;
        }

        vertex.position = v0.position;
        vertex.normal = n0;
        mesh.vs.push_back(vertex);
        
        vertex.position = v1.position;
        vertex.normal = n1;
        mesh.vs.push_back(vertex);
        
        vertex.position = v2.position;
        vertex.normal = n2;
        mesh.vs.push_back(vertex);
    };
    
    mesh.load();
};

void create_mesh(axiom::color_mesh3d& mesh, std::vector<std::vector<axiom::vertex_element3d>>& elements, vec3 color) {
    for(int j = 0; j < elements.size(); ++j) {
        std::vector<axiom::output_vertex> surface_vertices;
        std::vector<uint> surface_indices;
        axiom::create_mesh(elements[j], &surface_vertices, &surface_indices);

        std::unordered_map<uint, vec3> normals;

        for(uint i = 0; i < surface_indices.size(); i += 3) {
            uint i0 = surface_indices[i];
            uint i1 = surface_indices[i + 1];
            uint i2 = surface_indices[i + 2];

            axiom::output_vertex v0 = surface_vertices[i0];
            axiom::output_vertex v1 = surface_vertices[i1];
            axiom::output_vertex v2 = surface_vertices[i2];

            vec3 scaled_normal = cross(v0.position - v2.position, v1.position - v2.position);

            if(v0.element != v1.element || v1.element != v2.element || v2.element != v0.element) {
                i0 |= 0x80000000;
                i1 |= 0x80000000;
                i2 |= 0x80000000;
            }
            
            if(!normals.contains(i0)) normals[i0] = vec3(0.0f);
            if(!normals.contains(i1)) normals[i1] = vec3(0.0f);
            if(!normals.contains(i2)) normals[i2] = vec3(0.0f);

            if(!(elements[j][v0.element].radii.x + elements[j][v0.element].radii.y == 0.0f || 
            elements[j][v0.element].radii.y + elements[j][v0.element].radii.z == 0.0f || 
            elements[j][v0.element].radii.z + elements[j][v0.element].radii.x == 0.0f)) {
                normals[i0] += scaled_normal;
            }

            if(!(elements[j][v1.element].radii.x + elements[j][v1.element].radii.y == 0.0f || 
            elements[j][v1.element].radii.y + elements[j][v1.element].radii.z == 0.0f || 
            elements[j][v1.element].radii.z + elements[j][v1.element].radii.x == 0.0f)) {
                normals[i1] += scaled_normal;
            }

            if(!(elements[j][v2.element].radii.x + elements[j][v2.element].radii.y == 0.0f || 
            elements[j][v2.element].radii.y + elements[j][v2.element].radii.z == 0.0f || 
            elements[j][v2.element].radii.z + elements[j][v2.element].radii.x == 0.0f)) {
                normals[i2] += scaled_normal;
            }

            //
        };

        for(auto& [key, normal] : normals) {
            float len = length(normal);
            if(len != 0.0f) normal /= len;
        }

        for(uint i = 0; i < surface_indices.size(); i += 3) {
            uint i0 = surface_indices[i];
            uint i1 = surface_indices[i + 1];
            uint i2 = surface_indices[i + 2];

            axiom::output_vertex v0 = surface_vertices[i0];
            axiom::output_vertex v1 = surface_vertices[i1];
            axiom::output_vertex v2 = surface_vertices[i2];
            
            if(v0.element != v1.element || v0.element != v2.element || v1.element != v2.element) {
                i0 |= 0x80000000;
                i1 |= 0x80000000;
                i2 |= 0x80000000;
            }
            
            axiom::color_vertex3d vertex;
            vertex.color = vec4(color, 1.0);

            vec3 n0 = normals[i0];
            vec3 n1 = normals[i1];
            vec3 n2 = normals[i2];

            bool o0 = length(n0) == 0.0f;
            bool o1 = length(n1) == 0.0f;
            bool o2 = length(n2) == 0.0f;

            if(o0 || o1 || o2) {
                vec3 normal = normalize(cross(v0.position - v2.position, v1.position - v2.position));

                if(o0) n0 = normal;
                if(o1) n1 = normal;
                if(o2) n2 = normal;
            }

            vertex.position = v0.position;
            vertex.normal = n0;
            mesh.vs.push_back(vertex);
            
            vertex.position = v1.position;
            vertex.normal = n1;
            mesh.vs.push_back(vertex);
            
            vertex.position = v2.position;
            vertex.normal = n2;
            mesh.vs.push_back(vertex);
        };
    }
    
    mesh.load();
};

void create_collider(axiom::collider3d& collider, axiom::transform3d& transform, std::vector<std::vector<axiom::vertex_element3d>>& elements, std::vector<float> masses, bool is_static = false) {
    for(int i = 0; i < elements.size(); ++i) {
        axiom::collision_shape3d shape;
        shape.elements = elements[i];
        collider.shapes.push_back(shape);
    }
    
    collider.is_static = is_static;
    collider.allow_rotation = true;

    vec3 offset = axiom::initialize_collider(collider, masses);
    transform.position += transform.orientation * offset;
    axiom::create_bounding_box(collider);

    for(auto& ee : elements) for(auto& element : ee) element.center -= offset;

    for(int i = 0; i < elements.size(); ++i) if(collider.shapes[i].elements.size() >= 3) collider.shapes[i].faces = get_faces(collider.shapes[i].elements);
};

void create_cube(vec3 position, mat3 orientation, float diameter, vec3 color, float mass = 1.0f) {
    axiom::transform3d transform;
    axiom::color_mesh3d mesh;
    axiom::collider3d collider;
    
    std::vector<axiom::vertex_element3d> elements = {
        axiom::vertex_element3d{vec3(-1.0f, -1.0f, -1.0f) * 0.5f / axiom::sqrt3 * diameter},
        axiom::vertex_element3d{vec3(1.0f, -1.0f, -1.0f) * 0.5f / axiom::sqrt3 * diameter},
        axiom::vertex_element3d{vec3(-1.0f, 1.0f, -1.0f) * 0.5f / axiom::sqrt3 * diameter},
        axiom::vertex_element3d{vec3(1.0f, 1.0f, -1.0f) * 0.5f / axiom::sqrt3 * diameter},
        axiom::vertex_element3d{vec3(-1.0f, -1.0f, 1.0f) * 0.5f / axiom::sqrt3 * diameter},
        axiom::vertex_element3d{vec3(1.0f, -1.0f, 1.0f) * 0.5f / axiom::sqrt3 * diameter},
        axiom::vertex_element3d{vec3(-1.0f, 1.0f, 1.0f) * 0.5f / axiom::sqrt3 * diameter},
        axiom::vertex_element3d{vec3(1.0f, 1.0f, 1.0f) * 0.5f / axiom::sqrt3 * diameter},
    };

    transform.position = position;
    transform.orientation = orientation;

    create_mesh(mesh, elements, color);
    create_collider(collider, transform, elements, mass);

    uint entity = axiom::insert_entity();
    axiom::insert_component(entity, transform);
    axiom::insert_component(entity, mesh);
    axiom::insert_component(entity, collider);
}

void create_tetrahedron(vec3 position, mat3 orientation, float diameter, vec3 color, float mass = 1.0f) {
    axiom::transform3d transform;
    axiom::color_mesh3d mesh;
    axiom::collider3d collider;
    uint entity = axiom::insert_entity();

    std::vector<axiom::vertex_element3d> elements = {
        axiom::vertex_element3d{vec3(1.0f, -1.0f, -1.0f) / axiom::sqrt3 * 0.5f * diameter},
        axiom::vertex_element3d{vec3(-1.0f, 1.0f, -1.0f) / axiom::sqrt3 * 0.5f * diameter},
        axiom::vertex_element3d{vec3(-1.0f, -1.0f, 1.0f) / axiom::sqrt3 * 0.5f * diameter},
        axiom::vertex_element3d{vec3(1.0f, 1.0f, 1.0f) / axiom::sqrt3 * 0.5f * diameter},
    };

    transform.position = position;
    transform.orientation = orientation;

    create_mesh(mesh, elements, color);
    create_collider(collider, transform, elements, mass);

    axiom::insert_component(entity, transform);
    axiom::insert_component(entity, mesh);
    axiom::insert_component(entity, collider);
}

void create_bipyramid(vec3 position, mat3 orientation, float diameter, int num_vertices, vec3 color, float mass = 1.0f) {
    axiom::transform3d transform;
    axiom::color_mesh3d mesh;
    axiom::collider3d collider;
    
    std::vector<axiom::vertex_element3d> elements = {
        axiom::vertex_element3d{vec3(0.0f, 0.0f, 1.0f) * 0.5f * diameter},
        axiom::vertex_element3d{vec3(0.0f, 0.0f, -1.0f) * 0.5f * diameter},
    };

    for(int i = 0; i < num_vertices; ++i) {
        float angle = axiom::pi * 2.0f * (float(i) / num_vertices);
        
        auto element = axiom::vertex_element3d{vec3(cos(angle), sin(angle), 0) * 0.5f * diameter};
        elements.push_back(element);
    }

    transform.position = position;
    transform.orientation = orientation;

    create_mesh(mesh, elements, color);
    create_collider(collider, transform, elements, mass);
    
    uint entity = axiom::insert_entity();
    axiom::insert_component(entity, transform);
    axiom::insert_component(entity, mesh);
    axiom::insert_component(entity, collider);
}

void create_dodecahedron(vec3 position, mat3 orientation, float diameter, vec3 color, float mass = 1.0f) {
    axiom::transform3d transform;
    axiom::color_mesh3d mesh;
    axiom::collider3d collider;

    float phi = (1.0f + sqrt(5.0f)) / 2.0f;

    std::vector<vec3> vs = {
        vec3(-1, -1, -1),
        vec3(1, -1, -1),
        vec3(-1, 1, -1),
        vec3(1, 1, -1),
        vec3(-1, -1, 1),
        vec3(1, -1, 1),
        vec3(-1, 1, 1),
        vec3(1, 1, 1),
        
        vec3(0, -phi, -1.0f / phi),
        vec3(0, phi, -1.0f / phi),
        vec3(0, -phi, 1.0f / phi),
        vec3(0, phi, 1.0f / phi),
        
        vec3(-1.0f / phi, 0, -phi),
        vec3(1.0f / phi, 0, -phi),
        vec3(-1.0f / phi, 0, phi),
        vec3(1.0f / phi, 0, phi),

        vec3(-phi, -1.0f / phi, 0),
        vec3(phi, -1.0f / phi, 0),
        vec3(-phi, 1.0f / phi, 0),
        vec3(phi, 1.0f / phi, 0),
    };

    std::vector<axiom::vertex_element3d> elements;
    for(vec3 v : vs) elements.push_back(axiom::vertex_element3d{v / axiom::sqrt3 * diameter * 0.5f});

    transform.position = position;
    transform.orientation = orientation;

    create_mesh(mesh, elements, color);
    create_collider(collider, transform, elements, mass);
    
    uint entity = axiom::insert_entity();
    axiom::insert_component(entity, transform);
    axiom::insert_component(entity, mesh);
    axiom::insert_component(entity, collider);
}

void create_icosahedron(vec3 position, mat3 orientation, float diameter, vec3 color, float mass = 1.0f) {
    axiom::transform3d transform;
    axiom::color_mesh3d mesh;
    axiom::collider3d collider;

    float phi = (1.0f + sqrt(5.0f)) / 2.0f;

    std::vector<vec3> vs = {
        vec3(0.0f, -1.0f, -phi),
        vec3(0.0f, 1.0f, -phi),
        vec3(0.0f, -1.0f, phi),
        vec3(0.0f, 1.0f, phi),

        vec3(-1.0f, -phi, 0.0f),
        vec3(1.0f, -phi, 0.0f),
        vec3(-1.0f, phi, 0.0f),
        vec3(1.0f, phi, 0.0f),

        vec3(-phi, 0.0f, -1.0f),
        vec3(phi, 0.0f, -1.0f),
        vec3(-phi, 0.0f, 1.0f),
        vec3(phi, 0.0f, 1.0f),
    };

    float rad = sqrt(1.0f + phi * phi);

    std::vector<axiom::vertex_element3d> elements;
    for(vec3 v : vs) elements.push_back(axiom::vertex_element3d{v / rad * 0.5f * diameter});

    transform.position = position;
    transform.orientation = orientation;

    create_mesh(mesh, elements, color);
    create_collider(collider, transform, elements, mass);
    
    uint entity = axiom::insert_entity();
    axiom::insert_component(entity, transform);
    axiom::insert_component(entity, mesh);
    axiom::insert_component(entity, collider);
}

void create_ui() {
    auto* ui_system = &axiom::global_core.ecs->get_system<axiom::ui_system>();
    auto* csystem = &axiom::global_core.ecs->get_system<chat_system>();
    auto* msystem = &axiom::global_core.ecs->get_system<main_system>();

    std::function<void()> switch_lipsum = [ui_system]() {
        ui_system->position(axiom::position_mode::TOP_LEFT);

        ui_system->input_reset();
        ui_system->input_z(0.1f);

        vec2 window_size = vec2(384, 384);

        axiom::window_widget::insert("Lipsum", window_size, (vec2(ui_system->window->screen_size) - window_size) * 0.5f, axiom::color_blue);
        
        ui_system->buffer(vec4(0.0f));
        axiom::panel_widget::insert();
        axiom::scroll_widget::insert(6.0f, true);
        ui_system->buffer(vec4(2.0f));

        axiom::column_widget::insert();

        std::string lipsum = R"(Lorem ipsum dolor sit amet, consectetur adipiscing elit. Vestibulum neque mi, tincidunt vitae efficitur in, porta eget erat. Nam vitae leo nec ligula imperdiet lacinia. Praesent sed elit vitae diam finibus convallis at a leo. Duis finibus dolor nisl, vitae tristique lectus egestas a. Nullam quam lectus, fringilla a iaculis vel, suscipit sit amet lectus. Nulla rutrum dapibus enim et tincidunt. Suspendisse et lacus ac dui tristique bibendum et a orci. Donec maximus nulla quis scelerisque placerat. Sed sagittis quam est, vestibulum condimentum sem feugiat ac. Cras non est at nisl fringilla interdum. Vestibulum ut neque sagittis, dictum ligula non, gravida mi. Quisque a nunc lorem. Nam libero libero, aliquet eu tincidunt sit amet, sollicitudin suscipit ex. Curabitur lacinia magna augue, vitae laoreet nisl placerat a. Donec convallis nulla sed nulla lacinia, sed tempus orci volutpat. Quisque vitae turpis eu nisl cursus dignissim.

    Aliquam interdum lectus risus, id efficitur ipsum bibendum vitae. Donec nulla ante, pretium in ullamcorper nec, bibendum ac nunc. Vivamus metus nisl, suscipit ac commodo at, viverra at enim. Pellentesque egestas facilisis sagittis. Duis vel sodales augue. Aliquam erat volutpat. Nam vel lectus at dui congue tincidunt. Phasellus placerat aliquet urna eu congue. Quisque turpis mauris, accumsan at tincidunt ut, dapibus sit amet erat. Nullam enim felis, facilisis nec vulputate eu, congue et nunc. Nunc eros turpis, placerat ac sem eget, pulvinar ultrices arcu. Etiam placerat dui eros, eget commodo metus tempus et. Maecenas volutpat lacinia nisi, eu laoreet sapien ultrices nec.)";

        axiom::text_widget::insert(lipsum, axiom::text_alignment::LEFT, true);

        ui_system->input_z(0.0f);
    };

    std::function<void()> switch_render = [msystem, ui_system]() {
        std::function<void(axiom::render_target&)> render_func = [msystem](axiom::render_target& f) {
            static axiom::vertices vertices;
            static double rotation = 0.0f;

            float r = 0.75f;

            vec2 scale = vec2(f.size) / float(glm::min(f.size.x, f.size.y));
            mat4 matrix = glm::scale(vec3(1.0f / scale, 1.0f));
            mat4 rot = glm::rotate((float)rotation, vec3(0.0f, 0.0f, 1.0f));
            matrix = matrix * rot;

            rotation += axiom::global_core.ecs->delta_time;

            struct color_vertex {
                vec3 position;
                vec3 color;
                vec2 tex_coord;
            };

            /*
            std::vector<color_vertex> vs = {
                color_vertex({-sqrt(3.0f) * 0.5f * r, -0.5f * r, 0.5f}, {1.0f, 0.0f, 0.0f}, vec2(-sqrt(3.0f) * 0.5f, -0.5f)),
                color_vertex({0.0f, r, 0.5f}, {0.0f, 1.0f, 0.0f}, vec2(0.0f, 1.0f)),
                color_vertex({sqrt(3.0f) * 0.5f * r, -0.5f * r, 0.5f}, {0.0f, 0.0f, 1.0f}, vec2(sqrt(3.0f) * 0.5f, -0.5f))
            };
            */

            std::vector<color_vertex> vs = {
                color_vertex({-1.0f * r, -1.0f * r, 0.5f}, {1.0f, 0.0f, 0.0f}, vec2(0.0f, 0.0f)),
                color_vertex({1.0f * r, -1.0f * r, 0.5f}, {1.0f, 1.0f, 0.0f}, vec2(1.0f, 0.0f)),
                color_vertex({-1.0f * r, 1.0f * r, 0.5f}, {0.0f, 0.0f, 1.0f}, vec2(0.0f, 1.0f)),
                color_vertex({1.0f * r, 1.0f * r, 0.5f}, {0.0f, 1.0f, 0.0f}, vec2(1.0f, 1.0f)),
            };

            vs = {vs[0], vs[1], vs[3], vs[0], vs[3], vs[2]};

            if(!vertices.initialized) vertices.init();

            vertices.vertex_buffer_data(vs.data(), vs.size(), sizeof(color_vertex), GL_STATIC_DRAW);

            vertices.add_vertex_attribute(0, 3, GL_FLOAT, false, sizeof(color_vertex), 0);
            vertices.add_vertex_attribute(1, 3, GL_FLOAT, false, sizeof(color_vertex), sizeof(float) * 3);
            vertices.add_vertex_attribute(2, 2, GL_FLOAT, false, sizeof(color_vertex), sizeof(float) * 6);

            msystem->shaders["test"].use();
            msystem->textures["test"].bind(0);
            vertices.bind();

            glUniformMatrix4fv(0, 1, false, &matrix[0][0]);

            vertices.draw_vertices(GL_TRIANGLES);
        };
        
        std::vector<axiom::texture_format> formats = {axiom::texture_format::RGBA8};
        std::vector<axiom::texture_attachment> attachments = {axiom::texture_attachment::COLOR0};

        msystem->targets.push_back(std::make_unique<axiom::render_target>(axiom::render_target::create(render_func, ivec2(400, 400), ivec2(0), formats, attachments)));

        //
        ui_system->input_reset();
        ui_system->input_z(0.1f);
        ui_system->buffer(vec4(0.0f));

        vec2 window_size = vec2(384, 384);
        axiom::window_widget::insert("Render", window_size, (vec2(ui_system->window->screen_size) - window_size) * 0.5f, axiom::color_blue);
        axiom::render_widget::insert(msystem->targets[1].get(), 0);
    };

    std::function<void()> switch_profiler = [msystem, ui_system]() {
        //
        ui_system->input_reset();
        ui_system->input_z(0.1f);
        ui_system->buffer(vec4(0.0f));

        vec2 window_size = vec2(384, 384);
        axiom::window_widget::insert("Profiler", window_size, (vec2(ui_system->window->screen_size) - window_size) * 0.5f, axiom::color_blue);
        axiom::panel_widget::insert();
        axiom::scroll_widget::insert(6.0f, true);

        ui_system->buffer(vec4(4.0f));
        ui_system->position(axiom::position_mode::TOP_LEFT);
        axiom::column_widget::insert();

        axiom::text_widget::insert("", axiom::text_alignment::LEFT, true, 
            [](std::string prev) {
                static double time = 1000.0f;

                time += axiom::global_core.ecs->delta_time;

                if(time > 1.0f) {
                    struct p_entry {
                        std::string name;
                        ulong occurances = 0;
                        ulong avg_time = 0;
                    };

                    ulong avg_time = 0;

                    std::unordered_map<std::string, p_entry> entries;

                    for(auto e : axiom::prof.frames) {
                        avg_time += e.end - e.start;

                        for(auto& entry : e.entries) {
                            if(!entries.contains(entry.name)) {
                                entries.emplace(entry.name, p_entry(entry.name));
                            }

                            p_entry& ee = entries[entry.name];

                            ee.avg_time += entry.end - entry.start;
                            ++ee.occurances;
                        }
                    }
                    

                    std::string str;
                    str += "FRAME\n";
                    str += "avg time: ";
                    str += axiom::to_base(float(avg_time) / axiom::prof.frames.size() / 1000.0f, 10, 3) + " ms" + "\n";

                    str += "\n";

                    for(auto& [k, entry] : entries) {
                        double time = double(entry.avg_time) / entry.occurances / 1000.0f;

                        str += entry.name + ": " + axiom::to_base(float(time), 10, 3) + " ms";

                        str + "\n";
                    }

                    //

                    time = 0.0f;

                    return str;
                } else {
                    return prev;
                }
            }
        );
    };

    std::function<void()> switch_camera = [msystem, ui_system]() {
        ui_system->input_reset();
        ui_system->input_z(0.1f);
        ui_system->buffer(vec4(0.0f));

        // insert camera
        
        uint camera_entity = axiom::insert_entity();
        axiom::camera3d cam;
        axiom::transform3d tf;

        cam.fov = 90.0f;

        tf.position = vec3(0.0f, 0.0f, 1.0f);
        tf.orientation = axiom::rotate_to(vec3(0.0f, 0.0f, -1.0f), vec3(0.0f, 1.0f, 0.0f));

        axiom::insert_component(camera_entity, cam);
        axiom::insert_component(camera_entity, tf);
        
        //
        
        std::function<void(axiom::render_widget*, axiom::render_target*)> callback_func = [msystem, ui_system, camera = camera_entity](axiom::render_widget* self, axiom::render_target* target) {
            static bool movement_capture = false;
            static float movement_speed = 1.0f;

            static uint constraint_index = 0xFFFFFFFF;
            static float constraint_dist = 0.0f;
            
            axiom::transform3d& camera_transform = axiom::get_component<axiom::transform3d>(camera);

            if(ui_system->click_capture == self->self) {
                if(movement_capture == false) {
                    ui_system->hide_cursor();
                    movement_capture = true;
                }
            } else {
                if(movement_capture) {
                    ui_system->show_cursor();
                    movement_capture = false;
                }
            }

            if(ui_system->hover_capture == self->self) {
                if(ui_system->window->scroll_delta != 0.0f) {
                    movement_speed *= pow(2, ui_system->window->scroll_delta * 0.5f);
                }
            }

            if(ui_system->click_capture == self->self) {
                if(movement_capture) {
                    glm::vec3 raw_movement = {0, 0, 0};
                    float rotate_value = 0.0f;

                    vec3 rotate = vec3(0.0f);
                    rotate.x = -ui_system->window->cursor_delta.x;
                    rotate.y = -ui_system->window->cursor_delta.y;

                    if(ui_system->window->input_map[axiom::input_code::KEY_Q]) {
                        rotate.z -= 1;
                    }
                    if(ui_system->window->input_map[axiom::input_code::KEY_E]) {
                        rotate.z += 1;
                    }

                    //

                    if(ui_system->window->input_map[axiom::input_code::KEY_A]) {
                        raw_movement.x -= 1;
                    }
                    if(ui_system->window->input_map[axiom::input_code::KEY_D]) {
                        raw_movement.x += 1;
                    } 
                    if(ui_system->window->input_map[axiom::input_code::KEY_S]) {
                        raw_movement.z += 1;
                    }
                    if(ui_system->window->input_map[axiom::input_code::KEY_W]) {
                        raw_movement.z -= 1;
                    }
                    if(ui_system->window->input_map[axiom::input_code::KEY_SPACE]) {
                        raw_movement.y += 1;
                    }
                    if(ui_system->window->input_map[axiom::input_code::KEY_LEFT_SHIFT]) {
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
                    glm::mat3 rotate_z_mat = (mat3)glm::rotate(float(2 * axiom::pi * (1.0 / 128) * rotate.z * (axiom::global_core.ecs->delta_time * 60)), dir);

                    camera_transform.orientation = rotate_z_mat * rotate_x_mat * rotate_y_mat * camera_transform.orientation;
                    camera_transform.position += translation_vec * (float)axiom::global_core.ecs->delta_time * movement_speed;
                }
            }
        };

        std::function<void(axiom::render_target&)> render_func = [camera = camera_entity](axiom::render_target& f) {
            base_render(camera, f);
        };
        
        std::function<void()> on_close = [camera_entity]() {
            axiom::global_core.ecs->erase_entity(camera_entity);
        };

        std::vector<axiom::texture_format> formats = {axiom::texture_format::RGBA8, axiom::texture_format::RGBA8, axiom::texture_format::RGBA8, axiom::texture_format::DEPTHF};
        std::vector<axiom::texture_attachment> attachments = {axiom::texture_attachment::COLOR0, axiom::texture_attachment::COLOR1, axiom::texture_attachment::COLOR2, axiom::texture_attachment::DEPTH};

        msystem->targets.push_back(std::make_unique<axiom::render_target>(axiom::render_target::create(render_func, ivec2(400, 400), ivec2(0, 0), formats, attachments)));
        
        vec2 window_size = vec2(384, 384);

        axiom::window_widget::insert("Camera", window_size, (vec2(ui_system->window->screen_size) - window_size) * 0.5f, axiom::color_blue, on_close);
        axiom::panel_widget::insert();
        axiom::render_widget::insert(msystem->targets.back().get(), 0, callback_func);
    };

    std::function<void()> switch_debugger = [msystem, ui_system]() {
        uint camera_entity = axiom::insert_entity();

        axiom::camera3d cam;
        axiom::transform3d tf;

        cam.fov = 90.0f;

        tf.position = vec3(0.0f, 0.0f, 1.0f);
        tf.orientation = glm::identity<mat3>();

        axiom::insert_component(camera_entity, cam);
        axiom::insert_component(camera_entity, tf);

        //

        std::function<void(axiom::render_widget*, axiom::render_target*)> callback_func = [msystem, ui_system, camera = camera_entity](axiom::render_widget* self, axiom::render_target* target) {
            static bool movement_capture = false;
            static vec3 center = vec3(0.0f);
            static float dist = 4.0f;
            
            axiom::transform3d& camera_transform = axiom::get_component<axiom::transform3d>(camera);

            if(ui_system->click_capture == self->self) {
                if(movement_capture == false) {
                    ui_system->hide_cursor();
                    movement_capture = true;
                }
            } else {
                if(movement_capture) {
                    ui_system->show_cursor();
                    movement_capture = false;
                }
            }

            if(ui_system->hover_capture == self->self) {
                if(ui_system->window->scroll_delta != 0.0f) {
                    dist *= pow(2, -ui_system->window->scroll_delta * 0.5f);
                }
            }

            if(ui_system->click_capture == self->self) {
                if(movement_capture) {
                    glm::vec3 raw_movement = {0, 0, 0};
                    float rotate_value = 0.0f;

                    vec3 rotate = vec3(0.0f);
                    rotate.x = ui_system->window->cursor_delta.x;
                    rotate.y = ui_system->window->cursor_delta.y;

                    if(ui_system->window->input_map[axiom::input_code::KEY_Q]) {
                        rotate.z -= 1;
                    }
                    if(ui_system->window->input_map[axiom::input_code::KEY_E]) {
                        rotate.z += 1;
                    }

                    //

                    float len = length(raw_movement);
                    if(len != 0.0f) raw_movement = glm::normalize(raw_movement);
                    
                    glm::vec3 translation_vec = camera_transform.orientation * raw_movement;
                    
                    vec3 dir = -camera_transform.orientation[2];
                    vec3 u = camera_transform.orientation[1];
                    glm::mat3 rotate_y_mat = (mat3)glm::rotate(float(2 * axiom::pi * (1.0 / 1024) * -rotate.y), glm::normalize(glm::cross(u, dir)));
                    glm::mat3 rotate_x_mat = (mat3)glm::rotate(float(2 * axiom::pi * (1.0 / 1024) * -rotate.x), u);
                    glm::mat3 rotate_z_mat = (mat3)glm::rotate(float(2 * axiom::pi * (1.0 / 128) * rotate.z * (axiom::global_core.ecs->delta_time * 60)), dir);

                    camera_transform.orientation = rotate_z_mat * rotate_x_mat * rotate_y_mat * camera_transform.orientation;
                    camera_transform.position = center + normalize(rotate_x_mat * rotate_y_mat * (camera_transform.position - center)) * dist;
                }
            }
            
            camera_transform.position = center + normalize(camera_transform.position - center) * dist;
        };

        static uint debugger_mode = 0;
        static uint selected_frame = 0;
        static uint selected_event = 0;
        static uint selected_step = 0;

        std::function<void(axiom::render_target&)> render_func = [msystem, camera = camera_entity](axiom::render_target& f) {
            main_system& msystem = axiom::get_system<main_system>();
            axiom::ui_system& ui_system = axiom::get_system<axiom::ui_system>();

            f.framebuffer.bind();
            vec3 background = axiom::hex_color(0x000000);
            glClearColor(background.x, background.y, background.z, 1.0f);
            glClearDepth(0.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            //

            glEnable(GL_CULL_FACE);
            
            static axiom::vertices vertices;
            if(!vertices.initialized) {
                vertices.init();
            }

            //

            axiom::transform3d& camera_transform = axiom::get_component<axiom::transform3d>(camera);
            axiom::camera3d& camera_cam = axiom::get_component<axiom::camera3d>(camera);

            camera_cam.aspect = vec2(f.size) / (float)glm::min(f.size.x, f.size.y);
            
            mat4 view = axiom::get_view(camera_cam, camera_transform);
            mat4 proj = axiom::get_proj(camera_cam);

            // render lines

            auto& physics_system = axiom::get_system<axiom::physics_system3d>();
            axiom::collision_event* event = nullptr;
            
            std::vector<axiom::collision_event> found_events;
            if(selected_frame < physics_system.debugger.frame_list.size()) {
                for(auto& ev : physics_system.debugger.frames[physics_system.debugger.frame_list[selected_frame]].collision_events) {
                    if(axiom::filter(ev)) found_events.push_back(ev);
                }
                if(found_events.size() == 0) found_events = physics_system.debugger.frames[physics_system.debugger.frame_list[selected_frame]].collision_events;
                
                if(physics_system.debugger.frames.size() && selected_event < found_events.size()) {
                    event = &found_events[selected_event];
                
                    axiom::collider3d& ca = axiom::get_component<axiom::collider3d>(event->collider_a);
                    axiom::collider3d& cb = axiom::get_component<axiom::collider3d>(event->collider_b);
                    axiom::transform3d ta = event->transform_a;
                    axiom::transform3d tb = event->transform_b;

                    if(debugger_mode == 0) {
                        // render grid

                        std::vector<vec2> vs = {
                            vec2(-1.0f, -1.0f),
                            vec2(1.0f, -1.0f),
                            vec2(-1.0f, 1.0f),
                            vec2(1.0f, 1.0f)
                        };

                        vs = {vs[0], vs[1], vs[3], vs[0], vs[3], vs[2]};

                        vertices.vertex_buffer_data(vs.data(), vs.size(), sizeof(vec2), GL_STATIC_DRAW);
                        vertices.add_vertex_attribute(0, 2, GL_FLOAT, false, sizeof(vec2), 0);

                        //

                        axiom::transform3d grid_transform;
                        grid_transform.position = vec3(0.0f);
                        grid_transform.orientation = glm::identity<mat3>();

                        mat4 model = axiom::get_model(grid_transform, camera_transform);

                        axiom::shader& grid_shader = msystem.shaders["grid3d"];
                        glDisable(GL_DEPTH_TEST);

                        grid_shader.use();
                        vertices.bind();
                        
                        f.framebuffer.textures[1].bind(0);

                        glUniformMatrix4fv(0, 1, false, &model[0][0]);
                        glUniformMatrix4fv(1, 1, false, &view[0][0]);
                        glUniformMatrix4fv(2, 1, false, &proj[0][0]);
                        glUniform3f(3, 8, 8, 8);
                        
                        vertices.draw_vertices(GL_TRIANGLES);
                        glEnable(GL_DEPTH_TEST);

                        // render debug info

                        std::vector<axiom::vertex_element3d> a_vertices = ca.shapes[event->shape_a].elements;
                        std::vector<axiom::vertex_element3d> b_vertices = cb.shapes[event->shape_b].elements;

                        vec3 ta_pos = ta.position;

                        ta.position += ta.orientation * ca.shapes[event->shape_a].position;
                        ta.orientation = ta.orientation * ca.shapes[event->shape_a].orientation;
                        tb.position += tb.orientation * cb.shapes[event->shape_b].position;
                        tb.orientation = tb.orientation * cb.shapes[event->shape_b].orientation;

                        vec3 a_rel_pos = transform_vertices(a_vertices, ta, ta_pos);
                        vec3 b_rel_pos = transform_vertices(b_vertices, tb, ta_pos);

                        std::vector<vec3> position;
                        std::vector<vec4> tex_range;
                        std::vector<vec4> color;
                        std::vector<vec2> size;
                        for(auto& a : a_vertices) {
                            for(auto& b : b_vertices) {
                                position.push_back(a.center - b.center);
                                tex_range.push_back(vec4(48, 96, 6, 6));
                                color.push_back(vec4(1.0f));
                                size.push_back(vec2(6));
                            }
                        }

                        render_billboards(camera, position, tex_range, color, size, f.size, msystem.textures["ui"]);

                        //


                        struct debug_triangle {
                            uint a;
                            uint b;
                            uint c;

                            vec3 color;
                        };

                        std::vector<vec3> points;
                        std::vector<debug_triangle> triangles;

                        std::vector<uint> indices;
                        std::vector<vec3> colors;

                        std::vector<vec3> line;

                        uint step = 0;
                        uint index = 0;

                        while(true) {
                            uint s = step;

                            if(index == event->gjk.size()) break;
                            auto gjk_step = event->gjk[index];
                            ++index;

                            if(gjk_step.erase_index != 0xFFFFFFFF) {
                                uint eindex = gjk_step.erase_index;
                                for(int i = 0; i < triangles.size(); ++i) {
                                    auto& triangle = triangles[i];

                                    if(triangle.a == eindex || triangle.b == eindex || triangle.c == eindex) triangle.color = axiom::hsv_color(0.0f, 0.75f, 1.0f);
                                }
                                
                                ++step;
                                if(step > selected_step) break;

                                points.erase(points.begin() + eindex);
                                for(int i = 0; i < triangles.size(); ++i) {
                                    auto& triangle = triangles[i];

                                    if(triangle.a == eindex || triangle.b == eindex || triangle.c == eindex) {
                                        triangles.erase(triangles.begin() + i);
                                        --i;
                                    } else {
                                        if(triangle.a > eindex) --triangle.a;
                                        if(triangle.b > eindex) --triangle.b;
                                        if(triangle.c > eindex) --triangle.c;
                                    }

                                }
                            }

                            if(length(gjk_step.search_origin) != 0.0f) {
                                line.push_back(gjk_step.search_origin);
                                line.push_back(gjk_step.search_origin + gjk_step.search_direction * 0.25f);
                                ++step;
                                if(step > selected_step) break;
                            }
                            
                            points.push_back(gjk_step.input_point);
                            if(points.size() == 3) {
                                triangles.clear();
                                triangles.push_back(debug_triangle(0, 1, 2, axiom::hsv_color(2.0f, 0.75f, 1.0f)));
                            } else if(points.size() == 4) {
                                triangles.push_back(debug_triangle(0, 1, 3, axiom::hsv_color(2.0f, 0.75f, 1.0f)));
                                triangles.push_back(debug_triangle(0, 2, 3, axiom::hsv_color(2.0f, 0.75f, 1.0f)));
                                triangles.push_back(debug_triangle(1, 2, 3, axiom::hsv_color(2.0f, 0.75f, 1.0f)));
                            }

                            ++step;
                            if(step > selected_step) break;

                            //

                            line.clear();
                        }

                        if(step <= selected_step && event->epa.size()) {
                            for(auto& triangle : triangles) triangle.color = axiom::hsv_color(4.0f, 0.75f, 1.0f);
                        }
                        
                        index = 0;
                        while(true) {
                            if(index == event->epa.size()) break;
                            auto epa_step = event->epa[index];
                            ++index;

                            //
                            
                            if(length(epa_step.search_origin) != 0.0f) {
                                line.push_back(epa_step.search_origin);
                                line.push_back(epa_step.search_origin + epa_step.search_direction * 0.25f);
                                ++step;
                                if(step > selected_step) break;
                            }

                            std::vector<ulong> edges;
                            for(uint i : epa_step.erase_triangles) {
                                debug_triangle& triangle = triangles[i];

                                ulong ab = (ulong(glm::max(triangle.a, triangle.b)) << 32) | ulong(glm::min(triangle.a, triangle.b));
                                ulong bc = (ulong(glm::max(triangle.b, triangle.c)) << 32) | ulong(glm::min(triangle.b, triangle.c));
                                ulong ca = (ulong(glm::max(triangle.c, triangle.a)) << 32) | ulong(glm::min(triangle.c, triangle.a));

                                edges.push_back(ab);
                                edges.push_back(bc);
                                edges.push_back(ca);

                            }
                            
                            std::sort(epa_step.erase_triangles.begin(), epa_step.erase_triangles.end(), std::greater<>());
                            for(uint i : epa_step.erase_triangles) {
                                triangles.erase(triangles.begin() + i);
                            }

                            for(uint64_t edge : edges) {
                                if(std::count(edges.begin(), edges.end(), edge) == 1) {
                                    debug_triangle triangle;
                                    triangle.a = edge & 0xFFFFFFFFull;
                                    triangle.b = edge >> 32;
                                    triangle.c = points.size();

                                    triangle.color = axiom::hsv_color(4.0f, 0.75f, 1.0f);

                                    triangles.push_back(triangle);
                                }
                            }

                            points.push_back(epa_step.input_point);

                            ++step;
                            if(step > selected_step) break;
                            
                            line.clear();
                        }

                        if(triangles.size() == 0) {
                            if(points.size() == 2) {
                                indices.push_back(0);
                                indices.push_back(1);
                                colors.push_back(axiom::hsv_color(2.0f, 0.75f, 1.0f));
                            }
                        } else {
                            for(auto& triangle : triangles) {
                                indices.push_back(triangle.a);
                                indices.push_back(triangle.b);
                                indices.push_back(triangle.c);
                                
                                colors.push_back(triangle.color);
                            }
                        }

                        //

                        // render lines

                        std::vector<axiom::color_vertex3d> cvs;

                        uint i = 0;
                        for(int i = 0; i < indices.size(); i += 3) {
                            if(!colors.size()) break;

                            uint index_a = indices[i];
                            uint index_b = indices[i + 1];
                            uint index_c;
                            
                            if(indices.size() < i + 3) {
                                vec3 color = colors[i / 3];

                                axiom::color_vertex3d cva;
                                cva.color = vec4(color, 1.0f);
                                cva.position = points[index_a];
                                
                                axiom::color_vertex3d cvb;
                                cvb.color = vec4(color, 1.0f);
                                cvb.position = points[index_b];

                                cvs.push_back(cva);
                                cvs.push_back(cvb);
                            } else {
                                index_c = indices[i + 2];
                                
                                //
                                
                                vec3 color = colors[i / 3];

                                axiom::color_vertex3d cva;
                                cva.color = vec4(color, 1.0f);
                                cva.position = points[index_a];
                                
                                axiom::color_vertex3d cvb;
                                cvb.color = vec4(color, 1.0f);
                                cvb.position = points[index_b];

                                axiom::color_vertex3d cvc;
                                cvc.color = vec4(color, 1.0f);
                                cvc.position = points[index_c];

                                cvs.push_back(cva);
                                cvs.push_back(cvb);
                                cvs.push_back(cvb);
                                cvs.push_back(cvc);
                                cvs.push_back(cvc);
                                cvs.push_back(cva);
                            }
                        }

                        if(line.size()) {
                            axiom::color_vertex3d cv;
                            cv.position = line[0];
                            cv.color = vec4(axiom::hsv_color(1.0f, 0.75f, 1.0f), 1.0);
                            cvs.push_back(cv);
                            
                            cv.position = line[1];
                            cv.color = vec4(axiom::hsv_color(1.0f, 0.75f, 1.0f), 0.0);
                            cvs.push_back(cv);
                        }

                        vertices.vertex_buffer_data(cvs.data(), cvs.size(), sizeof(axiom::color_vertex3d), GL_STREAM_DRAW);
                        vertices.add_vertex_attribute(0, 3, GL_FLOAT, false, sizeof(axiom::color_vertex3d), 0);
                        vertices.add_vertex_attribute(1, 4, GL_FLOAT, false, sizeof(axiom::color_vertex3d), sizeof(float) * 3);
                        vertices.add_vertex_attribute(2, 3, GL_FLOAT, false, sizeof(axiom::color_vertex3d), sizeof(float) * 7);
                        
                        msystem.shaders["color3d"].use();
                        vertices.bind();

                        glUniformMatrix4fv(0, 1, false, &model[0][0]);
                        glUniformMatrix4fv(1, 1, false, &view[0][0]);
                        glUniformMatrix4fv(2, 1, false, &proj[0][0]);

                        vertices.draw_vertices(GL_LINES);

                        position = {event->point_m};
                        tex_range = {vec4(56, 104, 8, 8)};
                        color = {vec4(axiom::hsv_color(1.0f, 0.75f, 1.0f), 1.0f)};
                        size = {vec2(8.0f)};

                        for(auto v : event->data) {
                            position.push_back(v);
                            tex_range.push_back(vec4(48, 104, 8, 8));
                            color.push_back(vec4(axiom::hsv_color(1.0f, 0.75f, 1.0f), 1.0f));
                            size.push_back(vec2(8.0f));
                        }

                        render_billboards(camera, position, tex_range, color, size, f.size, msystem.textures["ui"]);
                    } else {
                        glEnable(GL_DEPTH_TEST);
                        
                        auto& collector = axiom::global_core.ecs->collectors["color_mesh3d"];

                        vec3 point = (event->point_a + event->point_b) * 0.5f;
                        point += event->transform_a.position;

                        { // a
                            axiom::transform3d transform = event->transform_a;
                            transform.position -= point;

                            if(axiom::has_component<axiom::color_mesh3d>(event->collider_a)) {
                                axiom::color_mesh3d& mesh = axiom::get_component<axiom::color_mesh3d>(event->collider_a);

                                mat4 model = axiom::get_model(transform, camera_transform);
                                axiom::shader& color_shader = msystem.shaders["color3d"];

                                //
                                
                                color_shader.use();

                                glUniformMatrix4fv(0, 1, false, &model[0][0]);
                                glUniformMatrix4fv(1, 1, false, &view[0][0]);
                                glUniformMatrix4fv(2, 1, false, &proj[0][0]);

                                mesh.vertices->draw_vertices(GL_TRIANGLES);
                            } else if(axiom::has_component<axiom::texture_mesh3d>(event->collider_a)) {
                                axiom::texture_mesh3d& mesh = axiom::get_component<axiom::texture_mesh3d>(event->collider_a);

                                mat4 model = axiom::get_model(transform, camera_transform);
                                axiom::shader& color_shader = msystem.shaders["texture3d"];

                                //

                                color_shader.use();

                                mesh.texture->bind(0);

                                glUniformMatrix4fv(0, 1, false, &model[0][0]);
                                glUniformMatrix4fv(1, 1, false, &view[0][0]);
                                glUniformMatrix4fv(2, 1, false, &proj[0][0]);

                                mesh.vertices->draw_vertices(GL_TRIANGLES);
                            } else if(axiom::has_component<axiom::texture_range_mesh3d>(event->collider_a)) {
                                axiom::texture_range_mesh3d& mesh = axiom::get_component<axiom::texture_range_mesh3d>(event->collider_a);

                                mat4 model = axiom::get_model(transform, camera_transform);
                                axiom::shader& color_shader = msystem.shaders["texture_range3d"];

                                //

                                color_shader.use();

                                mesh.texture->bind(0);

                                glUniformMatrix4fv(0, 1, false, &model[0][0]);
                                glUniformMatrix4fv(1, 1, false, &view[0][0]);
                                glUniformMatrix4fv(2, 1, false, &proj[0][0]);

                                mesh.vertices->draw_vertices(GL_TRIANGLES);
                            }
                        }

                        { // b
                            axiom::transform3d transform = event->transform_b;
                            transform.position -= point;

                            
                            if(axiom::has_component<axiom::color_mesh3d>(event->collider_b)) {
                                axiom::color_mesh3d& mesh = axiom::get_component<axiom::color_mesh3d>(event->collider_b);

                                mat4 model = axiom::get_model(transform, camera_transform);
                                axiom::shader& color_shader = msystem.shaders["color3d"];

                                //
                                
                                color_shader.use();

                                glUniformMatrix4fv(0, 1, false, &model[0][0]);
                                glUniformMatrix4fv(1, 1, false, &view[0][0]);
                                glUniformMatrix4fv(2, 1, false, &proj[0][0]);

                                mesh.vertices->draw_vertices(GL_TRIANGLES);
                            } else if(axiom::has_component<axiom::texture_mesh3d>(event->collider_b)) {
                                axiom::texture_mesh3d& mesh = axiom::get_component<axiom::texture_mesh3d>(event->collider_b);

                                mat4 model = axiom::get_model(transform, camera_transform);
                                axiom::shader& color_shader = msystem.shaders["texture3d"];

                                //

                                color_shader.use();

                                mesh.texture->bind(0);

                                glUniformMatrix4fv(0, 1, false, &model[0][0]);
                                glUniformMatrix4fv(1, 1, false, &view[0][0]);
                                glUniformMatrix4fv(2, 1, false, &proj[0][0]);

                                mesh.vertices->draw_vertices(GL_TRIANGLES);
                            } else if(axiom::has_component<axiom::texture_range_mesh3d>(event->collider_b)) {
                                axiom::texture_range_mesh3d& mesh = axiom::get_component<axiom::texture_range_mesh3d>(event->collider_b);

                                mat4 model = axiom::get_model(transform, camera_transform);
                                axiom::shader& color_shader = msystem.shaders["texture_range3d"];

                                //

                                color_shader.use();

                                mesh.texture->bind(0);

                                glUniformMatrix4fv(0, 1, false, &model[0][0]);
                                glUniformMatrix4fv(1, 1, false, &view[0][0]);
                                glUniformMatrix4fv(2, 1, false, &proj[0][0]);

                                mesh.vertices->draw_vertices(GL_TRIANGLES);
                            }
                        }

                        point = (event->point_a + event->point_b) * 0.5f;

                        // render grid

                        std::vector<vec2> vs = {
                            vec2(-1.0f, -1.0f),
                            vec2(1.0f, -1.0f),
                            vec2(-1.0f, 1.0f),
                            vec2(1.0f, 1.0f)
                        };

                        vs = {vs[0], vs[1], vs[3], vs[0], vs[3], vs[2]};

                        vertices.vertex_buffer_data(vs.data(), vs.size(), sizeof(vec2), GL_STATIC_DRAW);
                        vertices.add_vertex_attribute(0, 2, GL_FLOAT, false, sizeof(vec2), 0);
                        vertices.vertex_buffer_data(vs.data(), vs.size(), sizeof(vec2), GL_STATIC_DRAW);
                        vertices.add_vertex_attribute(0, 2, GL_FLOAT, false, sizeof(vec2), 0);

                        //

                        axiom::transform3d grid_transform;
                        grid_transform.position = vec3(0.0f);
                        grid_transform.orientation = glm::identity<mat3>();

                        mat4 model = axiom::get_model(grid_transform, camera_transform);

                        axiom::shader& grid_shader = msystem.shaders["grid3d"];
                        glEnable(GL_DEPTH_TEST);

                        grid_shader.use();
                        vertices.bind();

                        f.framebuffer.textures[1].bind(0);

                        glUniformMatrix4fv(0, 1, false, &model[0][0]);
                        glUniformMatrix4fv(1, 1, false, &view[0][0]);
                        glUniformMatrix4fv(2, 1, false, &proj[0][0]);

                        vertices.draw_vertices(GL_TRIANGLES);

                        // render debug info
                        
                        glDisable(GL_DEPTH_TEST);

                        std::vector<vec3> vv = {event->point_a - point, event->point_b - point};
                        std::vector<vec4> tex_range;
                        std::vector<vec4> color = {vec4(axiom::hsv_color(0.0f, 0.75f, 1.0f), 1.0f), vec4(axiom::hsv_color(3.0f, 0.75f, 1.0f), 1.0f)};
                        std::vector<vec2> size = {vec2(8.0f), vec2(8.0f)};

                        if(event->finished) tex_range = {vec4(48, 104, 8, 8), vec4(48, 104, 8, 8)};
                        else tex_range = {vec4(56, 104, 8, 8), vec4(56, 104, 8, 8)};

                        render_billboards(camera, vv, tex_range, color, size, f.size, msystem.textures["ui"]);

                        //

                        if(event->finished) {

                            if(event->manifold_a.size() == 1) {
                                std::vector<vec3> normals;

                                std::vector<vec3> position;
                                std::vector<vec4> tex_range;
                                std::vector<vec4> color;
                                std::vector<vec2> size;

                                position.push_back(event->manifold_a[0]);
                                tex_range.push_back(vec4(3, 0, 5, 5));
                                color.push_back(vec4(1.0f, 0.5f, 0.5f, 1.0f));
                                size.push_back(vec2(5, 5));
                                
                                position.push_back(event->manifold_b[0]);
                                tex_range.push_back(vec4(3, 0, 5, 5));
                                color.push_back(vec4(0.5f, 1.0f, 1.0f, 1.0f));
                                size.push_back(vec2(5, 5));

                                glDisable(GL_DEPTH_TEST);
                                render_billboards(camera, position, tex_range, color, size, f.size, msystem.textures["ui"]);
                            } else {
                                std::vector<axiom::color_vertex3d> cvs;
                            
                                for(int i = 0; i < event->manifold_a.size(); ++i) {
                                    vec3 p0 = event->manifold_a[i];
                                    vec3 p1 = event->manifold_a[(i + 1) % event->manifold_a.size()];

                                    axiom::color_vertex3d v0 = {p0, vec4(axiom::hsv_color(0.0f, 0.35f, 1.0f), 1.0)};
                                    axiom::color_vertex3d v1 = {p1, vec4(axiom::hsv_color(0.0f, 0.35f, 1.0f), 1.0)};
                                    
                                    cvs.push_back(v0);
                                    cvs.push_back(v1);
                                }
                                
                                for(int i = 0; i < event->manifold_b.size(); ++i) {
                                    vec3 p0 = event->manifold_b[i];
                                    vec3 p1 = event->manifold_b[(i + 1) % event->manifold_b.size()];

                                    axiom::color_vertex3d v0 = {p0, vec4(axiom::hsv_color(3.0f, 0.35f, 1.0f), 1.0)};
                                    axiom::color_vertex3d v1 = {p1, vec4(axiom::hsv_color(3.0f, 0.35f, 1.0f), 1.0)};

                                    cvs.push_back(v0);
                                    cvs.push_back(v1);
                                }

                                axiom::transform3d transform = {-point, glm::identity<mat3>()};
                                mat4 model = axiom::get_model(transform, camera_transform);

                                vertices.vertex_buffer_data(cvs.data(), cvs.size(), sizeof(axiom::color_vertex3d), GL_STREAM_DRAW);
                                vertices.add_vertex_attribute(0, 3, GL_FLOAT, false, sizeof(axiom::color_vertex3d), 0);
                                vertices.add_vertex_attribute(1, 4, GL_FLOAT, false, sizeof(axiom::color_vertex3d), sizeof(float) * 3);
                                vertices.add_vertex_attribute(2, 3, GL_FLOAT, false, sizeof(axiom::color_vertex3d), sizeof(float) * 7);
                                
                                msystem.shaders["color3d"].use();
                                vertices.bind();

                                glUniformMatrix4fv(0, 1, false, &model[0][0]);
                                glUniformMatrix4fv(1, 1, false, &view[0][0]);
                                glUniformMatrix4fv(2, 1, false, &proj[0][0]);

                                vertices.draw_vertices(GL_LINES);
                            }
                        }

                        point += event->transform_a.position;

                        { // bounding boxes
                            std::vector<vec3> vv = {
                                vec3(0.0f, 0.0f, 0.0f),
                                vec3(1.0f, 0.0f, 0.0f),
                                vec3(0.0f, 1.0f, 0.0f),
                                vec3(1.0f, 1.0f, 0.0f),
                                vec3(0.0f, 0.0f, 1.0f),
                                vec3(1.0f, 0.0f, 1.0f),
                                vec3(0.0f, 1.0f, 1.0f),
                                vec3(1.0f, 1.0f, 1.0f),
                            };

                            vv = {
                                vv[0], vv[1],
                                vv[2], vv[3],
                                vv[4], vv[5],
                                vv[6], vv[7],
                                
                                vv[0], vv[2],
                                vv[1], vv[3],
                                vv[4], vv[6],
                                vv[5], vv[7],
                                
                                vv[0], vv[4],
                                vv[1], vv[5],
                                vv[2], vv[6],
                                vv[3], vv[7],
                            };

                            axiom::bounding_box3d bba;
                            if(ca.shapes.size() > 1) bba = axiom::transform_bounding_box(ca.shapes[event->shape_a].bounding_box, ta.position, ta.orientation);
                            else bba = axiom::transform_bounding_box(ca.bounding_box, ta.position, ta.orientation);

                            axiom::bounding_box3d bbb;
                            if(cb.shapes.size() > 1) bbb = axiom::transform_bounding_box(cb.shapes[event->shape_b].bounding_box, tb.position, tb.orientation);
                            else bbb = axiom::transform_bounding_box(cb.bounding_box, tb.position, tb.orientation);

                            std::vector<axiom::color_vertex3d> cvs;

                            for(vec3 v : vv) {
                                cvs.push_back(axiom::color_vertex3d(v * (bba.maximum - bba.minimum) + bba.minimum - point, vec4(1.0f)));
                            }
                            for(vec3 v : vv) {
                                cvs.push_back(axiom::color_vertex3d(v * (bbb.maximum - bbb.minimum) + bbb.minimum - point, vec4(1.0f)));
                            }
                        
                            axiom::transform3d transform = {vec3(0.0f), glm::identity<mat3>()};
                            mat4 model = axiom::get_model(transform, camera_transform);

                            vertices.vertex_buffer_data(cvs.data(), cvs.size(), sizeof(axiom::color_vertex3d), GL_STREAM_DRAW);
                            vertices.add_vertex_attribute(0, 3, GL_FLOAT, false, sizeof(axiom::color_vertex3d), 0);
                            vertices.add_vertex_attribute(1, 4, GL_FLOAT, false, sizeof(axiom::color_vertex3d), sizeof(float) * 3);
                            vertices.add_vertex_attribute(2, 3, GL_FLOAT, false, sizeof(axiom::color_vertex3d), sizeof(float) * 7);
                            
                            msystem.shaders["color3d"].use();
                            vertices.bind();

                            glUniformMatrix4fv(0, 1, false, &model[0][0]);
                            glUniformMatrix4fv(1, 1, false, &view[0][0]);
                            glUniformMatrix4fv(2, 1, false, &proj[0][0]);

                            vertices.draw_vertices(GL_LINES);
                        }

                        { // points
                            std::vector<vec3> position;
                            std::vector<vec4> tex_range;
                            std::vector<vec4> color;
                            std::vector<vec2> size;

                            {
                                auto& cca = ca.shapes[event->shape_a];

                                std::vector<axiom::vertex_element3d> vertices = cca.elements;

                                axiom::transform3d tta = ta;
                                tta.position += tta.orientation * cca.position;
                                tta.orientation = tta.orientation * cca.orientation;
                                axiom::transform_vertices(vertices, tta, vec3(0.0f));

                                for(axiom::vertex_element3d v : vertices) {
                                    position.push_back(v.center - point);
                                    tex_range.push_back(vec4(54, 96, 6, 6));
                                    color.push_back(vec4(1.0f, 0.35f, 0.35f, 1.0f));
                                    size.push_back(vec2(6, 6));
                                }
                            }
                            {
                                auto& ccb = cb.shapes[event->shape_b];
                                std::vector<axiom::vertex_element3d> vertices = ccb.elements;

                                axiom::transform3d ttb = tb;
                                ttb.position += ttb.orientation * ccb.position;
                                ttb.orientation = ttb.orientation * ccb.orientation;
                                axiom::transform_vertices(vertices, ttb, vec3(0.0f));

                                for(axiom::vertex_element3d v : vertices) {
                                    position.push_back(v.center - point);
                                    tex_range.push_back(vec4(54, 96, 6, 6));
                                    color.push_back(vec4(0.35f, 1.0f, 1.0f, 1.0f));
                                    size.push_back(vec2(6, 6));
                                }
                            }
                            
                            position.push_back(ta.position - point);
                            tex_range.push_back(vec4(48, 104, 8, 8));
                            color.push_back(vec4(1.0f));
                            size.push_back(vec2(8, 8));
                            
                            position.push_back(tb.position - point);
                            tex_range.push_back(vec4(48, 104, 8, 8));
                            color.push_back(vec4(1.0f));
                            size.push_back(vec2(8, 8));

                            render_billboards(camera, position, tex_range, color, size, f.size, msystem.textures["ui"]);
                        }

                        glEnable(GL_DEPTH_TEST);
                    }
                }
            }
        };

        std::function<void()> on_close = [camera_entity]() {
            axiom::global_core.ecs->erase_entity(camera_entity);
        };
        
        std::vector<axiom::texture_format> formats = {axiom::texture_format::RGBA8, axiom::texture_format::DEPTHF};
        std::vector<axiom::texture_attachment> attachments = {axiom::texture_attachment::COLOR0, axiom::texture_attachment::DEPTH};

        msystem->targets.push_back(std::make_unique<axiom::render_target>(axiom::render_target::create(render_func, ivec2(400, 400), ivec2(0), formats, attachments)));

        //
        ui_system->input_reset();
        ui_system->input_z(0.1f);
        ui_system->buffer(vec4(0.0f));

        vec2 window_size = vec2(768, 384);
        axiom::window_widget::insert("Render", window_size, (vec2(ui_system->window->screen_size) - window_size) * 0.5f, axiom::color_blue, on_close);
        axiom::split_widget::insert(axiom::layout_mode::ROW, {{0.75f, axiom::panel_mode::SCALE}, {0.25f, axiom::panel_mode::SIZE}});
        
        msystem->targets.push_back(std::make_unique<axiom::render_target>(axiom::render_target::create(render_func, ivec2(400, 400), ivec2(0, 0), formats, attachments)));

        axiom::panel_widget::insert();
        axiom::render_widget::insert(msystem->targets.back().get(), 0, callback_func);
        
        ui_system->position(axiom::position_mode::TOP_RIGHT);
        
        ui_system->buffer(vec4(6.0f));
        axiom::column_widget::insert();
        
        axiom::row_widget::insert();

        axiom::button_widget::insert(vec2(32.0f), axiom::color_purple, vec4(0, 116, 12, 12), 
            [](axiom::button_widget& self) {
                if(self.pressed) {
                    debugger_mode = (debugger_mode + 1) % 2;
                }

                if(debugger_mode == 0) self.icon = vec4(0, 80, 12, 12);
                else if(debugger_mode == 1) self.icon = vec4(16, 80, 12, 12);
            }
        );
        
        ui_system->input_step();

        axiom::row_widget::insert();

        axiom::button_widget::insert(vec2(32.0f), axiom::color_red, vec4(64, 84, 12, 12), 
            [](axiom::button_widget& self) {
                static double timer = 0.0;
                static double freq = 0.0625;
                static double delay = 0.25;

                if(self.held) {
                    if(timer > freq || self.pressed) {
                        --selected_frame;
                    }

                    if(timer > freq) {
                        timer = glm::mod(timer, freq);
                    }

                    timer += axiom::delta_time();
                } else {
                    timer = -delay + freq;
                }
            }
        );
        
        axiom::button_widget::insert(vec2(32.0f), axiom::color_red, vec4(76, 84, 12, 12), 
            [](axiom::button_widget& self) {
                static double timer = 0.0;
                static double freq = 0.0625;
                static double delay = 0.25;

                if(self.held) {
                    if(timer > freq || self.pressed) {
                        ++selected_frame;
                    }

                    if(timer > freq) {
                        timer = glm::mod(timer, freq);
                    }

                    timer += axiom::delta_time();
                } else {
                    timer = -delay + freq;
                }
            }
        );

        ui_system->input_step();

        axiom::row_widget::insert();

        axiom::button_widget::insert(vec2(32.0f), axiom::color_red, vec4(64, 72, 12, 12), 
            [](axiom::button_widget& self) {
                static double timer = 0.0;
                static double freq = 0.0625;
                static double delay = 0.25;

                if(self.held) {
                    if(timer > freq || self.pressed) {
                        --selected_event;
                    }

                    if(timer > freq) {
                        timer = glm::mod(timer, freq);
                    }

                    timer += axiom::delta_time();
                } else {
                    timer = -delay + freq;
                }
            }
        );
        
        axiom::button_widget::insert(vec2(32.0f), axiom::color_red, vec4(76, 72, 12, 12), 
            [](axiom::button_widget& self) {
                static double timer = 0.0;
                static double freq = 0.0625;
                static double delay = 0.25;

                if(self.held) {
                    if(timer > freq || self.pressed) {
                        ++selected_event;
                    }

                    if(timer > freq) {
                        timer = glm::mod(timer, freq);
                    }

                    timer += axiom::delta_time();
                } else {
                    timer = -delay + freq;
                }
            }
        );

        ui_system->input_step();

        axiom::row_widget::insert();

        axiom::button_widget::insert(vec2(32.0f), axiom::color_red, vec4(64, 60, 12, 12), 
            [](axiom::button_widget& self) {
                static double timer = 0.0;
                static double freq = 0.0625;
                static double delay = 0.25;

                if(self.held) {
                    if(timer > freq || self.pressed) {
                        --selected_step;
                    }

                    if(timer > freq) {
                        timer = glm::mod(timer, freq);
                    }

                    timer += axiom::delta_time();
                } else {
                    timer = -delay + freq;
                }
            }
        );
        
        axiom::button_widget::insert(vec2(32.0f), axiom::color_red, vec4(76, 60, 12, 12), 
            [](axiom::button_widget& self) {
                static double timer = 0.0;
                static double freq = 0.0625;
                static double delay = 0.25;

                if(self.held) {
                    if(timer > freq || self.pressed) {
                        ++selected_step;
                    }

                    if(timer > freq) {
                        timer = glm::mod(timer, freq);
                    }

                    timer += axiom::delta_time();
                } else {
                    timer = -delay + freq;
                }
            }
        );

        ui_system->input_step();

        ui_system->input_step();

        //

        ui_system->buffer(vec4(4.0f));
        ui_system->position(axiom::position_mode::TOP_LEFT);
        axiom::column_widget::insert();
        axiom::match_widget::insert(vec4(0.5f, 0.5f, 0.5f, 0.25f), true);

        std::function<std::string(std::string)> debugger_info_func = [msystem](std::string prev) {
            axiom::physics_system3d& physics_system = axiom::get_system<axiom::physics_system3d>();

            std::string ret;
            ret += "selected event: " + std::to_string(selected_event) + "\n";
            ret += "selected step: " + std::to_string(selected_step) + "\n";
            ret += "selected physics frame: " + std::to_string(selected_frame) + "\n";
            ret += "cached frames: " + std::to_string(physics_system.debugger.frame_list.size()) + "\n\n";
            
            if(selected_frame < physics_system.debugger.frame_list.size()) {
                std::vector<axiom::collision_event> found_events;
                for(auto& ev : physics_system.debugger.frames[physics_system.debugger.frame_list[selected_frame]].collision_events) {
                    if(axiom::filter(ev)) found_events.push_back(ev);
                }
                if(found_events.size() == 0) found_events = physics_system.debugger.frames[physics_system.debugger.frame_list[selected_frame]].collision_events;
        
                ret += "events: " + std::to_string(found_events.size()) + "\n";

                //

                if(selected_event < found_events.size()) {
                    auto& event = found_events[selected_event];

                    ret += "frames: ";

                    uint num_frames = 0;
                    for(auto& gjk_step : event.gjk) {
                        if(length(gjk_step.search_origin) != 0.0f) ++num_frames;
                        if(gjk_step.erase_index != 0xFFFFFFFF) ++num_frames;
                        ++num_frames;
                    }
                    for(auto& epa_step : event.epa) {
                        ++num_frames;
                        if(length(epa_step.search_origin) != 0.0f) ++num_frames;
                        ++num_frames;
                    }
                    ret += std::to_string(num_frames);

                    ret += "\n\n";

                    if(event.finished) ret += "collision: TRUE";
                    else ret += "collision: FALSE";
                    ret += "\n";

                    ret += "GJK steps: " + std::to_string(event.gjk.size()) + "\n";
                    ret += "EPA steps: " + std::to_string(event.epa.size());
                } else ret += std::string("steps: -");
            } else ret += "events: -\n";

            return ret;
        };
        axiom::text_widget::insert(" ", axiom::text_alignment::LEFT, true, debugger_info_func);

        //
        ui_system->input_step();

        ui_system->input_step();
        axiom::panel_widget::insert();
    };
    
    std::shared_ptr<axiom::menu_node> node(new axiom::menu_node{
        "",
        {
            axiom::menu_node("Debug Windows", {
                axiom::menu_node("Profiler", {}, switch_profiler),
                axiom::menu_node("Phyiscs Debugger", {}, switch_debugger),
                axiom::menu_node("Camera", {}, switch_camera),
                axiom::menu_node("Render", {}, switch_render),
                axiom::menu_node("Lipsum", {}, switch_lipsum),
            })
        }
    });

    //

    std::function<void(axiom::tab_widget*)> func_chat_in = [ui_system, csystem](axiom::tab_widget* self) {
        ui_system->buffer(vec4(0.0f, 0.0f, 0.0f, 2.0f));

        ui_system->position(axiom::position_mode::TOP_LEFT);

        axiom::panel_widget::insert();
        
        ui_system->buffer(vec4(0.0f));
        axiom::column_widget::insert();
        axiom::scroll_widget::insert(8.0f, true);
        ui_system->buffer(vec4(0.0f));

        float buffer = 8.0f;

        ui_system->buffer(vec4(4.0f));
        ulong message_root = axiom::column_widget::insert();

        //
        ui_system->input_step(2);
        axiom::spacer_widget::insert(vec2(0.0f), vec2(FLT_MAX), false, vec4(0.0f), true);
        ui_system->buffer(vec4(8.0f));
        ui_system->position(axiom::position_mode::BOTTOM_LEFT);
        axiom::column_widget::insert();

        axiom::text_box_widget::insert(FLT_MAX, vec2(8.0f, 8.0f), ""//, 
            /*[&ui_system, buffer, message_root, self_color](axiom::text_box_widget& self) {
                auto& root = ui_system.widgets[message_root];
                ulong time_delta = 2.0f * 60.0f * 1000000.0f;

                if(self.text[0]->string.size()) {
                    insert_message(message_root, "Averie", self.text[0]->string, axiom::get_timestamp(), self_color);
                    axiom::scroll_widget* parent = dynamic_cast<axiom::scroll_widget*>(ui_system.widgets[root->parent].get());
                    parent->scroll_pos = -FLT_MAX * 0.5f;
                    parent->anchor_widget = 0xFFFFFFFFFFFFFFFD;

                    self.text[0]->string = "";
                }
            }*/
        );
        
        ui_system->set_attrib(vec2(160, 16), vec2(FLT_MAX, 16), vec2(1.0f));

        csystem->enable(message_root);
    };

    
    std::function<void(axiom::tab_widget*)> func_chat_out = [ui_system, csystem](axiom::tab_widget* self) {
        csystem->disable();

        auto children = ui_system->get_children(self->children[0]);
        children.push_back(self->children[0]);
        
        ui_system->erase(children);
        self->children.clear();
    };

    std::function<void(axiom::tab_widget*)> func_settings = [ui_system, node, msystem](axiom::tab_widget* self) {
        ui_system->buffer(vec4(0.0f, 0.0f, 0.0f, 2.0f));

        axiom::panel_widget::insert();

        ui_system->buffer(vec4(4.0f));
        ui_system->position(axiom::position_mode::TOP_LEFT);

        axiom::column_widget::insert();
        axiom::grid_widget::insert(3);

        //
        
        ui_system->position(axiom::position_mode::CENTER_LEFT);
        axiom::text_widget::insert("light azimuth", axiom::text_alignment::LEFT, false);
        axiom::spacer_widget::insert(vec2(0, 0), vec2(FLT_MAX, 0), false);
        axiom::row_widget::insert();
        ui_system->set_attrib(vec2(160, 16), vec2(160, 16), vec2(1.0f));

        axiom::slider_widget::insert(vec2(120, 16), 6, axiom::color_blue, vec2(-180.0f, 180.0f), 0.0f, 0.0f, "",
            [msystem](axiom::slider_widget& self) {
                if(!self.pressed) self.current_value = msystem->light_azimuth;
                else msystem->light_azimuth = self.current_value;

                self.text[0]->string = axiom::to_base(self.current_value, 10, 3);
            }
        );
        ui_system->set_attrib(vec2(0, 16), vec2(FLT_MAX, 16), vec2(1.0f));
        ui_system->input_step();
        
        ui_system->position(axiom::position_mode::CENTER_LEFT);
        axiom::text_widget::insert("light altitude", axiom::text_alignment::LEFT, false);
        axiom::spacer_widget::insert(vec2(0, 0), vec2(FLT_MAX, 0), false);
        axiom::row_widget::insert();
        ui_system->set_attrib(vec2(160, 16), vec2(160, 16), vec2(1.0f));

        axiom::slider_widget::insert(vec2(120, 16), 6, axiom::color_blue, vec2(-90.0f, 90.0f), 0.0f, 0.0f, "",
            [msystem](axiom::slider_widget& self) {
                if(!self.pressed) self.current_value = msystem->light_altitude;
                else msystem->light_altitude = self.current_value;

                self.text[0]->string = axiom::to_base(self.current_value, 10, 3);
            }
        );
        ui_system->set_attrib(vec2(0, 16), vec2(FLT_MAX, 16), vec2(1.0f));
        ui_system->input_step();

        ui_system->input_step();
    };

    axiom::screen_widget::insert("axiom text", axiom::color_blue, ui_system->window);
    axiom::relative_widget::insert(
        [ui_system](axiom::relative_widget& widget) {
            axiom::screen_widget* parent = (axiom::screen_widget*)ui_system->widgets[widget.parent].get();
            widget.position = parent->position.x + vec2(parent->header, parent->size.y - parent->header);

            for(auto child : widget.children) {
                auto& child_widget = ui_system->widgets[child];

                child_widget->position = widget.position;
                child_widget->size = vec2(parent->size.x - parent->header, parent->header);
            }
        }
    );

    ui_system->position(axiom::position_mode::CENTER_LEFT);
    axiom::row_widget::insert();

    axiom::button_widget::insert(vec2(40.0f, 16.0f), axiom::color_blue, "TEST", 
        [node, ui_system](axiom::button_widget& self) {
            if(self.pressed) {
                ui_system->position(axiom::position_mode::TOP_LEFT);

                ui_system->input_reset();
                axiom::menu_widget::insert(self.position, 0.001, axiom::color_blue, 200, 16, FLT_MAX, node, {});
            }
        }
    );
    
    ui_system->input_root(1);
    ui_system->position(axiom::position_mode::TOP_LEFT);

    axiom::split_widget::insert(axiom::layout_mode::ROW, {{1.0f, axiom::panel_mode::SCALE}, {1.0f, axiom::panel_mode::SCALE}});
    axiom::panel_widget::insert();
    
    ui_system->buffer(vec4(0.0f, 0.0f, 0.0f, 2.0f));
    axiom::tab_widget::insert(24.0f, 2.0f, {
        axiom::tab("Settings", 80.0f, axiom::color_blue, func_settings),
        axiom::tab("Chat", 80.0f, axiom::color_blue, func_chat_in, func_chat_out),
    });

    //
    //
    //

    ui_system->input_root(2);
    
    axiom::panel_widget::insert();
    
    //
    
    ui_system->buffer(vec4(0.0f, 0.0f, 0.0f, 0.0f));

    //
    
    uint camera_entity = axiom::insert_entity();
    axiom::camera3d cam;
    axiom::transform3d tf;

    cam.fov = 90.0f;

    tf.position = normalize(vec3(0x8945B, 0x5BAD2, 0x53445)) * 350000.0f;
    tf.orientation = axiom::rotate_to(vec3(0.0f, 0.0f, -1.0f), vec3(0.0f, 1.0f, 0.0f));

    axiom::insert_component(camera_entity, cam);
    axiom::insert_component(camera_entity, tf);

    std::function<void(axiom::render_widget*, axiom::render_target*)> callback_func = [msystem, ui_system, camera = camera_entity](axiom::render_widget* self, axiom::render_target* target) {
        static bool movement_capture = false;
        static bool raycast_capture = false;
        static float movement_speed = 8.0f;

        static uint constraint_index = 0xFFFFFFFF;
        static float constraint_dist = 0.0f;
        
        axiom::transform3d& camera_transform = axiom::get_component<axiom::transform3d>(camera);

        if(ui_system->click_capture == self->self) {
            if(ui_system->window->input_map[axiom::input_code::KEY_LEFT_CTRL]) {
                if(raycast_capture == false) {
                    raycast_capture = true;
                    
                    axiom::transform3d& camera_transform = axiom::get_component<axiom::transform3d>(camera);
                    axiom::camera3d& camera_cam = axiom::get_component<axiom::camera3d>(camera);

                    //

                    vec2 screen_pos = (ui_system->window->cursor_pos - self->position) / self->size;
                    screen_pos = screen_pos * 2.0f - 1.0f;

                    vec4 vertex = vec4(screen_pos, 0.5f, 1.0f);

                    mat4 proj = axiom::get_proj(camera_cam);
                    mat4 inv_proj = glm::inverse(proj);

                    vertex = inv_proj * vertex;
                    vertex /= vertex.w;

                    vec3 dir = glm::normalize(camera_transform.orientation * vertex.xyz());

                    //

                    auto& psystem = axiom::global_core.ecs->get_system<axiom::physics_system3d>();

                    psystem.gravity = [](vec3 pos) {
                        return normalize(pos - vec3(0.0f, 0.0f, 0.0f)) * -27.5f;
                    };

                    uint hit;
                    uint shape_hit;
                    vec3 normal;
                    vec3 point;
                    std::unordered_set<uint> mask;

                    psystem.raycast(camera_transform.position, dir, 1.0f, 20.0f, 0.0f, mask, &hit, &shape_hit, &normal, &point);

                    std::cout << std::hex << hit << std::dec << std::endl;
                    axiom::param_collider = hit;
                    render_points_shape = hit;

                    if(hit != axiom::NULL_ENTITY) {
                        axiom::position_constraint pc;
                        pc.vs = {vec3(1.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f), vec3(0.0f, 0.0f, 1.0f)};
                        pc.a = hit;

                        axiom::transform3d& target_transform = axiom::get_component<axiom::transform3d>(hit);
                        pc.va = transpose(target_transform.orientation) * (point - target_transform.position);
                        pc.vb = point;
                        pc.max_impulse = axiom::get_component<axiom::collider3d>(hit).mass * 30.0f * psystem.physics_step * 2.0f;

                        constraint_dist = length(point - camera_transform.position);

                        //constraint_index = psystem.constraints.size();
                        //psystem.constraints.push_back(std::make_unique<axiom::position_constraint>(pc));
                    }
                }
            } else {
                if(movement_capture == false && raycast_capture == false) {
                    ui_system->hide_cursor();
                    movement_capture = true;
                }
            }
        } else {
            if(raycast_capture && constraint_index != 0xFFFFFFFF) {
                auto& psystem = axiom::global_core.ecs->get_system<axiom::physics_system3d>();
                psystem.constraints.erase(psystem.constraints.begin() + constraint_index);

                constraint_index = 0xFFFFFFFF;
            }

            if(movement_capture) ui_system->show_cursor();
            movement_capture = false;
            raycast_capture = false;
        }

        if(ui_system->hover_capture == self->self) {
            if(ui_system->window->scroll_delta != 0.0f) {
                movement_speed *= pow(2, ui_system->window->scroll_delta * 0.5f);
            }
        }

        if(ui_system->click_capture == self->self) {
            if(movement_capture) {
                glm::vec3 raw_movement = {0, 0, 0};
                float rotate_value = 0.0f;

                vec3 rotate = vec3(0.0f);
                rotate.x = -ui_system->window->cursor_delta.x;
                rotate.y = -ui_system->window->cursor_delta.y;

                if(ui_system->window->input_map[axiom::input_code::KEY_Q]) {
                    rotate.z -= 1;
                }
                if(ui_system->window->input_map[axiom::input_code::KEY_E]) {
                    rotate.z += 1;
                }

                //

                if(ui_system->window->input_map[axiom::input_code::KEY_A]) {
                    raw_movement.x -= 1;
                }
                if(ui_system->window->input_map[axiom::input_code::KEY_D]) {
                    raw_movement.x += 1;
                } 
                if(ui_system->window->input_map[axiom::input_code::KEY_S]) {
                    raw_movement.z += 1;
                }
                if(ui_system->window->input_map[axiom::input_code::KEY_W]) {
                    raw_movement.z -= 1;
                }
                if(ui_system->window->input_map[axiom::input_code::KEY_SPACE]) {
                    raw_movement.y += 1;
                }
                if(ui_system->window->input_map[axiom::input_code::KEY_LEFT_SHIFT]) {
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
                glm::mat3 rotate_z_mat = (mat3)glm::rotate(float(2 * axiom::pi * (1.0 / 128) * rotate.z * (axiom::global_core.ecs->delta_time * 60)), dir);

                camera_transform.orientation = rotate_z_mat * rotate_x_mat * rotate_y_mat * camera_transform.orientation;
                camera_transform.position += translation_vec * (float)axiom::global_core.ecs->delta_time * movement_speed;
            } else if(raycast_capture && constraint_index != 0xFFFFFFFF) {
                uint camera = *axiom::global_core.ecs->collectors["camera"].entities.begin();
                axiom::transform3d& camera_transform = axiom::get_component<axiom::transform3d>(camera);
                axiom::camera3d& camera_cam = axiom::get_component<axiom::camera3d>(camera);

                //

                vec2 screen_pos = (ui_system->window->cursor_pos - self->position) / self->size;
                screen_pos = screen_pos * 2.0f - 1.0f;

                vec4 vertex = vec4(screen_pos, 0.5f, 1.0f);

                mat4 proj = axiom::get_proj(camera_cam);
                mat4 inv_proj = glm::inverse(proj);

                vertex = inv_proj * vertex;
                vertex /= vertex.w;

                vec3 dir = glm::normalize(camera_transform.orientation * vertex.xyz());

                vec3 new_point = camera_transform.position + dir * constraint_dist;
                
                //

                auto& psystem = axiom::global_core.ecs->get_system<axiom::physics_system3d>();

                axiom::position_constraint* c = dynamic_cast<axiom::position_constraint*>(psystem.constraints[constraint_index].get());
                
                c->vb = new_point;
            }
        }
    };

    {
        std::function<void(axiom::render_target&)> render_func = [msystem, ui_system, camera = camera_entity](axiom::render_target& f) {
            base_render(camera, f);
        };

        std::vector<axiom::texture_format> formats = {axiom::texture_format::RGBA8, axiom::texture_format::RGBA8, axiom::texture_format::RGBA8, axiom::texture_format::DEPTHF};
        std::vector<axiom::texture_attachment> attachments = {axiom::texture_attachment::COLOR0, axiom::texture_attachment::COLOR1, axiom::texture_attachment::COLOR2, axiom::texture_attachment::DEPTH};

        msystem->targets.push_back(std::make_unique<axiom::render_target>(axiom::render_target::create(render_func, ivec2(400, 400), ivec2(0, 0), formats, attachments)));
    }

    axiom::render_widget::insert(msystem->targets[0].get(), 0, callback_func);

    ui_system->position(axiom::position_mode::TOP_LEFT);

    ui_system->buffer(vec4(4.0f));
    axiom::column_widget::insert();
    axiom::match_widget::insert(vec4(0.5f, 0.5f, 0.5f, 0.25f), true);

    std::function<std::string(std::string)> fps_func = [msystem](std::string prev) {
        static double time = axiom::get_time();

        double current_time = axiom::get_time();
        if(current_time - time > 1.0) {

            double elapsed = current_time - time;

            double fps = double(msystem->frames) / elapsed;

            time = current_time;
            msystem->frames = 0;

            return "FPS: " + std::to_string(fps);
        }
        return prev;
    };

    axiom::text_widget::insert("FPS: ", axiom::text_alignment::LEFT, true, fps_func);

    std::function<std::string(std::string)> position_func = [msystem](std::string prev) {
        uint camera = *axiom::global_core.ecs->collectors["camera"].entities.begin();

        axiom::transform3d& transform = axiom::get_component<axiom::transform3d>(camera);
        //axiom::camera2d& cam = axiom::get_component<axiom::camera2d>(camera);
        
        return "position: " + axiom::to_base(transform.position.x, 16, 3) + " " + axiom::to_base(transform.position.y, 16, 3) + " " + axiom::to_base(transform.position.z, 16, 3);
    };

    axiom::text_widget::insert("position: ", axiom::text_alignment::LEFT, true, position_func);

    std::function<std::string(std::string)> direction_func = [msystem](std::string prev) {
        uint camera = *axiom::global_core.ecs->collectors["camera"].entities.begin();

        axiom::transform3d& transform = axiom::get_component<axiom::transform3d>(camera);
        //axiom::camera2d& cam = axiom::get_component<axiom::camera2d>(camera);
        vec3 direction = -transform.orientation[2];
        
        return "direction: " + axiom::to_base(direction.x, 16, 3) + " " + axiom::to_base(direction.y, 16, 3) + " " + axiom::to_base(direction.z, 16, 3);
    };

    axiom::text_widget::insert("direction: ", axiom::text_alignment::LEFT, true, direction_func);
    
    ui_system->input_step();
    ui_system->position(axiom::position_mode::TOP_RIGHT);
    ui_system->buffer(vec4(6.0f));
    axiom::column_widget::insert();
    axiom::row_widget::insert();

    axiom::button_widget::insert(vec2(32.0f), axiom::color_purple, vec4(0, 116, 12, 12), 
        [ui_system](axiom::button_widget& self) {
            static bool update = true;
            static bool psym = false;

            auto* physics = &axiom::global_core.ecs->get_system<axiom::physics_system3d>();
            auto& parent_widget = ui_system->widgets[self.parent];

            if(self.pressed || ui_system->window->pressed_buttons.contains(axiom::input_code::KEY_F5)) {
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
                            ui_system->widgets.erase(child);
                        }
                    }

                    self.icon = vec4(0, 116, 12, 12);
                } else {
                    ui_system->input_set(self.parent);
                    ui_system->buffer(vec4(6.0f));

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
    
    ui_system->input_step();
    axiom::row_widget::insert();

    axiom::button_widget::insert(vec2(32.0f), axiom::color_red, vec4(48, 116, 12, 12), 
        [ui_system](axiom::button_widget& self) {
            if(self.pressed) {
                render_grid = !render_grid;
                if(render_grid) self.color = axiom::color_red;
                else self.color = axiom::color_red * 0.75f;
            }
        }
    );
};

int main(int argc, char* argv[]) {
    axiom::window win = axiom::window(ivec2(64, 64), ivec2(512, 512), 6, "axiom test", false);

    //

    axiom::font_asset default_font = axiom::font_asset::load(resource_root + "fonts/axiom_default.bdf");
    axiom::texture font_tex = std::move(axiom::texture(default_font.texture, axiom::texture_format::RGBA8));

    generate_placeholder();

    //

    win.hide_cursor();

    axiom::ecs ecs;
    ecs.make_active();
    
    axiom::ui_system ui_system(&win);
    ui_system.font_assets = {&default_font};

    ecs.register_system(ui_system);
    
    chat_system csystem;
    ecs.register_system(csystem);

    axiom::physics_system2d psystem2d;
    ecs.register_system(psystem2d);
    
    axiom::physics_system3d psystem3d;
    ecs.register_system(psystem3d);

    main_system msystem(&win);
    msystem.textures.emplace("font_axiom_default", std::move(font_tex));
    ecs.register_system(msystem);
    
    axiom::signature sig = axiom::global_core.ecs->update_signature<axiom::transform3d>();
    axiom::global_core.ecs->update_signature<axiom::camera3d>(sig);
    axiom::collector col(sig);
    axiom::global_core.ecs->create_collector("camera", col);
   
    sig = axiom::global_core.ecs->update_signature<axiom::transform3d>();
    axiom::global_core.ecs->update_signature<axiom::color_mesh3d>(sig);
    col = axiom::collector(sig);
    axiom::global_core.ecs->create_collector("color_mesh3d", col);
    sig = axiom::global_core.ecs->update_signature<axiom::transform3d>();
    axiom::global_core.ecs->update_signature<axiom::texture_mesh3d>(sig);
    col = axiom::collector(sig);
    axiom::global_core.ecs->create_collector("texture_mesh3d", col);
    
    sig = axiom::global_core.ecs->update_signature<axiom::transform3d>();
    axiom::global_core.ecs->update_signature<axiom::texture_range_mesh3d>(sig);
    col = axiom::collector(sig);
    axiom::global_core.ecs->create_collector("texture_range_mesh3d", col);
    //

    float separation = 128000000.0f;
    vec3 direction = normalize(vec3(0.3f, 0.8f, 0.0f));

    {
        world_params params;
        params.num_tiles = 64;
        params.num_chunks = 4;
        params.seed = axiom::get_timestamp();
        params.orientation = glm::identity<mat3>();//axiom::rotate_to(vec3(1.0f, 0.0f, 0.0f), -direction);
        params.position = vec3(0.0f);//direction * 0.429f * separation;
        params.radii = vec3(180000.0f);

        make_world(params);
    }
    
    /*
    {
        world_params params;
        params.num_tiles = 64;
        params.num_chunks = 4;
        params.seed = axiom::get_timestamp() + 0x50F;
        params.orientation = axiom::rotate_to(vec3(1.0f, 0.0f, 0.0f), direction);
        params.position = -direction * (1.0f - 0.429f) * separation;
        params.radii = vec3(1707000.0f);

        make_world(params);
    }*/

    //

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

void make_world(world_params params) {
    std::vector<crater_population> populations;

    // blue moon
    populations.clear();
    populations.push_back({0.05f, 0.175f, 4.0f, 20, 0});
    populations.push_back({0.015f, 0.05f, 4.0f, 2500, 75});
    populations.push_back({0.001f, 0.015f, 4.0f, 50000, 300});
    std::vector<vec3> colors = {
        axiom::hsv_color(5.85, 0.4, 0.15),
        axiom::hsv_color(5.85, 0.4, 0.125),
        axiom::hsv_color(5.85, 0.4, 0.25),
        axiom::hsv_color(5.85, 0.4, 0.25)
    };

    uint seed = params.seed;
    vec3 position = params.position;
    mat3 orientation = params.orientation;
    vec3 dimensions = params.radii;
    float amplitude = 1000.0f;
    float noise_freq = 0.3f;
    float noise_offset = 0.0f;
    float age_value = 1.0f;
    float ejecta_value = 0.25f;
    float blend_value = 0.125f;

    create_planet(seed, position, orientation, dimensions, populations, colors, amplitude, noise_freq, noise_offset, age_value, ejecta_value, blend_value, params.num_chunks, params.num_tiles);
}