#include "Vec3.h"

#include <cmath>     // std::sqrt
#include <ostream>   // the full std::ostream, needed to actually write to it

Vec3 operator+(const Vec3& a, const Vec3& b) {
    return Vec3{ a.x + b.x, a.y + b.y, a.z + b.z };
}

Vec3 operator/(const Vec3& v, float s) {
    return Vec3{ v.x / s, v.y / s, v.z / s };
}

Vec3& operator+=(Vec3& a, const Vec3& b) {
    a.x += b.x; a.y += b.y; a.z += b.z;
    return a;
}

float dot(const Vec3& a, const Vec3& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

float length(const Vec3& v) {
    return std::sqrt(dot(v, v));
}

Vec3 cross(const Vec3& a, const Vec3& b) {
    return Vec3{ a.y * b.z - a.z * b.y,
                a.z * b.x - a.x * b.z,
                a.x * b.y - a.y * b.x };
}

std::ostream& operator<<(std::ostream& os, const Vec3& v) {
    os << "(" << v.x << ", " << v.y << ", " << v.z << ")";
    return os;
}