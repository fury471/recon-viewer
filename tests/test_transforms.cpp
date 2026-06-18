#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "core/math/Transforms.h"

using Catch::Approx;

TEST_CASE("lookAt puts the camera at the origin", "[transforms]") {
    Eigen::Vector3f eye(0.0f, 0.0f, 5.0f);
    Eigen::Vector3f target(0.0f, 0.0f, 0.0f);
    Eigen::Vector3f up(0.0f, 1.0f, 0.0f);

    Eigen::Matrix4f view = core::lookAt(eye, target, up);

    // The eye maps to the origin of camera space.
    Eigen::Vector4f eyeCam = view * Eigen::Vector4f(eye.x(), eye.y(), eye.z(), 1.0f);
    REQUIRE(eyeCam.x() == Approx(0.0f).margin(1e-5));
    REQUIRE(eyeCam.y() == Approx(0.0f).margin(1e-5));
    REQUIRE(eyeCam.z() == Approx(0.0f).margin(1e-5));

    // The world origin sits 5 units in front (negative z).
    Eigen::Vector4f originCam = view * Eigen::Vector4f(0.0f, 0.0f, 0.0f, 1.0f);
    REQUIRE(originCam.z() == Approx(-5.0f).margin(1e-5));
}

TEST_CASE("perspective has the expected Vulkan entries", "[transforms]") {
    Eigen::Matrix4f p = core::perspective(1.5707963f /*90 deg*/, 1.0f, 1.0f, 10.0f);

    REQUIRE(p(0, 0) == Approx(1.0f).margin(1e-4));    // cot/aspect, cot = 1
    REQUIRE(p(1, 1) == Approx(-1.0f).margin(1e-4));   // y flipped
    REQUIRE(p(3, 2) == Approx(-1.0f).margin(1e-4));   // w = -z
    REQUIRE(p(2, 2) == Approx(10.0f / (1.0f - 10.0f)).margin(1e-4));
}