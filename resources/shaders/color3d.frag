#version 460 core

layout(location = 3) uniform vec3 light;

layout(location = 0) out vec4 frag_color;
layout(location = 1) out vec4 frag_normal;
layout(location = 2) out vec4 frag_shading;

layout(location = 0) in vec4 color;
layout(location = 1) in vec3 normal;

void main() {
    frag_color = color;
    frag_normal = vec4(normal * 0.5 + 0.5, 1.0);
    frag_shading = vec4(normal * 0.5 + 0.5, 1.0);
    
    if(frag_color.w == 0.0) discard;
}