#version 460 core

layout(binding = 0) uniform sampler2D texture_sampler;

layout(location = 3) uniform vec3 light;

layout(location = 0) out vec4 frag_color;

layout(location = 0) in vec4 color;
layout(location = 1) in vec2 tex;
layout(location = 2) in vec3 normal;

void main() {
    vec4 tex_col = texture(texture_sampler, tex / vec2(textureSize(texture_sampler, 0)));

    float l = clamp(dot(normal, light), 0.0, 1.0);
    l = l * 0.75 + 0.25;
    if(length(normal) == 0.0) l = 1.0;

    frag_color = vec4(tex_col.xyz * color.xyz * l, tex_col.w * color.w);

    if(frag_color.w == 0.0) discard;
}