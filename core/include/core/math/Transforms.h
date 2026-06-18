#pragma once

#include <Eigen/Dense>
#include <cmath>

namespace core {

    // View matrix: world space -> camera space (camera at origin, looking down -z).
    inline Eigen::Matrix4f lookAt(const Eigen::Vector3f& eye,
        const Eigen::Vector3f& target,
        const Eigen::Vector3f& up) {
        Eigen::Vector3f f = (target - eye).normalized();   // forward
        Eigen::Vector3f s = f.cross(up).normalized();      // right
        Eigen::Vector3f u = s.cross(f);                    // true up

        Eigen::Matrix4f m = Eigen::Matrix4f::Identity();
        m(0, 0) = s.x();  m(0, 1) = s.y();  m(0, 2) = s.z();   // right  -> row 0
        m(1, 0) = u.x();  m(1, 1) = u.y();  m(1, 2) = u.z();   // up     -> row 1
        m(2, 0) = -f.x();  m(2, 1) = -f.y();  m(2, 2) = -f.z();   // -fwd   -> row 2
        m(0, 3) = -s.dot(eye);                                    // translations
        m(1, 3) = -u.dot(eye);
        m(2, 3) = f.dot(eye);
        return m;
    }

    // Perspective: camera space -> clip space (Vulkan: y-down, depth 0..1).
    inline Eigen::Matrix4f perspective(float fovyRadians, float aspect,
        float zNear, float zFar) {
        const float cot = 1.0f / std::tan(fovyRadians / 2.0f);

        Eigen::Matrix4f m = Eigen::Matrix4f::Zero();
        m(0, 0) = cot / aspect;                       // x scale
        m(1, 1) = -cot;                               // y scale, negated for Vulkan
        m(2, 2) = zFar / (zNear - zFar);              // A
        m(2, 3) = (zFar * zNear) / (zNear - zFar);    // B
        m(3, 2) = -1.0f;                              // w = -z (the divide)
        return m;
    }

}  // namespace core