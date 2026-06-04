#pragma once

#include <vector>
#include <cstdint>
#include "core/math/Vec3.h"

struct Mesh {
    std::vector<Vec3> vertices;
    std::vector<std::uint32_t> indices;   // every 3 indices form one triangle
    std::vector<Vec3> normals;            // optional, per-vertex
};