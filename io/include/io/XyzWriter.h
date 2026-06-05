#pragma once

#include <ostream>
#include "io/IWriter.h"

void writeXyz(std::ostream& out, const PointCloud& cloud);

class XyzWriter : public IWriter {
public:
    void write(const std::string& path, const PointCloud& cloud) override;
};