#include "Terrain/TerrainTypes.h"

#include "Core/Math/GlmInterop.h"

#include <algorithm>
#include <cmath>

namespace we::runtime::terrain {

bool TerrainAABB::ContainsPoint(const we::math::Vec3& p) const {
    return p.x >= min.x && p.x <= max.x
        && p.y >= min.y && p.y <= max.y
        && p.z >= min.z && p.z <= max.z;
}

bool TerrainAABB::IntersectsFrustumPlanes(const we::math::Vec4 planes[6]) const {
    const we::math::Vec3 center = (min + max) * 0.5f;
    const we::math::Vec3 extents = (max - min) * 0.5f;
    for (int i = 0; i < 6; ++i) {
        const we::math::Vec3 n(planes[i].x, planes[i].y, planes[i].z);
        const float r = extents.x * std::abs(n.x) + extents.y * std::abs(n.y) + extents.z * std::abs(n.z);
        const float d = we::math::Dot(n, center) + planes[i].w;
        if (d + r < 0.0f) {
            return false;
        }
    }
    return true;
}

TerrainFrustump TerrainFrustump::FromViewProj(const we::math::Mat4& viewProj) {
    TerrainFrustump f{};
    const glm::mat4& vp = we::math::AsGlm(viewProj);
    const we::math::Vec4 row0(vp[0][0], vp[1][0], vp[2][0], vp[3][0]);
    const we::math::Vec4 row1(vp[0][1], vp[1][1], vp[2][1], vp[3][1]);
    const we::math::Vec4 row2(vp[0][2], vp[1][2], vp[2][2], vp[3][2]);
    const we::math::Vec4 row3(vp[0][3], vp[1][3], vp[2][3], vp[3][3]);

    f.planes[0] = row3 + row0;
    f.planes[1] = row3 - row0;
    f.planes[2] = row3 + row1;
    f.planes[3] = row3 - row1;
    f.planes[4] = row3 + row2;
    f.planes[5] = row3 - row2;

    for (int i = 0; i < 6; ++i) {
        const float len = std::sqrt(f.planes[i].x * f.planes[i].x
            + f.planes[i].y * f.planes[i].y
            + f.planes[i].z * f.planes[i].z);
        if (len > 1e-6f) {
            f.planes[i] = f.planes[i] * (1.0f / len);
        }
    }
    return f;
}

} // namespace we::runtime::terrain
