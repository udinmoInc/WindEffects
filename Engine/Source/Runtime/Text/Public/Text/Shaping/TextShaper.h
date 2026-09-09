// ==============================================================================
// WindEffects — Text — TextShaper
// Public API surface for the Text module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "Text/Core/Errors.h"
#include "Text/Core/Types.h"
#include "Text/Export.h"

#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace we::runtime::text::assets {
class IFontAssetManager;
}

namespace we::runtime::text::shaping {

enum class TextDirection : uint8_t {
    LeftToRight,
    RightToLeft,
};

enum class Script : uint8_t {
    Common,
    Latin,
    Cyrillic,
    Greek,
    Arabic,
    Devanagari,
    Han,
    Hiragana,
    Katakana,
    Hangul,
    Emoji,
    Unknown,
};

struct ShapedGlyph {
    Codepoint codepoint = 0;
    uint32_t glyphIndex = 0;
    FontHandle fontHandle = kInvalidFontHandle;
    float xOffset = 0.0f;
    float yOffset = 0.0f;
    float xAdvance = 0.0f;
    float yAdvance = 0.0f;
    uint32_t cluster = 0;
};

struct ShapedRun {
    std::vector<ShapedGlyph> glyphs;
    TextDirection direction = TextDirection::LeftToRight;
    Script script = Script::Unknown;
};

struct ShapeOptions {
    float fontSize = 18.0f;
    bool enableKerning = true;
    bool enableLigatures = true;
    TextDirection direction = TextDirection::LeftToRight;
};

class TEXT_API ITextShaper {
public:
    virtual ~ITextShaper() = default;
    [[nodiscard]] virtual TextResult<std::vector<ShapedRun>> Shape(
        std::span<const Codepoint> codepoints,
        FontHandle primaryFont,
        const ShapeOptions& options) const = 0;
};

[[nodiscard]] TEXT_API Script DetectScript(Codepoint codepoint);
[[nodiscard]] TEXT_API Script DetectScriptFromName(std::string_view name);
[[nodiscard]] TEXT_API std::unique_ptr<ITextShaper> CreateTextShaper(assets::IFontAssetManager& assetManager);

} // namespace we::runtime::text::shaping
