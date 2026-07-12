#pragma once

#include "Icons/Assets/IconAsset.h"
#include "Icons/Export.h"

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace we::runtime::icons::compiling {

struct CompileOptions {
    std::filesystem::path inputDir;
    std::filesystem::path outputDir;
};

struct CompileResult {
    std::vector<std::filesystem::path> atlasPaths;
    std::filesystem::path metaPath;
    uint32_t iconCount = 0;
    uint32_t tierCount = 0;
};

class ICONS_API IIconCompiler {
public:
    virtual ~IIconCompiler() = default;
    [[nodiscard]] virtual IconResult<CompileResult> Compile(const CompileOptions& options) const = 0;
};

[[nodiscard]] ICONS_API std::unique_ptr<IIconCompiler> CreateIconCompiler();

} // namespace we::runtime::icons::compiling
