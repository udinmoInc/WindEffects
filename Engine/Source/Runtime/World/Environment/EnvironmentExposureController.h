#pragma once

#include <cstdint>
#include <glm/glm.hpp>

namespace we::runtime::world::environment {

class EnvironmentExposureController {
public:
    std::uint64_t EntityId = 0;

    float ExposureEV = 2.35f;
    float ExposureCompensation = 0.0f;
    float MinEV = -4.0f;
    float MaxEV = 8.0f;
    bool AutoExposure = true;

    void ApplyDefaults();
    float GetEffectiveExposureEV(float sunDerivedEV) const;
};

} // namespace we::runtime::world::environment
