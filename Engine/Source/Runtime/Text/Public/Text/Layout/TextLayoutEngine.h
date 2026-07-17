#pragma once

#include "Text/Assets/GlyphResolver.h"
#include "Text/Core/Types.h"
#include "Text/Export.h"
#include "Text/Layout/TextStyle.h"
#include "Text/Shaping/BidiProcessor.h"
#include "Text/Shaping/TextShaper.h"

#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace we::runtime::text::layout {

struct LayoutConstraints {
    float maxWidth = 0.0f;
    float maxHeight = 0.0f;
    TextAlign horizontalAlign = TextAlign::Left;
    VerticalAlign verticalAlign = VerticalAlign::Top;
    float lineSpacing = 1.0f;
    float paragraphSpacing = 0.0f;
    float letterSpacing = 0.0f;
    std::vector<float> tabStops;
    bool wordWrap = true;
    bool clip = false;
    float dpiScale = 1.0f;
};

struct CaretPosition {
    size_t codepointIndex = 0;
    float x = 0.0f;
    float y = 0.0f;
    uint32_t lineIndex = 0;
};

struct LineMetrics {
    float width = 0.0f;
    float height = 0.0f;
    float baselineY = 0.0f;
    uint32_t lineIndex = 0;
    size_t startIndex = 0;
    size_t endIndex = 0;
};

struct PositionedGlyph {
    assets::ResolvedGlyph glyph;
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    uint32_t lineIndex = 0;
    TextStyle style{};
    ColorRGBA color{};
    float msdfPixelRange = 4.0f;
};

struct LayoutResult {
    std::vector<PositionedGlyph> glyphs;
    Bounds bounds{};
    std::vector<LineMetrics> lines;
    std::vector<CaretPosition> caretMap;
};

class TEXT_API ITextLayoutEngine {
public:
    virtual ~ITextLayoutEngine() = default;
    [[nodiscard]] virtual LayoutResult Layout(
        std::span<const Codepoint> codepoints,
        const TextStyle& style,
        const LayoutConstraints& constraints,
        FontHandle primaryFont) const = 0;
};

[[nodiscard]] TEXT_API std::unique_ptr<ITextLayoutEngine> CreateTextLayoutEngine(
    shaping::ITextShaper& shaper,
    assets::IGlyphResolver& glyphResolver,
    assets::IFallbackResolver& fallbackResolver,
    shaping::IBidiProcessor* bidiProcessor = nullptr);

} // namespace we::runtime::text::layout
