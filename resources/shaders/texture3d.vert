#version 460 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 tex;
layout(location = 2) in vec4 color;
layout(location = 3) in vec3 normal;

layout(location = 0) uniform mat4 model;
layout(location = 1) uniform mat4 view;
layout(location = 2) uniform mat4 proj;

layout(location = 0) out vec4 o_color;
layout(location = 1) out vec2 o_texture;
layout(location = 2) out vec3 o_normal;

void main() {
    gl_Position = view * model * vec4(position, 1.0);

    gl_Position = proj * gl_Position;

    o_color = color;
    o_texture = tex;
    o_normal = mat3(model) * normal;
}