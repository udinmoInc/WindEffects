#pragma once

#include <cstdint>
#include <glm/glm.hpp>

namespace we::runtime::world::environment {

class EnvironmentExposureController {
public:
    static constexpr int kDefaultsVersion = 1;

    std::uint64_t EntityId = 0;

    float ExposureEV = 0.0f;
    float ExposureCompensation = 0.0f;
    float MinEV = -2.0f;
    float MaxEV = 14.0f;
    bool AutoExposure = true;
    int AppliedDefaultsVersion = 0;

    void ApplyDefaults();
    bool NeedsDefaultMigration() const { return AppliedDefaultsVersion < kDefaultsVersion; }
    float GetEffectiveExposureEV(float sunDerivedEV) const;
};

} // namespace we::runtime::world::environment
