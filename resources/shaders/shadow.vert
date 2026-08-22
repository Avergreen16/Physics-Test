#version 460 core

layout(location = 0) in vec2 position;

layout(location = 0) uniform mat4 model;
layout(location = 1) uniform mat4 view;
layout(location = 2) uniform mat4 proj;

layout(location = 0) out vec2 coords;
layout(location = 1) flat out mat4 inv_model;
layout(location = 5) flat out mat4 inv_view;
layout(location = 9) flat out mat4 inv_proj;

void main() {
    gl_Position = vec4(position, 0.5, 1.0);

    inv_model = inverse(model);
    inv_view = inverse(view);
    inv_proj = inverse(proj);

    coords = position;
}