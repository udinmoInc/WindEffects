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
