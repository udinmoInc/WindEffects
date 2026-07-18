#pragma once

#include <cstdint>
#include "Core/Math/Types.h"

namespace we::runtime::world::environment {

class EnvironmentDirectionalLight {
public:
    std::uint64_t EntityId = 0;

    float Intensity = 10.0f;
    int TemperatureKelvin = 6500;
    bool CastDynamicShadows = true;
    bool AtmosphereSun = true;
    we::math::Vec3 Rotation{ -45.0f, 35.0f, 0.0f };
    we::math::Vec3 Color{ 1.0f, 0.96f, 0.86f };

    void ApplyDefaults();
    we::math::Vec3 GetLightDirection() const;
    we::math::Vec3 GetColorFromTemperature() const;
    void SyncFromEntityTransform(const we::math::Vec3& position, const we::math::Vec3& rotation, const we::math::Vec4& color);
    void ApplyToEntity(we::math::Vec3& position, we::math::Vec3& rotation, we::math::Vec4& color) const;
};

} // namespace we::runtime::world::environment
