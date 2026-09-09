// ==============================================================================
// WindEffects — World — EnvironmentExposureController
// Internal implementation for the World module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "Environment/EnvironmentExposureController.h"

#include <algorithm>

namespace we::runtime::world::environment {

void EnvironmentExposureController::ApplyDefaults() {
    ExposureEV = 0.0f;
    ExposureCompensation = 0.0f;
    MinEV = -2.0f;
    MaxEV = 14.0f;
    AutoExposure = true;
    AppliedDefaultsVersion = kDefaultsVersion;
}

float EnvironmentExposureController::GetEffectiveExposureEV(float sunDerivedEV) const {
    const float base = AutoExposure ? sunDerivedEV : ExposureEV;
    return std::clamp(base + ExposureCompensation, MinEV, MaxEV);
}

} // namespace we::runtime::world::environment
