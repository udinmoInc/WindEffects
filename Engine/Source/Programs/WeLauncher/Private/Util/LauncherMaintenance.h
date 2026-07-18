#pragma once

#include <string>

namespace we::programs::welauncher {

struct AssociationResult {
    bool ok = false;
    std::string message;
};

[[nodiscard]] AssociationResult AssociateProjectExtension(const std::string& extensionWithDot);
[[nodiscard]] bool OpenPathInExplorer(const std::string& pathUtf8);
[[nodiscard]] bool OpenUrl(const std::string& url);

} // namespace we::programs::welauncher
