// ==============================================================================
// WindEffects — AssetProcessors — ProcessTypes
// Internal implementation for the AssetProcessors module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "AssetProcessors/ProcessTypes.h"

namespace we::runtime::assetprocessors {

bool ProcessResult::HasErrors() const noexcept {
    for (const auto& d : diagnostics) {
        if (d.severity == we::runtime::assetimporter::ImportSeverity::Error
            || d.severity == we::runtime::assetimporter::ImportSeverity::Fatal) {
            return true;
        }
    }
    return !success && !skipped;
}

} // namespace we::runtime::assetprocessors
