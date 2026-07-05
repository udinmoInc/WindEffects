#include "Environment/EnvironmentExposureController.h"

#include <algorithm>

namespace we::runtime::world::environment {

void EnvironmentExposureController::ApplyDefaults() {
    ExposureEV = 2.35f;
    ExposureCompensation = 0.0f;
    MinEV = -4.0f;
    MaxEV = 8.0f;
    AutoExposure = true;
}

float EnvironmentExposureController::GetEffectiveExposureEV(float sunDerivedEV) const {
    const float base = AutoExposure ? sunDerivedEV : ExposureEV;
    return std::clamp(base + ExposureCompensation, MinEV, MaxEV);
}

} // namespace we::runtime::world::environment
