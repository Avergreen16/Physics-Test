#pragma once

#define GLM_FORCE_SWIZZLE
#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/glm.hpp"
#include "glm/gtx/matrix_transform_2d.hpp"
#include "glm/gtx/transform.hpp"
#include "glm/gtx/quaternion.hpp"
#include "glm/gtx/orthonormalize.hpp"

#include <include/render.hpp>

using ivec2 = glm::ivec2;
using ivec3 = glm::ivec3;
using ivec4 = glm::ivec4;

using vec2 = glm::vec2;
using vec3 = glm::vec3;
using vec4 = glm::vec4;

using uvec2 = glm::uvec2;
using uvec3 = glm::uvec3;
using uvec4 = glm::uvec4;

using mat2 = glm::mat2;
using mat3 = glm::mat3;
using mat4 = glm::mat4;

using uint = unsigned int;
using ulong = uint64_t;

#include <vector>

struct terrain_pixel {
    float elevation = 0.0f;
};

std::vector<mat3> get_orientations();

struct terrain_generator {
    int size = 256;

    std::vector<terrain_pixel> terrain_pixels;
    std::vector<mat3> orientations;
    std::vector<mat2> rotations;
    std::vector<uint> edges;

    int get_index(ivec3 pos);

    void init();
    axiom::texture_asset render();
};