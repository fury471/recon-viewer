#include <iostream>
#include "core/types/PointCloud.h"   // brings in PointCloud, AABB, and (via it) Vec3

int main() {
    PointCloud cloud;
    cloud.positions.push_back(Vec3{ 0.0f, 0.0f, 0.0f });
    cloud.positions.push_back(Vec3{ 2.0f, 0.0f, 0.0f });
    cloud.positions.push_back(Vec3{ 0.0f, 6.0f, 0.0f });

    AABB box = boundingBox(cloud);

    std::cout << "the cloud has " << cloud.positions.size() << " points\n";
    std::cout << "the centroid is " << centroid(cloud) << "\n";
    std::cout << "the bounding box is [" << box.min << ", " << box.max << "]\n";
    return 0;
}