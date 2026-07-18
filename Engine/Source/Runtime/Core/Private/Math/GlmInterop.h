#pragma once

// Private glm interop — never include from Public headers.

#include "Core/Math/Types.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace we::math {

inline glm::vec2 ToGlm(const Vec2& v) { return {v.x, v.y}; }
inline glm::vec3 ToGlm(const Vec3& v) { return {v.x, v.y, v.z}; }
inline glm::vec4 ToGlm(const Vec4& v) { return {v.x, v.y, v.z, v.w}; }
inline glm::quat ToGlm(const Quat& q) { return {q.w, q.x, q.y, q.z}; }

inline Vec2 FromGlm(const glm::vec2& v) { return {v.x, v.y}; }
inline Vec3 FromGlm(const glm::vec3& v) { return {v.x, v.y, v.z}; }
inline Vec4 FromGlm(const glm::vec4& v) { return {v.x, v.y, v.z, v.w}; }
inline Quat FromGlm(const glm::quat& q) { return {q.x, q.y, q.z, q.w}; }

inline Mat4 FromGlm(const glm::mat4& m) {
    Mat4 out;
    const float* src = &m[0][0];
    for (int i = 0; i < 16; ++i) {
        out.m[i] = src[i];
    }
    return out;
}

inline glm::mat4 ToGlm(const Mat4& m) {
    glm::mat4 out(1.0f);
    float* dst = &out[0][0];
    for (int i = 0; i < 16; ++i) {
        dst[i] = m.m[i];
    }
    return out;
}

// Layout-compatible views for Private translation units that still call glm APIs.
inline glm::vec2& AsGlm(Vec2& v) { return reinterpret_cast<glm::vec2&>(v); }
inline const glm::vec2& AsGlm(const Vec2& v) { return reinterpret_cast<const glm::vec2&>(v); }
inline glm::vec3& AsGlm(Vec3& v) { return reinterpret_cast<glm::vec3&>(v); }
inline const glm::vec3& AsGlm(const Vec3& v) { return reinterpret_cast<const glm::vec3&>(v); }
inline glm::vec4& AsGlm(Vec4& v) { return reinterpret_cast<glm::vec4&>(v); }
inline const glm::vec4& AsGlm(const Vec4& v) { return reinterpret_cast<const glm::vec4&>(v); }
inline glm::mat4& AsGlm(Mat4& m) { return reinterpret_cast<glm::mat4&>(m); }
inline const glm::mat4& AsGlm(const Mat4& m) { return reinterpret_cast<const glm::mat4&>(m); }

} // namespace we::math
