#pragma once

#include "World/Export.h"

#include <cstdint>
#include "Core/Math/Types.h"

namespace we::runtime::world::environment {

class WORLD_API EnvironmentSkyLight {
public:
    std::uint64_t EntityId = 0;

    float Intensity = 1.0f;
    bool RealTimeCapture = true;
    we::math::Vec3 LowerHemisphereColor{ 0.05f, 0.05f, 0.06f };
    we::math::Vec3 UpperHemisphereColor{ 0.05f, 0.08f, 0.12f };

    void ApplyDefaults();
    we::math::Vec3 GetAmbientColor() const;
    void SyncFromEntity(const we::math::Vec4& color);
    void ApplyToEntity(we::math::Vec4& color) const;
};

} // namespace we::runtime::world::environment
