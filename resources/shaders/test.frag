#version 460 core

layout(binding = 0) uniform sampler2D test_texture;

layout(location = 0) in vec3 color;
layout(location = 1) in vec2 tex_coord;

layout(location = 0) out vec4 frag_color;

void main() {
    frag_color = vec4(color * texture(test_texture, tex_coord).xyz, 1.0);
}