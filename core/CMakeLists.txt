find_package(Eigen3 CONFIG REQUIRED)

add_library(core
    src/math/Vec3.cpp
    src/types/PointCloud.cpp
 "include/core/math/Transforms.h")

target_link_libraries(core PUBLIC Eigen3::Eigen)
target_include_directories(core PUBLIC include)