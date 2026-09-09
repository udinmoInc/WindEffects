// ==============================================================================
// WindEffects — AssetImporter — ImportTypes
// Internal implementation for the AssetImporter module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "AssetImporter/ImportTypes.h"

namespace we::runtime::assetimporter {

bool ImportResult::HasErrors() const noexcept {
    for (const auto& d : diagnostics) {
        if (d.severity == ImportSeverity::Error || d.severity == ImportSeverity::Fatal) {
            return true;
        }
    }
    return !success;
}

std::string ImportResult::PrimaryErrorMessage() const {
    for (const auto& d : diagnostics) {
        if (d.severity == ImportSeverity::Fatal || d.severity == ImportSeverity::Error) {
            return d.message;
        }
    }
    return success ? std::string{} : std::string{ "Import failed." };
}

} // namespace we::runtime::assetimporter
