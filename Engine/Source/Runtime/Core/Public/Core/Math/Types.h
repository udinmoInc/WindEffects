#pragma once

// Engine-owned math types. Public headers must use these instead of third-party math.
// Layout matches common GLSL/HLSL conventions (column-major mat4).

#include "Core/Export.h"

#include <cmath>
#include <cstddef>

namespace we::math {

struct Vec2 {
    float x = 0.0f;
    float y = 0.0f;

    constexpr Vec2() = default;
    constexpr explicit Vec2(float v) : x(v), y(v) {}
    constexpr Vec2(float x_, float y_) : x(x_), y(y_) {}

    float& operator[](std::size_t i) { return i == 0 ? x : y; }
    const float& operator[](std::size_t i) const { return i == 0 ? x : y; }
};

struct Vec3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    constexpr Vec3() = default;
    constexpr explicit Vec3(float v) : x(v), y(v), z(v) {}
    constexpr Vec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}

    float& operator[](std::size_t i) { return (&x)[i]; }
    const float& operator[](std::size_t i) const { return (&x)[i]; }
};

struct Vec4 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float w = 0.0f;

    constexpr Vec4() = default;
    constexpr explicit Vec4(float v) : x(v), y(v), z(v), w(v) {}
    constexpr Vec4(float x_, float y_, float z_, float w_) : x(x_), y(y_), z(z_), w(w_) {}
    constexpr Vec4(const Vec3& v, float w_) : x(v.x), y(v.y), z(v.z), w(w_) {}

    float& operator[](std::size_t i) { return (&x)[i]; }
    const float& operator[](std::size_t i) const { return (&x)[i]; }
};

struct Mat4 {
    float m[16] = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    };

    constexpr Mat4() = default;
    explicit Mat4(float diagonal) {
        for (float& v : m) v = 0.0f;
        m[0] = m[5] = m[10] = m[15] = diagonal;
    }

    float* data() { return m; }
    const float* data() const { return m; }
    float& operator[](std::size_t i) { return m[i]; }
    const float& operator[](std::size_t i) const { return m[i]; }
};

struct Quat {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float w = 1.0f;

    constexpr Quat() = default;
    constexpr Quat(float x_, float y_, float z_, float w_) : x(x_), y(y_), z(z_), w(w_) {}
};

inline constexpr Vec2 operator+(const Vec2& a, const Vec2& b) { return {a.x + b.x, a.y + b.y}; }
inline constexpr Vec2 operator-(const Vec2& a, const Vec2& b) { return {a.x - b.x, a.y - b.y}; }
inline constexpr Vec2 operator*(const Vec2& a, float s) { return {a.x * s, a.y * s}; }
inline constexpr Vec2 operator*(float s, const Vec2& a) { return a * s; }
inline constexpr bool operator==(const Vec2& a, const Vec2& b) { return a.x == b.x && a.y == b.y; }
inline constexpr bool operator!=(const Vec2& a, const Vec2& b) { return !(a == b); }

inline constexpr Vec3 operator+(const Vec3& a, const Vec3& b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
inline constexpr Vec3 operator-(const Vec3& a, const Vec3& b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
inline constexpr Vec3 operator-(const Vec3& a) { return {-a.x, -a.y, -a.z}; }
inline constexpr Vec3 operator*(const Vec3& a, float s) { return {a.x * s, a.y * s, a.z * s}; }
inline constexpr Vec3 operator*(float s, const Vec3& a) { return a * s; }
inline constexpr Vec3 operator*(const Vec3& a, const Vec3& b) { return {a.x * b.x, a.y * b.y, a.z * b.z}; }
inline constexpr Vec3 operator/(const Vec3& a, float s) { return {a.x / s, a.y / s, a.z / s}; }
inline constexpr bool operator==(const Vec3& a, const Vec3& b) {
    return a.x == b.x && a.y == b.y && a.z == b.z;
}
inline constexpr bool operator!=(const Vec3& a, const Vec3& b) { return !(a == b); }
inline Vec3& operator+=(Vec3& a, const Vec3& b) { a.x += b.x; a.y += b.y; a.z += b.z; return a; }
inline Vec3& operator-=(Vec3& a, const Vec3& b) { a.x -= b.x; a.y -= b.y; a.z -= b.z; return a; }

inline constexpr Vec4 operator+(const Vec4& a, const Vec4& b) { return {a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w}; }
inline constexpr Vec4 operator-(const Vec4& a, const Vec4& b) { return {a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w}; }
inline constexpr Vec4 operator*(const Vec4& a, float s) { return {a.x * s, a.y * s, a.z * s, a.w * s}; }
inline constexpr bool operator==(const Vec4& a, const Vec4& b) {
    return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w;
}
inline constexpr bool operator!=(const Vec4& a, const Vec4& b) { return !(a == b); }

inline Mat4 operator*(const Mat4& a, const Mat4& b) {
    Mat4 out(0.0f);
    for (int col = 0; col < 4; ++col) {
        for (int row = 0; row < 4; ++row) {
            float sum = 0.0f;
            for (int k = 0; k < 4; ++k) {
                sum += a.m[k * 4 + row] * b.m[col * 4 + k];
            }
            out.m[col * 4 + row] = sum;
        }
    }
    return out;
}

inline float Length(const Vec3& v) {
    return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

inline Vec3 Normalize(const Vec3& v) {
    const float len = Length(v);
    return len > 0.0f ? v / len : Vec3{};
}

inline constexpr float Dot(const Vec3& a, const Vec3& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline constexpr Vec3 Cross(const Vec3& a, const Vec3& b) {
    return {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

} // namespace we::math
