#pragma once

#include <string>
#include "core/types/PointCloud.h"

class IReader {
public:
    virtual ~IReader() = default;
    virtual PointCloud read(const std::string& path) = 0;
};