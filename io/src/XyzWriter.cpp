#include "io/XyzWriter.h"

#include <fstream>
#include <iomanip>

void writeXyz(std::ostream& out, const PointCloud& cloud) {
    out << std::setprecision(9);   // enough digits to round-trip a float exactly
    for (const Vec3& p : cloud.positions) {
        out << p.x << ' ' << p.y << ' ' << p.z << '\n';
    }
}

void XyzWriter::write(const std::string& path, const PointCloud& cloud) {
    std::ofstream file(path);
    writeXyz(file, cloud);
}