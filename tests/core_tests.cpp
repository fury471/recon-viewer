#include <catch2/catch_test_macros.hpp>
#include "core/math/Vec3.h"

TEST_CASE("dot product of perpendicular vectors is zero") {
    Vec3 a{ 1.0f, 0.0f, 0.0f };
    Vec3 b{ 0.0f, 1.0f, 0.0f };
    REQUIRE(dot(a, b) == 0.0f);
}