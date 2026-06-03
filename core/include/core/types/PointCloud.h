#pragma once

#include <vector>
#include "core/math/Vec3.h"

struct PointCloud {
    std::vector<Vec3> positions;
    std::vector<Vec3> normals;   // may stay empty until a stage estimates them
    std::vector<Vec3> colors;    // RGB, each channel in [0, 1]
};

struct AABB {
    Vec3 min;
    Vec3 max;
};

Vec3 centroid(const PointCloud& cloud);
AABB boundingBox(const PointCloud& cloud);