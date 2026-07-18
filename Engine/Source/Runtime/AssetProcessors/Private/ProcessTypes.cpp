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
