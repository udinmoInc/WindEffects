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
