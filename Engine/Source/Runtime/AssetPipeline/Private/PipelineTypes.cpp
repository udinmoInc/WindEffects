// ==============================================================================
// WindEffects — AssetPipeline — PipelineTypes
// Internal implementation for the AssetPipeline module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "AssetPipeline/PipelineTypes.h"

namespace we::runtime::assetpipeline {

std::string PipelineResult::PrimaryErrorMessage() const {
    for (const auto& d : diagnostics) {
        if (d.severity == we::runtime::assetimporter::ImportSeverity::Error
            || d.severity == we::runtime::assetimporter::ImportSeverity::Fatal) {
            return d.message.empty() ? d.code : d.message;
        }
    }
    for (const auto& a : assets) {
        if (!a.success && !a.skipped) {
            return "Asset pipeline build failed.";
        }
    }
    return success ? std::string{} : "Asset pipeline failed.";
}

} // namespace we::runtime::assetpipeline
