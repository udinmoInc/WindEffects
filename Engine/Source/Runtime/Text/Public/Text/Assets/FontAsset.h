// ==============================================================================
// WindEffects — Text — FontAsset
// Public API surface for the Text module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "Text/Core/Errors.h"
#include "Text/Core/FontTypes.h"
#include "Text/Export.h"

#include <cstdint>
#include <filesystem>
#include <memory>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

namespace we::runtime::text::assets {

constexpr uint32_t kWeFontMagic = 0x4E464557; // "WEFN" little-endian
constexpr uint16_t kWeFontVersion = 1;

struct FontAsset {
    FontMetrics metrics{};
    AtlasFormat atlasFormat = AtlasFormat::Msdf;
    std::vector<GlyphMetrics> glyphs;
    std::vector<KerningPair> kerning;
    std::vector<UnicodeRange> coverage;
    std::vector<AtlasPage> atlasPages;
    /// Original .ttf/.otf path used for HarfBuzz shaping (not the .wefont asset path).
    std::filesystem::path sourcePath;

    [[nodiscard]] const GlyphMetrics* FindGlyph(Codepoint codepoint) const;
    [[nodiscard]] std::unordered_map<Codepoint, size_t> BuildGlyphIndex() const;
};

class TEXT_API FontAssetReader {
public:
    [[nodiscard]] static we::runtime::text::TextResult<FontAsset> LoadFromFile(const std::filesystem::path& path);
    [[nodiscard]] static we::runtime::text::TextResult<FontAsset> LoadFromMemory(std::span<const std::byte> data);
};

class TEXT_API FontAssetWriter {
public:
    [[nodiscard]] static we::runtime::text::TextResult<void> WriteToFile(
        const FontAsset& asset,
        const std::filesystem::path& path);
};

class TEXT_API FontAssetValidator {
public:
    struct ValidationReport {
        bool isValid = true;
        std::vector<std::string> errors;
        std::vector<std::string> warnings;
    };

    [[nodiscard]] static ValidationReport Validate(const FontAsset& asset);
};

} // namespace we::runtime::text::assets
