add_library(core
    src/math/Vec3.cpp
    src/types/PointCloud.cpp
)

target_include_directories(core PUBLIC include)