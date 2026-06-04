#pragma once

#include "core/types/PointCloud.h"
#include "core/types/Mesh.h"

class ISurfaceReconstructor {
public:
    virtual ~ISurfaceReconstructor() = default;
    virtual Mesh reconstruct(const PointCloud& cloud) = 0;
};