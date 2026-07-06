#include "Environment/EnvironmentExposureController.h"

#include <algorithm>

namespace we::runtime::world::environment {

void EnvironmentExposureController::ApplyDefaults() {
    ExposureEV = 0.0f;
    ExposureCompensation = 0.0f;
    MinEV = -2.0f;
    MaxEV = 14.0f;
    AutoExposure = true;
}

float EnvironmentExposureController::GetEffectiveExposureEV(float sunDerivedEV) const {
    const float base = AutoExposure ? sunDerivedEV : ExposureEV;
    return std::clamp(base + ExposureCompensation, MinEV, MaxEV);
}

} // namespace we::runtime::world::environment
