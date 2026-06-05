#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <sstream>

#include "core/math/Vec3.h"
#include "core/types/PointCloud.h"
#include "core/types/Mesh.h"
#include "core/interfaces/ISurfaceReconstructor.h"
#include "io/XyzReader.h"

using Catch::Approx;

TEST_CASE("Vec3 dot product") {
    SECTION("known value") {
        Vec3 a{ 1.0f, 2.0f, 3.0f };
        Vec3 b{ 4.0f, 5.0f, 6.0f };
        REQUIRE(dot(a, b) == Approx(32.0f));   // 4 + 10 + 18
    }
    SECTION("perpendicular vectors give zero") {
        Vec3 x{ 1.0f, 0.0f, 0.0f };
        Vec3 y{ 0.0f, 1.0f, 0.0f };
        REQUIRE(dot(x, y) == Approx(0.0f));
    }
}

TEST_CASE("Vec3 length is Pythagoras in 3D") {
    Vec3 v{ 3.0f, 4.0f, 0.0f };
    REQUIRE(length(v) == Approx(5.0f));        // classic 3-4-5 triangle
}

TEST_CASE("Vec3 cross product is perpendicular to both inputs") {
    Vec3 a{ 1.0f, 2.0f, 3.0f };
    Vec3 b{ 4.0f, 5.0f, 6.0f };
    Vec3 n = cross(a, b);
    REQUIRE(dot(n, a) == Approx(0.0f));
    REQUIRE(dot(n, b) == Approx(0.0f));
}

TEST_CASE("Vec3 arithmetic operators") {
    Vec3 a{ 1.0f, 2.0f, 3.0f };
    Vec3 b{ 4.0f, 5.0f, 6.0f };

    SECTION("addition adds component-wise") {
        Vec3 s = a + b;
        REQUIRE(s.x == Approx(5.0f));
        REQUIRE(s.y == Approx(7.0f));
        REQUIRE(s.z == Approx(9.0f));
    }
    SECTION("scalar division divides each component") {
        Vec3 h = b / 2.0f;
        REQUIRE(h.x == Approx(2.0f));
        REQUIRE(h.y == Approx(2.5f));
        REQUIRE(h.z == Approx(3.0f));
    }
}

TEST_CASE("PointCloud centroid is the average position") {
    PointCloud cloud;
    cloud.positions.push_back(Vec3{ 0.0f, 0.0f, 0.0f });
    cloud.positions.push_back(Vec3{ 3.0f, 0.0f, 0.0f });
    cloud.positions.push_back(Vec3{ 0.0f, 3.0f, 0.0f });

    Vec3 c = centroid(cloud);
    REQUIRE(c.x == Approx(1.0f));
    REQUIRE(c.y == Approx(1.0f));
    REQUIRE(c.z == Approx(0.0f));
}

TEST_CASE("PointCloud bounding box") {
    SECTION("encloses all points") {
        PointCloud cloud;
        cloud.positions.push_back(Vec3{ -1.0f, 2.0f,  5.0f });
        cloud.positions.push_back(Vec3{ 4.0f, 0.0f, -3.0f });

        AABB box = boundingBox(cloud);
        REQUIRE(box.min.x == Approx(-1.0f));
        REQUIRE(box.min.y == Approx(0.0f));
        REQUIRE(box.min.z == Approx(-3.0f));
        REQUIRE(box.max.x == Approx(4.0f));
        REQUIRE(box.max.y == Approx(2.0f));
        REQUIRE(box.max.z == Approx(5.0f));
    }

    SECTION("an empty cloud yields an inverted box") {
        PointCloud empty;
        AABB box = boundingBox(empty);
        // min > max is the deliberate "no points" signal from Tutorial 1.
        REQUIRE(box.min.x > box.max.x);
    }

}

TEST_CASE("length of a unit-ish vector needs tolerance") {
    Vec3 v{ 1.0f, 1.0f, 1.0f };
    REQUIRE(length(v) == Approx(1.7320508f));   // sqrt(3)
}

TEST_CASE("squaring a length should recover the squared norm") {
    Vec3 v{ 1.0f, 1.0f, 0.0f };
    REQUIRE(length(v) * length(v) == Approx(dot(v, v)));
}

// A placeholder implementation: just turns the cloud's points into vertices.
class StubSurface : public ISurfaceReconstructor {
public:
    Mesh reconstruct(const PointCloud& cloud) override {
        Mesh mesh;
        mesh.vertices = cloud.positions;   // no real surfacing yet — that's Phase 3
        return mesh;
    }
};

class CentroidSurface : public ISurfaceReconstructor {
public:
    Mesh reconstruct(const PointCloud& cloud) override {
        Mesh mesh;
        mesh.vertices.push_back(centroid(cloud));   // a single vertex: the cloud's center
        return mesh;
    }
};

Mesh runStage(ISurfaceReconstructor& stage, const PointCloud& cloud) {
    return stage.reconstruct(cloud);
}
// runStage(stub, cloud)  → 2 vertices;  runStage(centroidStage, cloud) → 1

TEST_CASE("a stage can be swapped at runtime through a pointer") {
    StubSurface stub;
	CentroidSurface centroidStage;

    PointCloud cloud;
    cloud.positions.push_back(Vec3{ 1.0f, 2.0f, 3.0f });
    cloud.positions.push_back(Vec3{ 4.0f, 5.0f, 6.0f });

    ISurfaceReconstructor* stage = &stub;   // pointer: holds the address of stub

    Mesh m = stage->reconstruct(cloud);     // dispatches to StubSurface → 2 vertices
    stage = &centroidStage;                 // re-point at the other implementation
    Mesh n = stage->reconstruct(cloud);     // dispatches to CentroidSurface → 1 vertex

    REQUIRE(m.vertices.size() == 2);
	REQUIRE(n.vertices.size() == 1);
}

TEST_CASE("a stage can be invoked polymorphically via a function") {
    StubSurface stub;
    CentroidSurface centroidStage;

    PointCloud cloud;
    cloud.positions.push_back(Vec3{ 1.0f, 2.0f, 3.0f });
    cloud.positions.push_back(Vec3{ 4.0f, 5.0f, 6.0f });

    Mesh m = runStage(stub, cloud);             // dispatches to StubSurface::reconstruct
    Mesh n = runStage(centroidStage, cloud);    // dispatches to CentroidSurface::reconstruct
    REQUIRE(m.vertices.size() == 2);
    REQUIRE(n.vertices.size() == 1);
}

TEST_CASE("readXyz parses whitespace-separated points") {
    std::istringstream in("0 0 0\n1 2 3\n4 5 6\n");
    PointCloud cloud = readXyz(in);

    REQUIRE(cloud.positions.size() == 3);
    REQUIRE(cloud.positions[1].x == Approx(1.0f));
    REQUIRE(cloud.positions[1].y == Approx(2.0f));
    REQUIRE(cloud.positions[1].z == Approx(3.0f));
}