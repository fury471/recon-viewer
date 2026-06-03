#pragma once

#include <iosfwd>   // forward-declares std::ostream — keeps this header light

// The atom of recon-viewer's data model.
struct Vec3 {
    float x;
    float y;
    float z;
};

Vec3  operator+(const Vec3& a, const Vec3& b);
Vec3  operator/(const Vec3& v, float s);
Vec3& operator+=(Vec3& a, const Vec3& b);

float dot(const Vec3& a, const Vec3& b);
float length(const Vec3& v);
Vec3  cross(const Vec3& a, const Vec3& b);

std::ostream& operator<<(std::ostream& os, const Vec3& v);