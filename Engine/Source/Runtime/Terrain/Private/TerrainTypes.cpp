#include "Terrain/TerrainTypes.h"

#include <algorithm>
#include <cmath>

namespace we::runtime::terrain {

bool TerrainAABB::ContainsPoint(const glm::vec3& p) const {
    return p.x >= min.x && p.x <= max.x
        && p.y >= min.y && p.y <= max.y
        && p.z >= min.z && p.z <= max.z;
}

bool TerrainAABB::IntersectsFrustumPlanes(const glm::vec4 planes[6]) const {
    const glm::vec3 center = (min + max) * 0.5f;
    const glm::vec3 extents = (max - min) * 0.5f;
    for (int i = 0; i < 6; ++i) {
        const glm::vec3 n(planes[i].x, planes[i].y, planes[i].z);
        const float r = extents.x * std::abs(n.x) + extents.y * std::abs(n.y) + extents.z * std::abs(n.z);
        const float d = glm::dot(n, center) + planes[i].w;
        if (d + r < 0.0f) {
            return false;
        }
    }
    return true;
}

TerrainFrustump TerrainFrustump::FromViewProj(const glm::mat4& viewProj) {
    TerrainFrustump f{};
    // Column-major glm mat4: extract frustum planes (Gribb/Hartmann).
    const glm::vec4 row0(viewProj[0][0], viewProj[1][0], viewProj[2][0], viewProj[3][0]);
    const glm::vec4 row1(viewProj[0][1], viewProj[1][1], viewProj[2][1], viewProj[3][1]);
    const glm::vec4 row2(viewProj[0][2], viewProj[1][2], viewProj[2][2], viewProj[3][2]);
    const glm::vec4 row3(viewProj[0][3], viewProj[1][3], viewProj[2][3], viewProj[3][3]);

    f.planes[0] = row3 + row0; // left
    f.planes[1] = row3 - row0; // right
    f.planes[2] = row3 + row1; // bottom
    f.planes[3] = row3 - row1; // top
    f.planes[4] = row3 + row2; // near
    f.planes[5] = row3 - row2; // far

    for (int i = 0; i < 6; ++i) {
        const float len = std::sqrt(f.planes[i].x * f.planes[i].x
            + f.planes[i].y * f.planes[i].y
            + f.planes[i].z * f.planes[i].z);
        if (len > 1e-6f) {
            f.planes[i] /= len;
        }
    }
    return f;
}

} // namespace we::runtime::terrain
