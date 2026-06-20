#pragma once

#include "core/math/Transforms.h"

#include <Eigen/Dense>
#include <algorithm>
#include <cmath>

namespace render {

    // A camera that orbits a target point on the surface of a sphere,
    // controlled by a distance and two angles (yaw = around, pitch = up/down).
    class OrbitCamera {
    public:
        // Spin around the target (radians) — for mouse drag next step.
        void orbit(float dYaw, float dPitch) {
            yaw_ += dYaw;
            pitch_ += dPitch;
            const float limit = 1.55f;   // ~88.8 deg: stop short of the poles
            pitch_ = std::clamp(pitch_, -limit, limit);
        }

        // Move closer (factor < 1) or farther (factor > 1).
        void zoom(float factor) {
            distance_ = std::max(0.1f, distance_ * factor);
        }

        // Where the camera sits, from its angles and distance (spherical -> xyz).
        Eigen::Vector3f eye() const {
            const float cp = std::cos(pitch_);
            return target_ + distance_ * Eigen::Vector3f(cp * std::sin(yaw_),
                std::sin(pitch_),
                cp * std::cos(yaw_));
        }

        // The view matrix for the current pose.
        Eigen::Matrix4f viewMatrix() const {
            return core::lookAt(eye(), target_, Eigen::Vector3f(0.0f, 1.0f, 0.0f));
        }

    private:
        Eigen::Vector3f target_ = Eigen::Vector3f::Zero();
        float           distance_ = 3.0f;
        float           yaw_ = 0.0f;   // around the vertical axis
        float           pitch_ = 0.0f;   // up/down
    };

}  // namespace render