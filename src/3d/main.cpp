#include <include/core.hpp>
#include <include/render.hpp>
#include <include/ui.hpp>
#include <include/physics3d.hpp>

#include "physics-objects.hpp"

struct star {
    vec3 dir;
    vec3 color;
    float luminosity;
};

void render_billboards(uint camera, std::vector<vec3> origins, std::vector<vec4> textures, std::vector<vec4> colors, std::vector<vec2> sizes, ivec2 framebuffer_size, axiom::texture& texture) {
    static axiom::vertices vertices;
    if (!vertices.initialized)
        vertices.init();

    axiom::transform3d camera_transform = axiom::ecs.get_component<axiom::transform3d>(camera);
    axiom::camera3d& camera_cam = axiom::ecs.get_component<axiom::camera3d>(camera);
    mat4 view = axiom::get_view(camera_cam, camera_transform);
    mat4 proj = axiom::get_proj(camera_cam);

    std::vector<axiom::texture_vertex3d> tvs;

    axiom::transform3d transform;
    transform.position = vec3(0.0f);
    transform.orientation = glm::identity<mat3>();

    mat4 model = axiom::get_model(transform, camera_transform);

    mat4 inv_model = glm::inverse(model);
    mat4 inv_view = glm::inverse(view);
    mat4 inv_proj = glm::inverse(proj);

    uint i = 0;
    for(vec3 vvv : origins) {
        vec4 pos = view * model * vec4(vvv, 1.0f);

        pos = proj * pos;
        pos /= pos.w;
        
        vec2 pp = pos.xy() * vec2(framebuffer_size) * 0.5f;
        pp = round(pp);
        pp /= vec2(framebuffer_size) * 0.5f;
        
        vec4 pcenter = vec4(pp, pos.z, 1.0);
        pcenter = inv_proj * pcenter;
        pcenter /= pcenter.w;

        //

        vec2 size = sizes[i] / vec2(framebuffer_size);

        vec4 p = vec4(size, pos.z, 1.0f);
        p = inv_proj * p;
        p /= p.w;

        size = glm::abs(p.xy());

        pcenter = inv_model * inv_view * pcenter;

        vvv = pcenter.xyz();

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
    // glUniform1f(3, msystem.light_contrast);

    vertices.draw_vertices_triangles();
}

std::vector<star> create_stars(uint num_stars) {
    axiom::random32 rand(0xFF8F);
    std::vector<star> stars;

    for(int i = 0; i < num_stars; ++i) {
        star s;
        s.dir = rand.unit_vector();
        s.luminosity = pow(rand(), 14) * 5.75f + 0.25f;

        float col = rand() * 3;
        float f = glm::fract(col);
        if(col < 1.0f) {
            s.color = axiom::hsv_color(f, 1.0f, 1.0f);
        } else if(col < 2.0) {
            s.color = axiom::hsv_color(1.0f, 1.0f - f, 1.0f);
        } else {
            s.color = axiom::hsv_color(3.5f, f, 1.0f);
        }

        s.color = s.color * 0.25f + 0.75f;

        stars.push_back(s);
    }

    return stars;
}

void render_lines(uint camera, std::vector<vec3> points, std::vector<vec4> colors) {
    static axiom::vertices vertices;
    if (!vertices.initialized)
        vertices.init();

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
    // glUniform1f(3, msystem.light_contrast);

    vertices.draw_vertices_lines();
}

auto base_render = [](uint camera_entity, axiom::framebuffer& framebuffer) {
    static auto stars = create_stars(5000);
    
    axiom::transform3d& camera_transform = axiom::ecs.get_component<axiom::transform3d>(camera_entity);
    axiom::camera3d& camera = axiom::ecs.get_component<axiom::camera3d>(camera_entity);
    
    std::vector<vec3> origins;
    std::vector<vec4> textures;
    std::vector<vec4> colors;
    std::vector<vec2> sizes;

    for(auto star : stars) {
        vec3 pos = camera_transform.position + star.dir * length(camera_transform.position) * 0.25f;

        vec4 color = vec4(star.color, 1.0f);

        uint lum = floor(star.luminosity);
        float frac = glm::fract(star.luminosity);

        switch(lum) {
            case 0: {
                color.w = frac;

                origins.push_back(pos);
                textures.push_back(vec4(0.0f, 48.0f, 2.0f, 2.0f));
                sizes.push_back(vec2(2.0f));
                colors.push_back(color);

                break;
            }
            case 1: {
                color.w = 1.0f - frac;

                origins.push_back(pos);
                textures.push_back(vec4(0.0f, 48.0f, 2.0f, 2.0f));
                sizes.push_back(vec2(2.0f));
                colors.push_back(color);

                color.w = frac;
                origins.push_back(pos);
                textures.push_back(vec4(2.0f, 48.0f, 4.0f, 4.0f));
                sizes.push_back(vec2(4.0f));
                colors.push_back(color);

                break;
            }
            case 2: {
                color.w = 1.0f - frac;
                origins.push_back(pos);
                textures.push_back(vec4(2.0f, 48.0f, 4.0f, 4.0f));
                sizes.push_back(vec2(4.0f));
                colors.push_back(color);

                color.w = frac;
                origins.push_back(pos);
                textures.push_back(vec4(6.0f, 48.0f, 6.0f, 6.0f));
                sizes.push_back(vec2(6.0f));
                colors.push_back(color);

                break;
            }
            case 3: {
                color.w = 1.0 - frac;
                origins.push_back(pos);
                textures.push_back(vec4(6.0f, 48.0f, 6.0f, 6.0f));
                sizes.push_back(vec2(6.0f));
                colors.push_back(color);

                color.w = frac;
                origins.push_back(pos);
                textures.push_back(vec4(12.0f, 48.0f, 8.0f, 8.0f));
                sizes.push_back(vec2(8.0f));
                colors.push_back(color);

                break;
            }
            case 4: {
                color.w = 1.0 - frac;
                origins.push_back(pos);
                textures.push_back(vec4(12.0f, 48.0f, 8.0f, 8.0f));
                sizes.push_back(vec2(8.0f));
                colors.push_back(color);

                color.w = frac;
                origins.push_back(pos);
                textures.push_back(vec4(20.0f, 48.0f, 10.0f, 10.0f));
                sizes.push_back(vec2(10.0f));
                colors.push_back(color);

                break;
            }
            case 5: {
                color.w = 1.0 - frac;
                origins.push_back(pos);
                textures.push_back(vec4(20.0f, 48.0f, 10.0f, 10.0f));
                sizes.push_back(vec2(10.0f));
                colors.push_back(color);

                color.w = frac;
                origins.push_back(pos);
                textures.push_back(vec4(30.0f, 48.0f, 12.0f, 12.0f));
                sizes.push_back(vec2(12.0f));
                colors.push_back(color);

                break;
            }
        }
    }

    glDisable(GL_DEPTH_TEST);
    render_billboards(camera_entity, origins, textures, colors, sizes, framebuffer.size, axiom::get_texture("tilesheet"));
    
    glEnable(GL_DEPTH_TEST);

    //

    mat4 view = axiom::get_view(camera, camera_transform);
    mat4 proj = axiom::get_proj(camera);

    float light_contrast = 0.75f;

    // render shape

    glEnable(GL_DEPTH_TEST);

    auto& collector_color = axiom::ecs.collectors["color_mesh3d"];

    for(uint entity : collector_color.entities) {
        axiom::transform3d& transform = axiom::ecs.get_component<axiom::transform3d>(entity);
        axiom::color_mesh3d& mesh = axiom::ecs.get_component<axiom::color_mesh3d>(entity);

        mat4 model = axiom::get_model(transform, camera_transform);
        axiom::shader& color_shader = axiom::get_shader("color3d");

        vec3 light_dir = normalize(vec3(1.0f, 1.0f, 1.0f));

        //

        color_shader.use();

        glUniformMatrix4fv(0, 1, false, &model[0][0]);
        glUniformMatrix4fv(1, 1, false, &view[0][0]);
        glUniformMatrix4fv(2, 1, false, &proj[0][0]);
        glUniform1f(3, light_contrast);

        mesh.vertices->draw_vertices_triangles();
    }

    auto& collector_texture = axiom::ecs.collectors["texture_mesh3d"];

    for(uint entity : collector_texture.entities) {
        axiom::transform3d& transform = axiom::ecs.get_component<axiom::transform3d>(entity);
        axiom::texture_mesh3d& mesh = axiom::ecs.get_component<axiom::texture_mesh3d>(entity);

        mat4 model = axiom::get_model(transform, camera_transform);
        axiom::shader& texture_shader = axiom::get_shader("texture3d");

        vec3 light_dir = normalize(vec3(1.0f, 1.0f, 1.0f));

        //

        texture_shader.use();
        mesh.texture->bind(0);

        glUniformMatrix4fv(0, 1, false, &model[0][0]);
        glUniformMatrix4fv(1, 1, false, &view[0][0]);
        glUniformMatrix4fv(2, 1, false, &proj[0][0]);
        glUniform1f(3, light_contrast);

        mesh.vertices->draw_vertices_triangles();
    }

    auto& collector_texture_range = axiom::ecs.collectors["texture_range_mesh3d"];

    for(uint entity : collector_texture_range.entities) {
        axiom::transform3d& transform = axiom::ecs.get_component<axiom::transform3d>(entity);
        axiom::texture_range_mesh3d& mesh = axiom::ecs.get_component<axiom::texture_range_mesh3d>(entity);

        mat4 model = axiom::get_model(transform, camera_transform);
        axiom::shader& texture_shader = axiom::get_shader("texture_range3d");

        //

        texture_shader.use();
        mesh.texture->bind(0);

        glUniformMatrix4fv(0, 1, false, &model[0][0]);
        glUniformMatrix4fv(1, 1, false, &view[0][0]);
        glUniformMatrix4fv(2, 1, false, &proj[0][0]);
        glUniform1f(3, light_contrast);

        mesh.vertices->draw_vertices_triangles();
    }
};

void render_skybox(uint camera_entity, axiom::framebuffer& framebuffer) {
    axiom::transform3d& camera_transform = axiom::ecs.get_component<axiom::transform3d>(camera_entity);
    axiom::camera3d& camera = axiom::ecs.get_component<axiom::camera3d>(camera_entity);

    glDisable(GL_DEPTH_TEST);

    auto& vertices = axiom::get_vertices();

    std::vector<vec2> vs = {
        vec2(-1.0f, -1.0f),
        vec2(1.0f, -1.0f),
        vec2(-1.0f, 1.0f),
        vec2(1.0f, 1.0f)};

    vs = {vs[0], vs[1], vs[3], vs[0], vs[3], vs[2]};

    vertices.vertex_buffer_data(vs.data(), vs.size(), sizeof(vec2), GL_STATIC_DRAW);
    vertices.add_vertex_attribute(0, 2, GL_FLOAT, false, sizeof(vec2), 0);

    axiom::transform3d grid_transform;
    grid_transform.position = vec3(0.0f);
    grid_transform.orientation = glm::identity<mat3>();

    mat4 model = axiom::get_model(grid_transform, camera_transform);
    mat4 view = axiom::get_view(camera, camera_transform);
    mat4 proj = axiom::get_proj(camera);

    axiom::get_shader("skybox").use();

    framebuffer.textures[0].bind(0);
    framebuffer.textures[framebuffer.depth_texture].bind(1);
    vertices.bind();

    axiom::push_uniform(0, &model);
    axiom::push_uniform(1, &view);
    axiom::push_uniform(2, &proj);
    axiom::push_uniform(3, vec3(axiom::max_float));

    vertices.draw_vertices_triangles();
    
    glEnable(GL_DEPTH_TEST);
}

void render_grid(uint camera_entity, axiom::framebuffer& framebuffer) {
    axiom::transform3d& camera_transform = axiom::ecs.get_component<axiom::transform3d>(camera_entity);
    axiom::camera3d& camera = axiom::ecs.get_component<axiom::camera3d>(camera_entity);

    auto& vertices = axiom::get_vertices();

    std::vector<vec2> vs = {
        vec2(-1.0f, -1.0f),
        vec2(1.0f, -1.0f),
        vec2(-1.0f, 1.0f),
        vec2(1.0f, 1.0f)};

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

std::function<void(axiom::framebuffer&, axiom::transform3d&, mat4, mat4, float)> shadow_func = [](axiom::framebuffer& fbuffer, axiom::transform3d& shadow_transform, mat4 view, mat4 proj, float pixel_size){
    fbuffer.bind();
    mat4 model = glm::identity<mat4>();

    //glCullFace(GL_FRONT);
    //glEnable(GL_CULL_FACE);

    float light_contrast = 0.75f;

    // render shape

    glDisable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

    auto& collector_color = axiom::ecs.collectors["color_mesh3d"];

    for(uint entity : collector_color.entities) {
        axiom::transform3d& transform = axiom::ecs.get_component<axiom::transform3d>(entity);
        axiom::color_mesh3d& mesh = axiom::ecs.get_component<axiom::color_mesh3d>(entity);

        if(mesh.radius > pixel_size) {
            mat4 model = axiom::get_model(transform, shadow_transform);
            axiom::shader& color_shader = axiom::get_shader("color3d");

            vec3 light_dir = normalize(vec3(1.0f, 1.0f, 1.0f));

            //

            color_shader.use();

            glUniformMatrix4fv(0, 1, false, &model[0][0]);
            glUniformMatrix4fv(1, 1, false, &view[0][0]);
            glUniformMatrix4fv(2, 1, false, &proj[0][0]);
            glUniform1f(3, light_contrast);

            mesh.vertices->draw_vertices_triangles();
        }
    }

    auto& collector_texture = axiom::ecs.collectors["texture_mesh3d"];

    for(uint entity : collector_texture.entities) {
        axiom::transform3d& transform = axiom::ecs.get_component<axiom::transform3d>(entity);
        axiom::texture_mesh3d& mesh = axiom::ecs.get_component<axiom::texture_mesh3d>(entity);

        if(mesh.radius > pixel_size) {
            mat4 model = axiom::get_model(transform, shadow_transform);
            axiom::shader& texture_shader = axiom::get_shader("texture3d");

            vec3 light_dir = normalize(vec3(1.0f, 1.0f, 1.0f));

            //

            texture_shader.use();
            mesh.texture->bind(0);

            glUniformMatrix4fv(0, 1, false, &model[0][0]);
            glUniformMatrix4fv(1, 1, false, &view[0][0]);
            glUniformMatrix4fv(2, 1, false, &proj[0][0]);
            glUniform1f(3, light_contrast);

            mesh.vertices->draw_vertices_triangles();
        }
    }

    auto& collector_texture_range = axiom::ecs.collectors["texture_range_mesh3d"];

    for(uint entity : collector_texture_range.entities) {
        axiom::transform3d& transform = axiom::ecs.get_component<axiom::transform3d>(entity);
        axiom::texture_range_mesh3d& mesh = axiom::ecs.get_component<axiom::texture_range_mesh3d>(entity);

        if(mesh.radius > pixel_size) {
            mat4 model = axiom::get_model(transform, shadow_transform);
            axiom::shader& texture_shader = axiom::get_shader("texture_range3d");

            //

            texture_shader.use();
            mesh.texture->bind(0);

            glUniformMatrix4fv(0, 1, false, &model[0][0]);
            glUniformMatrix4fv(1, 1, false, &view[0][0]);
            glUniformMatrix4fv(2, 1, false, &proj[0][0]);
            glUniform1f(3, light_contrast);

            mesh.vertices->draw_vertices_triangles();
        }
    }

    //glDisable(GL_CULL_FACE);
};

int main(int argc, char **argv) {
    axiom::window window(ivec2(256), ivec2(512), 0, "Axiom", false);

    axiom::ui_init(&window);
    axiom::render_init(&window);
    axiom::physics3d_init();

    axiom::physics_system3d& physics_system = axiom::ecs.get_system<axiom::physics_system3d>();
    physics_system.sim_active = false;

    axiom::render_system& render_system = axiom::ecs.get_system<axiom::render_system>();
    render_system.targets.reserve(5);
    
    {
        axiom::text_asset vert;
        axiom::text_asset frag;
        axiom::texture_asset texasset;

        vert = axiom::text_asset::load(render_system.resource_root + "/shaders/skybox.vert");
        frag = axiom::text_asset::load(render_system.resource_root + "/shaders/skybox.frag");
        render_system.shaders.emplace("skybox", std::move(axiom::shader(vert, frag)));
    }

    //axiom::ecs.get_system<axiom::ui_system>().font_handler.process_ttf("res/Oxanium-Medium.ttf", "test");

    // create collectors
    axiom::signature sig;
    axiom::collector col;

    sig = axiom::update_signature<axiom::transform3d>();
    axiom::update_signature<axiom::color_mesh3d>(sig);
    col = axiom::collector(sig);
    axiom::ecs.create_collector("color_mesh3d", col);
    sig = axiom::update_signature<axiom::transform3d>();
    axiom::update_signature<axiom::texture_mesh3d>(sig);
    col = axiom::collector(sig);
    axiom::ecs.create_collector("texture_mesh3d", col);
    sig = axiom::update_signature<axiom::transform3d>();
    axiom::update_signature<axiom::texture_range_mesh3d>(sig);
    col = axiom::collector(sig);
    axiom::ecs.create_collector("texture_range_mesh3d", col);

    //

    create_cuboid(vec3(0.0f), glm::identity<mat3>(), vec3(64.0f, 64.0f, 0.5f), vec3(0.75f, 0.75f, 0.75f), 0.0f);

    //
    
    axiom::random32 rand(0xFFF);

    mat3 ori = random_orientation(rand);

    uint prev = 0.0f;
    uint prev_e = 0;

    for(int i = 0; i < 10; ++i) {
        uint e = create_capsule(vec3(0.0f, 0.0f, 12.0f) + ori * vec3(0.0f, 0.0f, 1.125f * i), ori, vec2(0.25f, 1.0f), ivec2(12, 6), axiom::hsv_color(rand() * 0.125f + 2.25f, 0.65f, 1.0f), 2.0f);

        if(i != 0) {
            axiom::position_constraint pc;
            pc.vs = {vec3(1.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f), vec3(0.0f, 0.0f, 1.0f)};
            pc.a = prev_e;
            pc.b = e;

            pc.va = vec3(0.0f, 0.0f, 0.5625f);
            pc.vb = vec3(0.0f, 0.0f, -0.5625f);

            physics_system.constraints.push_back(std::make_unique<axiom::position_constraint>(pc));
        }

        prev_e = e;
    }

    {
        mat3 main_ori = random_orientation(rand);

        ivec3 array = ivec3(8, 8, 8);
        vec3 origin = vec3(0.0f, 0.0f, 4.5f);
        float size = axiom::sqrt3 * 0.25f;
        float sep = size;

        for(int x = 0; x < array.x; ++x) {
            for(int y = 0; y < array.y; ++y) {
                for(int z = 0; z < array.z; ++z) {
                    vec3 pos = vec3(x, y, z) + 0.5f - vec3(array) * 0.5f;
                    pos = main_ori * pos;

                    pos *= sep;
                    pos += origin;

                    uint n = rand.next();
                    if(n % 16 < 10) {
                        vec3 color = axiom::hsv_color(rand() * 0.125f + 5.875f, 0.8f, 1.0f);

                        create_cube(pos, main_ori, size, color);
                    } else if(n % 16 < 12) {
                        vec3 color = axiom::hsv_color(rand() * 0.125f + 0.875f, 0.8f, 1.0f);

                        create_tetrahedron(pos, main_ori, size, color);
                    } else if(n % 16 < 14) {
                        vec3 color = axiom::hsv_color(rand() * 0.125f + 3.875f, 0.8f, 1.0f);
                        uint n = rand.next() % 5 + 3;

                        create_bipyramid(pos, main_ori, size, n, color);
                    } else if(n % 16 < 15) {
                        vec3 color = axiom::hsv_color(rand() * 0.125f + 0.3f, 0.8f, 1.0f);

                        create_dodecahedron(pos, main_ori, size, color);
                    } else if(n % 16 < 16) {
                        vec3 color = axiom::hsv_color(rand() * 0.125f + 3.0f, 0.8f, 1.0f);

                        create_icosahedron(pos, main_ori, size, color);
                    }
                }
            }
        }
    }

    //

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

    // create objects
    world_params params {
        .radii = vec3(100000.0f),
        .position = vec3(-300000.0f, -80000.0f, 100000.0f),
        .num_chunks = 8,
        .num_tiles = 16
    };
    make_world(params);

    // create target callback and pass in camera

    static bool do_render_grid = false;

    auto target_callback = [&window, camera_entity](axiom::render_target& target) {
        target.framebuffer.bind();
        target.framebuffer.clear(vec4(0.0f, 0.0f, 0.0f, 1.0f));

        axiom::camera3d& camera = axiom::ecs.get_component<axiom::camera3d>(camera_entity);
        camera.aspect = target.size;

        base_render(camera_entity, target.framebuffer);

        target.shadow->call();

        //render_skybox(camera_entity, target.framebuffer);
        if(do_render_grid) render_grid(camera_entity, target.framebuffer);
    };

    auto widget_callback = [camera_entity, &window](axiom::render_widget *self) {
        static bool movement_capture = false;
        static bool raycast_capture = false;
        static float movement_speed = 8.0f;

        static uint constraint_index = 0xFFFFFFFF;
        static float constraint_dist = 0.0f;
        
        axiom::transform3d& camera_transform = axiom::ecs.get_component<axiom::transform3d>(camera_entity);
        axiom::ui_system& ui_system = axiom::ecs.get_system<axiom::ui_system>();
        auto& psystem = axiom::ecs.get_system<axiom::physics_system3d>();

        if(ui_system.window->pressed_buttons.contains(axiom::input_code::KEY_F5)) psystem.sim_active = !psystem.sim_active;

        if(ui_system.click_capture == self->self) {
            if(ui_system.window->input_map[axiom::input_code::KEY_LEFT_CTRL]) {
                if(raycast_capture == false) {
                    raycast_capture = true;
                    
                    axiom::transform3d& camera_transform = axiom::ecs.get_component<axiom::transform3d>(camera_entity);
                    axiom::camera3d& camera_cam = axiom::ecs.get_component<axiom::camera3d>(camera_entity);

                    //

                    vec2 screen_pos = (ui_system.window->cursor_pos - self->position) / self->size;
                    screen_pos = screen_pos * 2.0f - 1.0f;

                    vec4 vertex = vec4(screen_pos, 0.5f, 1.0f);

                    mat4 proj = axiom::get_proj(camera_cam);
                    mat4 inv_proj = glm::inverse(proj);

                    vertex = inv_proj * vertex;
                    vertex /= vertex.w;

                    vec3 dir = glm::normalize(camera_transform.orientation * vertex.xyz());

                    //

                    psystem.gravity = [](vec3 pos) {
                        return vec3(0.0f, 0.0f, 1.0f) * -9.81f;
                    };

                    uint hit;
                    uint shape_hit;
                    vec3 normal;
                    vec3 point;
                    std::unordered_set<uint> mask;

                    psystem.raycast(camera_transform.position, dir, 1.0f, 20.0f, 0.0f, mask, &hit, &shape_hit, &normal, &point);

                    axiom::param_collider = hit;
                    //render_points_shape = hit;

                    if(hit != axiom::NULL_ENTITY) {
                        axiom::position_constraint pc;
                        pc.vs = {vec3(1.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f), vec3(0.0f, 0.0f, 1.0f)};
                        pc.a = hit;

                        axiom::transform3d& target_transform = axiom::ecs.get_component<axiom::transform3d>(hit);
                        pc.va = transpose(target_transform.orientation) * (point - target_transform.position);
                        pc.vb = point;
                        //pc.max_impulse = axiom::ecs.get_component<axiom::collider3d>(hit).mass * 30.0f * psystem.physics_step * 2.0f;

                        constraint_dist = length(point - camera_transform.position);

                        constraint_index = psystem.constraints.size();
                        psystem.constraints.push_back(std::make_unique<axiom::position_constraint>(pc));
                    }
                }
            } else {
                if(movement_capture == false && raycast_capture == false) {
                    ui_system.window->disable_cursor();
                    movement_capture = true;
                }
            }
        } else {
            if(raycast_capture && constraint_index != 0xFFFFFFFF) {
                auto& psystem = axiom::ecs.get_system<axiom::physics_system3d>();
                psystem.constraints.erase(psystem.constraints.begin() + constraint_index);

                constraint_index = 0xFFFFFFFF;
            }

            if(movement_capture) ui_system.window->show_cursor();
            movement_capture = false;
            raycast_capture = false;
        }

        if(ui_system.hover_capture == self->self) {
            if(ui_system.window->scroll_delta != 0.0f) {
                movement_speed *= pow(2, ui_system.window->scroll_delta * 0.5f);
            }
        }

        if(ui_system.click_capture == self->self) {
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
            } else if(raycast_capture && constraint_index != 0xFFFFFFFF) {
                uint camera = camera_entity;
                axiom::transform3d& camera_transform = axiom::ecs.get_component<axiom::transform3d>(camera);
                axiom::camera3d& camera_cam = axiom::ecs.get_component<axiom::camera3d>(camera);

                //

                vec2 screen_pos = (ui_system.window->cursor_pos - self->position) / self->size;
                screen_pos = screen_pos * 2.0f - 1.0f;

                vec4 vertex = vec4(screen_pos, 0.5f, 1.0f);

                mat4 proj = axiom::get_proj(camera_cam);
                mat4 inv_proj = glm::inverse(proj);

                vertex = inv_proj * vertex;
                vertex /= vertex.w;

                vec3 dir = glm::normalize(camera_transform.orientation * vertex.xyz());

                vec3 new_point = camera_transform.position + dir * constraint_dist;
                
                //

                auto& psystem = axiom::ecs.get_system<axiom::physics_system3d>();

                axiom::position_constraint* c = dynamic_cast<axiom::position_constraint*>(psystem.constraints[constraint_index].get());
                c->vb = new_point;
            }
        }
    };

    //

    std::vector<axiom::texture_format> format;
    std::vector<axiom::texture_attachment> attachment;

    format = {axiom::texture_format::RGBA8, axiom::texture_format::RGBA8, axiom::texture_format::RGBA8, axiom::texture_format::DEPTHF};
    attachment = {axiom::texture_attachment::COLOR0, axiom::texture_attachment::COLOR1, axiom::texture_attachment::COLOR2, axiom::texture_attachment::DEPTH};
    axiom::render_target* rt = axiom::render_target::create(
        target_callback,
        ivec2(512),
        format,
        attachment, {}
    );
    
    axiom::shadow_renderer::create(5, 8.0f, 1.0f / 16.0f, 2048, camera_entity, rt, shadow_func);
    
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
                axiom::transform3d& camera_transform = axiom::ecs.get_component<axiom::transform3d>(camera_entity);

                return "position: " + axiom::to_base(camera_transform.position.x, 10, 3) + " " + axiom::to_base(camera_transform.position.y, 10, 3) + " " + axiom::to_base(camera_transform.position.z, 10, 3);
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

                auto* physics = &axiom::ecs.get_system<axiom::physics_system3d>();
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

        ui_system.input_step();

        axiom::row_widget::insert();

        axiom::button_widget::insert(vec2(32.0f), axiom::color_red, vec4(48, 116, 12, 12), 
            [](axiom::button_widget& self) {
                if(self.pressed) {
                    do_render_grid = !do_render_grid;
                    if(do_render_grid) self.color = axiom::color_red;
                    else self.color = axiom::color_red * 0.75f;
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
                                    std::string lipsum = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Sed pulvinar sit amet urna eget commodo. Morbi pulvinar ac mauris ut tempus. Mauris aliquet ultrices nulla. In feugiat rutrum pulvinar. Nulla dictum nisl nec auctor venenatis. Etiam vel pretium metus, quis auctor tortor. Quisque quam metus, scelerisque id sagittis ac, iaculis vitae leo. Sed dapibus purus dolor, et vestibulum velit sagittis et. Aliquam vel gravida lectus, eget viverra velit. Cras bibendum, risus in bibendum volutpat, tortor dui cursus augue, quis vulputate ante nibh a ante. Nam varius arcu ac felis vestibulum suscipit. Aliquam mauris justo, placerat sit amet tincidunt eu, auctor eu nunc. Donec suscipit arcu et risus sagittis porttitor. Praesent tellus mauris, semper quis dictum sit amet, tristique in nisl. Aenean nec metus feugiat neque porttitor vulputate vel vitae sem.\nQuisque vulputate imperdiet magna ac porttitor. Vivamus eget neque sed purus tempor placerat pharetra vitae felis. Class aptent taciti sociosqu ad litora torquent per conubia nostra, per inceptos himenaeos. Nullam dui justo, tempus ut neque sed, finibus vestibulum eros. Morbi egestas risus non justo rhoncus blandit. Nulla facilisis a lacus vitae rutrum. Praesent facilisis ligula et lacus semper tincidunt. Mauris at urna justo. Vivamus ornare molestie turpis vulputate auctor.\nSuspendisse pellentesque, urna consectetur suscipit dapibus, leo felis scelerisque sapien, nec elementum risus risus ac ipsum. Cras interdum massa neque. In lacinia volutpat ex at pretium. Pellentesque eleifend eu elit eu fermentum. Ut eros erat, viverra vel feugiat vitae, pulvinar id dui. Sed mattis lorem ac sapien eleifend, vitae finibus turpis suscipit. Mauris viverra nunc non eros efficitur, non efficitur odio porttitor. Aenean sed eros vitae tortor hendrerit pretium at a tellus. Nulla vel accumsan justo. Etiam dignissim ac justo nec pharetra. Pellentesque habitant morbi tristique senectus et netus et malesuada fames ac turpis egestas. Aliquam maximus aliquam tempus. Aenean et dui ullamcorper, consectetur arcu non, condimentum est. Vivamus in neque sit amet dolor feugiat sollicitudin at quis felis. In hac habitasse platea dictumst.";

                                    ui_system.input_reset();
                                    ui_system.position(axiom::position_mode::TOP_LEFT);
                                    axiom::window_widget::insert("Axiom", ivec2(200, 200), ivec2(400, 100), axiom::color_red);
                                    axiom::panel_widget::insert();
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