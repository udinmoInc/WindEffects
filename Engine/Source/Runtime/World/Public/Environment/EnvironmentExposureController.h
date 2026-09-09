// ==============================================================================
// WindEffects — World — EnvironmentExposureController
// Public API surface for the World module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "World/Export.h"

#include <cstdint>
#include "Core/Math/Types.h"

namespace we::runtime::world::environment {

class WORLD_API EnvironmentExposureController {
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
