// ==============================================================================
// WindEffects — AssetProcessors — ProcessStage
// Internal implementation for the AssetProcessors module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "AssetProcessors/ProcessStage.h"

namespace we::runtime::assetprocessors {

std::string_view ProcessStageToString(ProcessStage stage) {
    switch (stage) {
    case ProcessStage::Validate: return "Validate";
    case ProcessStage::AnalyzeDependencies: return "AnalyzeDependencies";
    case ProcessStage::Optimize: return "Optimize";
    case ProcessStage::GenerateLods: return "GenerateLods";
    case ProcessStage::GenerateMips: return "GenerateMips";
    case ProcessStage::GenerateThumbnail: return "GenerateThumbnail";
    case ProcessStage::WriteSidecars: return "WriteSidecars";
    case ProcessStage::PostProcess: return "PostProcess";
    default: return "Unknown";
    }
}

} // namespace we::runtime::assetprocessors
