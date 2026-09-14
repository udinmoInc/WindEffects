// ==============================================================================
// WindEffects — KindUI — UiGpuPathStats
// Public API surface for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "KindUI/Export.h"

#include <cstdint>

namespace we::runtime::kindui {

/// Cumulative upload / buffer metrics for the shared KindUI GPU path
/// (`UiImmediateRenderer`). Exposed via `OverlayRenderer::GetGpuPathStats()`.
struct KINDUI_API UiGpuPathStats {
    uint64_t geometryUploadCount = 0;
    uint64_t geometryUploadBytes = 0;
    uint64_t geometryUploadSkipCount = 0;
    uint64_t geometryContentHashSkipCount = 0;
    uint64_t bufferCreateCount = 0;
    uint64_t bufferReallocCount = 0;
    uint64_t bufferRetireCount = 0;
    uint64_t textureCreateCount = 0;
    uint64_t textureUpdateCount = 0;
    uint64_t textureUpdateBytes = 0;
    uint64_t textureRetireCount = 0;
    float lastGeometryUploadCpuMs = 0.0f;
    float accumGeometryUploadCpuMs = 0.0f;
};

} // namespace we::runtime::kindui
