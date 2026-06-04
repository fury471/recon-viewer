add_library(core
    src/math/Vec3.cpp
    src/types/PointCloud.cpp
 "include/core/types/Mesh.h" "include/core/interfaces/ISurfaceReconstructor.h")

target_include_directories(core PUBLIC include)