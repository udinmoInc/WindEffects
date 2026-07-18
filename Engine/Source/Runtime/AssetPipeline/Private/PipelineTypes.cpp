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
