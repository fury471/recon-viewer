#pragma once

#include <istream>
#include "io/IReader.h"

// The parsing core, working on any input stream — easy to test.
PointCloud readXyz(std::istream& in);

class XyzReader : public IReader {
public:
    PointCloud read(const std::string& path) override;
};