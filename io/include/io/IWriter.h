#pragma once

#include <string>
#include "core/types/PointCloud.h"

class IWriter {
public:
    virtual ~IWriter() = default;
    virtual void write(const std::string& path, const PointCloud& cloud) = 0;
};