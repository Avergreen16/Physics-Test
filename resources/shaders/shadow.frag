#version 460 core

layout(binding = 0) uniform sampler2D viewport_normal_tex;
layout(binding = 1) uniform sampler2D viewport_shading_tex;
layout(binding = 2) uniform sampler2D viewport_depth_tex;
layout(binding = 3) uniform sampler2D shadow_normal_tex[5];
layout(binding = 8) uniform sampler2D shadow_depth_tex[5];

//
layout(location = 0) in vec2 coords;
layout(location = 1) in mat4 inv_viewport_model;
layout(location = 5) in mat4 inv_viewport_view;
layout(location = 9) in mat4 inv_viewport_proj;

layout(location = 3) uniform mat4 shadow_model[5];
layout(location = 8) uniform mat4 shadow_view[5];
layout(location = 13) uniform mat4 shadow_proj[5];

layout(location = 0) out vec4 frag_color;

vec3 hsv_color(float hue, float saturation, float value) {
    float x = mod(hue, 1.0) * 6.0;
    float frac = fract(x);

    if(x < 1.0) return vec3(1.0, frac, 0.0);
    if(x < 2.0) return vec3(1.0 - frac, 1.0, 0.0);
    if(x < 3.0) return vec3(0.0, 1.0, frac);
    if(x < 4.0) return vec3(0.0, 1.0 - frac, 1.0);
    if(x < 5.0) return vec3(frac, 0.0, 1.0);
    return vec3(1.0, 0.0, 1.0 - frac);
}

vec4 blend(vec4 dst, vec4 src) {
    return dst * (1.0 - src.w) + src * src.w;
}

mat4 get_transform(mat4 m) {
    return mat4(
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0,
        m[3][0], m[3][1], m[3][2], 1
    );
}

void main() {
    ivec2 ptexel = ivec2(gl_FragCoord.xy);

    float depth = texelFetch(viewport_depth_tex, ptexel, 0).r;
    vec3 normal = texelFetch(viewport_normal_tex, ptexel, 0).rgb * 2.0 - 1.0;
    vec3 shading = texelFetch(viewport_shading_tex, ptexel, 0).rgb * 2.0 - 1.0;

    vec4 pos = vec4(vec2(gl_FragCoord.xy) / vec2(textureSize(viewport_depth_tex, 0)) * 2.0 - 1.0, depth, 1.0);

    pos = inv_viewport_proj * pos;
    pos /= pos.w;
    
    pos = inv_viewport_view * pos;

    depth = pos.z;


    uint include = 0xFFFFFFFF;
    float sd = 0.0;
    vec3 light_dir;
    float max_depth = 3e34;

    light_dir = normalize(transpose(mat3(shadow_view[0] * shadow_proj[0])) * vec3(0.0, 0.0, 1.0));
    vec3 ppos = light_dir * -1e10;

    for(int i = 3; i >= 0; --i) {
        float texel_size = 1.0 / 16.0 * pow(8.0, i);
        vec4 spos = pos;

        mat4 smodel = shadow_model[i];// * inv_viewport_model);
        mat4 sview = shadow_view[i];
        mat4 sproj = shadow_proj[i];

        spos = sview * spos;

        float pdepth = spos.z;

        spos = sproj * spos;
        spos /= spos.w;

        if(include == 0xFFFFFFFF && spos.x < 1.0 && spos.x > -1.0 && spos.y < 1.0 && spos.y > -1.0 && spos.z >= 0.0 && spos.z < 1.0) {
            ivec2 stexel = ivec2(floor((spos.xy * 0.5 + 0.5) * vec2(textureSize(shadow_depth_tex[i], 0))));

            vec3 snormal = texelFetch(shadow_normal_tex[i], stexel, 0).rgb * 2.0 - 1.0;
            float sdepth = texelFetch(shadow_depth_tex[i], stexel, 0).r;

            spos = vec4(spos.xy, sdepth, 1.0);
            spos = inverse(sproj) * spos;
            spos = inverse(sview) * spos;

            sdepth = dot(vec3(spos), light_dir);
            pdepth = dot(vec3(pos), light_dir);

            vec4 pp = vec4(ppos, 1.0);
            pp = sview * pp;
            pp = sproj * pp;

            if(pp.x < 1.0 && pp.x > -1.0 && pp.y < 1.0 && pp.y > -1.0 && pp.z >= 0.0 && pp.z < 1.0 || dot(ppos, light_dir) < -1e9) {
                sd = sdepth;

                float bias = texel_size * 0.25;

                float cos_theta = clamp(dot(snormal, light_dir), 0.1, 1.0);
                float slope = sqrt(1.0 - cos_theta * cos_theta) / cos_theta;
                bias += slope * texel_size;

                if(sdepth - bias > pdepth) frag_color = vec4(0.0, 0.0, 0.0, 0.625);
                else frag_color = vec4(0.0, 0.0, 0.0, (1.0 - clamp(dot(light_dir, normal), 0.0, 1.0)) * 0.625); 
                
                max_depth = sdepth - bias;
            }

            if(sdepth > dot(ppos, light_dir)) {
                ppos = spos.xyz;
            }
        }
    }

    //
    
    //frag_color = vec4(normal, 0.75);
}