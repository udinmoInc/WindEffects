// ==============================================================================
// WindEffects — AssetCooker — CookTypes
// Internal implementation for the AssetCooker module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "AssetCooker/CookTypes.h"

namespace we::runtime::assetcooker {

std::string CookResult::PrimaryErrorMessage() const {
    for (const auto& d : diagnostics) {
        if (d.severity == we::runtime::assetimporter::ImportSeverity::Error
            || d.severity == we::runtime::assetimporter::ImportSeverity::Fatal) {
            return d.message.empty() ? d.code : d.message;
        }
    }
    return success ? std::string{} : "Cook failed.";
}

} // namespace we::runtime::assetcooker
