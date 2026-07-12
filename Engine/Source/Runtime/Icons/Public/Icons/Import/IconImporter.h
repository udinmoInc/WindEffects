#pragma once

#include "Icons/Compile/IconCompiler.h"
#include "Icons/Export.h"

// Legacy SVG import API retained for future offline tooling.
// Runtime and we-icon-compile do not use this path.

#include <filesystem>
#include <memory>
#include <vector>

namespace we::runtime::icons::importing {

struct ImportOptions {
    std::filesystem::path lucideIconsDir;
    std::filesystem::path editorIconsDir;
    std::filesystem::path outputDir;
};

struct ImportResult {
    std::vector<std::filesystem::path> atlasPaths;
    std::filesystem::path metaPath;
    uint32_t iconCount = 0;
};

class ICONS_API IIconImporter {
public:
    virtual ~IIconImporter() = default;
    [[nodiscard]] virtual IconResult<ImportResult> Import(const ImportOptions& options) const = 0;
};

[[nodiscard]] ICONS_API std::unique_ptr<IIconImporter> CreateIconImporter();

} // namespace we::runtime::icons::importing
