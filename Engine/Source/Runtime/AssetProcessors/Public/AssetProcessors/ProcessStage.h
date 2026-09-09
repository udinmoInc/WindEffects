// ==============================================================================
// WindEffects — AssetProcessors — ProcessStage
// Public API surface for the AssetProcessors module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "AssetProcessors/Export.h"

#include <cstdint>
#include <string_view>

namespace we::runtime::assetprocessors {

/// Ordered processing stages after a successful import. Importers never run these.
enum class ProcessStage : uint32_t {
    Validate = 0,
    AnalyzeDependencies,
    Optimize,
    GenerateLods,
    GenerateMips,
    GenerateThumbnail,
    WriteSidecars,
    PostProcess,
    Count
};

[[nodiscard]] ASSETPROCESSORS_API std::string_view ProcessStageToString(ProcessStage stage);

} // namespace we::runtime::assetprocessors
