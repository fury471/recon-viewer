#include "PointCloud.h"

#include <cmath>     // std::fmin, std::fmax
#include <limits>    // std::numeric_limits

Vec3 centroid(const PointCloud& cloud) {
    Vec3 sum{ 0.0f, 0.0f, 0.0f };
    for (const Vec3& p : cloud.positions) {
        sum += p;
    }
    float n = static_cast<float>(cloud.positions.size());
    return sum / n;
}

AABB boundingBox(const PointCloud& cloud) {
    AABB box;
    constexpr float inf = std::numeric_limits<float>::infinity();
    box.min = Vec3{ inf,  inf,  inf };
    box.max = Vec3{ -inf, -inf, -inf };
    for (const Vec3& p : cloud.positions) {
        box.min.x = std::fmin(box.min.x, p.x);
        box.min.y = std::fmin(box.min.y, p.y);
        box.min.z = std::fmin(box.min.z, p.z);
        box.max.x = std::fmax(box.max.x, p.x);
        box.max.y = std::fmax(box.max.y, p.y);
        box.max.z = std::fmax(box.max.z, p.z);
    }
    return box;
}