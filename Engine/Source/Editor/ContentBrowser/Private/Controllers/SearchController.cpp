#include "Controllers/SearchController.h"
#include "Registry/AssetTypeResolver.h"
#include <algorithm>
#include <cctype>

namespace we::editor::contentbrowser {

std::string SearchController::ToLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

bool SearchController::Matches(const AssetRecord& asset) const {
    if (m_Query.empty()) return true;

    if (m_UseRegex) {
        try {
            std::regex pattern(m_Query, std::regex::icase);
            if (std::regex_search(asset.name, pattern)) return true;
            if (std::regex_search(asset.virtualPath, pattern)) return true;
            if (std::regex_search(AssetTypeToString(asset.type), pattern)) return true;
            if (std::regex_search(asset.extension, pattern)) return true;
        } catch (...) {
            return false;
        }
        return false;
    }

    const auto contains = [&](const std::string& haystack) {
        return ToLower(haystack).find(m_LowerQuery) != std::string::npos;
    };

    return contains(asset.name) ||
           contains(asset.virtualPath) ||
           contains(AssetTypeToString(asset.type)) ||
           contains(asset.extension) ||
           contains(asset.parentPath);
}

} // namespace we::editor::contentbrowser
