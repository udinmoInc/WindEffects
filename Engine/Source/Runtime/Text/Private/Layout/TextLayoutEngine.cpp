#include "Text/Layout/TextLayoutEngine.h"

#include <algorithm>
#include <cmath>

namespace we::runtime::text::layout {

namespace {

bool IsWhitespace(const Codepoint codepoint)
{
    return codepoint == ' ' || codepoint == '\t' || codepoint == '\n' || codepoint == '\r';
}

bool IsBreakOpportunity(const Codepoint codepoint)
{
    return IsWhitespace(codepoint) || codepoint == '-';
}

float ResolveTabStop(const float x, const std::vector<float>& tabStops)
{
    for (const float stop : tabStops) {
        if (stop > x) {
            return stop;
        }
    }
    return std::ceil((x + 1.0f) / 40.0f) * 40.0f;
}

} // namespace

class TextLayoutEngine final : public ITextLayoutEngine {
public:
    TextLayoutEngine(
        shaping::ITextShaper& shaper,
        assets::IGlyphResolver& glyphResolver,
        assets::IFallbackResolver& fallbackResolver)
        : m_Shaper(shaper)
        , m_GlyphResolver(glyphResolver)
        , m_FallbackResolver(fallbackResolver)
    {
    }

    LayoutResult Layout(
        const std::span<const Codepoint> codepoints,
        const TextStyle& style,
        const LayoutConstraints& constraints,
        const FontHandle primaryFont) const override
    {
        LayoutResult result;
        if (codepoints.empty()) {
            return result;
        }

        shaping::ShapeOptions shapeOptions;
        shapeOptions.fontSize = style.sizePx * constraints.dpiScale;
        const auto shaped = m_Shaper.Shape(codepoints, primaryFont, shapeOptions);
        if (!shaped.ok || shaped.value.empty()) {
            return result;
        }

        // Prefer the explicitly requested face (e.g. Inter Bold) before family/fallback
        // lookup. BuildStack matches on family name only, so Regular and Bold both named
        // "Inter" would otherwise resolve UVs from the first loaded face while the UI
        // binds a different atlas — producing garbled MSDF glyphs.
        assets::FontStack stack;
        if (primaryFont != kInvalidFontHandle) {
            stack.faces.push_back(primaryFont);
        }
        const assets::FontStack fallback =
            m_FallbackResolver.BuildStack(style.family, shaped.value.front().script);
        for (const FontHandle handle : fallback.faces) {
            if (std::find(stack.faces.begin(), stack.faces.end(), handle) == stack.faces.end()) {
                stack.faces.push_back(handle);
            }
        }

        struct LineState {
            std::vector<PositionedGlyph> glyphs;
            float width = 0.0f;
            float maxAscent = 0.0f;
            float maxDescent = 0.0f;
            size_t startIndex = 0;
            size_t endIndex = 0;
        };

        std::vector<LineState> lineStates;
        lineStates.emplace_back();
        lineStates.back().startIndex = 0;

        float penX = 0.0f;
        size_t codepointIndex = 0;

        for (const shaping::ShapedRun& run : shaped.value) {
            for (const shaping::ShapedGlyph& shapedGlyph : run.glyphs) {
                if (shapedGlyph.codepoint == '\n') {
                    lineStates.back().endIndex = codepointIndex;
                    lineStates.emplace_back();
                    lineStates.back().startIndex = codepointIndex + 1;
                    penX = 0.0f;
                    ++codepointIndex;
                    continue;
                }

                const assets::ResolvedGlyph resolved = m_GlyphResolver.Resolve(stack, shapedGlyph.codepoint);
                const float displaySize = style.sizePx * constraints.dpiScale;
                const float geometryScale = std::max(resolved.geometryScale, 1.0f);
                const float quadScale = displaySize / geometryScale;
                const float bakeScale = displaySize / std::max(resolved.bakeSizePx, 1.0f);

                float advance = shapedGlyph.xAdvance + style.letterSpacing + constraints.letterSpacing;
                if (shapedGlyph.codepoint == '\t') {
                    penX = ResolveTabStop(penX, constraints.tabStops);
                    advance = 0.0f;
                }

                if (constraints.wordWrap && constraints.maxWidth > 0.0f
                    && penX + advance > constraints.maxWidth
                    && !lineStates.back().glyphs.empty()
                    && IsBreakOpportunity(shapedGlyph.codepoint)) {
                    lineStates.back().endIndex = codepointIndex;
                    lineStates.emplace_back();
                    lineStates.back().startIndex = codepointIndex;
                    penX = 0.0f;
                }

                PositionedGlyph positioned;
                positioned.glyph = resolved;
                const float planeTop = resolved.metrics.bounds.y + resolved.metrics.bounds.height;
                positioned.x = penX + shapedGlyph.xOffset + resolved.metrics.bounds.x * quadScale;
                positioned.y = shapedGlyph.yOffset - planeTop * quadScale;
                positioned.width = resolved.metrics.bounds.width * quadScale;
                positioned.height = resolved.metrics.bounds.height * quadScale;
                positioned.lineIndex = static_cast<uint32_t>(lineStates.size() - 1);
                positioned.style = style;
                positioned.color = style.color;
                positioned.color.a *= style.opacity;
                positioned.msdfPixelRange = resolved.msdfPixelRange * bakeScale;

                lineStates.back().glyphs.push_back(positioned);
                lineStates.back().width = penX + advance;
                lineStates.back().maxAscent = std::max(lineStates.back().maxAscent, positioned.height);
                lineStates.back().maxDescent = std::max(lineStates.back().maxDescent, 0.0f);
                lineStates.back().endIndex = codepointIndex;

                penX += advance;
                ++codepointIndex;
            }
        }

        float y = 0.0f;
        float totalHeight = 0.0f;
        float maxLineWidth = 0.0f;

        for (uint32_t lineIndex = 0; lineIndex < lineStates.size(); ++lineIndex) {
            LineState& line = lineStates[lineIndex];
            const float lineHeight = (style.sizePx * constraints.dpiScale) * constraints.lineSpacing;
            float alignOffset = 0.0f;
            if (constraints.horizontalAlign == TextAlign::Center) {
                alignOffset = std::max((constraints.maxWidth - line.width) * 0.5f, 0.0f);
            } else if (constraints.horizontalAlign == TextAlign::Right) {
                alignOffset = std::max(constraints.maxWidth - line.width, 0.0f);
            }

            LineMetrics metrics;
            metrics.lineIndex = lineIndex;
            metrics.width = line.width;
            metrics.height = lineHeight;
            metrics.baselineY = y + (style.sizePx * constraints.dpiScale) * 0.85f;
            metrics.startIndex = line.startIndex;
            metrics.endIndex = line.endIndex;
            result.lines.push_back(metrics);

            for (PositionedGlyph& glyph : line.glyphs) {
                glyph.x += alignOffset;
                glyph.y += metrics.baselineY;
                if (constraints.clip && constraints.maxWidth > 0.0f
                    && (glyph.x > constraints.maxWidth || glyph.y > constraints.maxHeight)) {
                    continue;
                }
                result.glyphs.push_back(glyph);
            }

            CaretPosition caret;
            caret.codepointIndex = line.endIndex;
            caret.x = line.width + alignOffset;
            caret.y = y;
            caret.lineIndex = lineIndex;
            result.caretMap.push_back(caret);

            maxLineWidth = std::max(maxLineWidth, line.width);
            totalHeight += lineHeight + constraints.paragraphSpacing;
            y += lineHeight + constraints.paragraphSpacing;
        }

        result.bounds.width = maxLineWidth;
        result.bounds.height = totalHeight;
        return result;
    }

private:
    shaping::ITextShaper& m_Shaper;
    assets::IGlyphResolver& m_GlyphResolver;
    assets::IFallbackResolver& m_FallbackResolver;
};

std::unique_ptr<ITextLayoutEngine> CreateTextLayoutEngine(
    shaping::ITextShaper& shaper,
    assets::IGlyphResolver& glyphResolver,
    assets::IFallbackResolver& fallbackResolver)
{
    return std::make_unique<TextLayoutEngine>(shaper, glyphResolver, fallbackResolver);
}

} // namespace we::runtime::text::layout
