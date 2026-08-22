#version 460 core

layout(location = 0) in vec2 position;
layout(location = 1) in vec4 color;

layout(location = 0) uniform mat4 model;
layout(location = 1) uniform mat4 view;
layout(location = 2) uniform mat4 proj;

layout(location = 0) out vec4 o_color;

void main() {
    gl_Position = view * model * vec4(position, 0.5, 1.0);

    gl_Position = proj * gl_Position;

    o_color = color;
}