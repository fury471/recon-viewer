#include "io/XyzReader.h"

#include <fstream>
#include <spdlog/spdlog.h>

PointCloud readXyz(std::istream& in) {
    PointCloud cloud;
    float x, y, z;
    while (in >> x >> y >> z) {
        cloud.positions.push_back(Vec3{ x, y, z });
    }
    return cloud;
}

PointCloud XyzReader::read(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        spdlog::error("XyzReader: could not open file '{}'", path);
        return {};   // an empty cloud — a deliberate, recognizable failure result
    }
    return readXyz(file);
}