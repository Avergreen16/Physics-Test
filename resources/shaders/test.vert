#version 460 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec2 tex_coord;

layout(location = 0) uniform mat4 model_matrix;

layout(location = 0) out vec3 ocolor;
layout(location = 1) out vec2 otex_coord;

void main() {
    gl_Position = model_matrix * vec4(position, 1.0);

    ocolor = color;
    otex_coord = tex_coord;
}