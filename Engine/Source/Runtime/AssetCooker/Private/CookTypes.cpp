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
