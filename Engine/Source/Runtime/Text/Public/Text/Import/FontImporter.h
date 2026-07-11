#pragma once

#include "Text/Assets/FontAsset.h"
#include "Text/Core/Types.h"
#include "Text/Export.h"

#include <filesystem>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

namespace we::runtime::text::importing {

using assets::FontAsset;

struct ImportOptions {
    float bakeSizePx = 18.0f;
    float msdfPixelRange = 4.0f;
    int atlasWidth = 0;
    int atlasHeight = 0;
    uint32_t ttcFaceIndex = 0;
    std::string charset = "basic";
    std::filesystem::path fallbackFontPath;
};

class TEXT_API IFontImporter {
public:
    virtual ~IFontImporter() = default;
    [[nodiscard]] virtual we::runtime::text::TextResult<FontAsset> Import(
        const std::filesystem::path& inputPath,
        const ImportOptions& options) const = 0;
};

class TEXT_API TtfOtfImporter final : public IFontImporter {
public:
    [[nodiscard]] we::runtime::text::TextResult<FontAsset> Import(
        const std::filesystem::path& inputPath,
        const ImportOptions& options) const override;
};

class TEXT_API TtcImporter final : public IFontImporter {
public:
    [[nodiscard]] we::runtime::text::TextResult<FontAsset> Import(
        const std::filesystem::path& inputPath,
        const ImportOptions& options) const override;
};

[[nodiscard]] TEXT_API std::unique_ptr<IFontImporter> CreateFontImporter(
    const std::filesystem::path& inputPath);

[[nodiscard]] TEXT_API std::unordered_set<Codepoint> BuildCharset(
    const std::string& charsetName);

} // namespace we::runtime::text::importing
