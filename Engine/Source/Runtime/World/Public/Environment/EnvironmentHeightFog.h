#pragma once

#include "World/Export.h"

#include <cstdint>
#include "Core/Math/Types.h"

namespace we::runtime::world::environment {

class WORLD_API EnvironmentHeightFog {
public:
    std::uint64_t EntityId = 0;

    float Density = 0.02f;
    float HeightFalloff = 0.2f;
    float StartDistance = 0.0f;
    bool VolumetricFog = true;
    we::math::Vec3 FogColor{ 0.72f, 0.78f, 0.85f };

    void ApplyDefaults();
    void SyncFromEntity(const we::math::Vec4& color, const we::math::Vec3& scale);
    void ApplyToEntity(we::math::Vec4& color, we::math::Vec3& scale) const;
};

} // namespace we::runtime::world::environment
