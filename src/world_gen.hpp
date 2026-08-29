#pragma once
#include <math.hpp>

struct crater_population {
    float min_size;
    float max_size;
    float distribution;
    int num_craters;
    int num_ejecta;
};

struct crater {
    vec3 position;
    float radius = 0.1f;
    float ejecta = 0.0f;
    float age = 0.0f;
    float height = 0.0f;
};

mat4 get_matrix(vec3 y, vec3 z, vec3 origin);

void create_planet(uint seed, vec3 position, mat3 orientation, vec3 dimensions, std::vector<crater_population> populations, std::vector<vec3> colors, float amplitude, float noise_freq, float noise_offset, float age_value, float ejecta_value, float blend_value, uint num_chunks, uint num_tiles);