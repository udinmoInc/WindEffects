#include "Text/Layout/TextLayoutEngine.h"
#include "Text/Shaping/BidiProcessor.h"

#include <algorithm>
#include <cmath>

namespace we::runtime::text::layout {

namespace {

// Forces ABI-safe PositionedGlyph (no TextStyle) into linked WEText.dll.
constexpr int kLayoutEngineAbiTag = 2;

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

float SnapPixel(const float v)
{
    return std::floor(v + 0.5f);
}

} // namespace

class TextLayoutEngine final : public ITextLayoutEngine {
public:
    TextLayoutEngine(
        shaping::ITextShaper& shaper,
        assets::IGlyphResolver& glyphResolver,
        assets::IFallbackResolver& fallbackResolver,
        shaping::IBidiProcessor* bidiProcessor)
        : m_Shaper(shaper)
        , m_GlyphResolver(glyphResolver)
        , m_FallbackResolver(fallbackResolver)
        , m_BidiProcessor(bidiProcessor)
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

        std::vector<shaping::VisualRun> visualRuns;
        if (m_BidiProcessor) {
            visualRuns = m_BidiProcessor->ReorderVisual(codepoints);
        } else {
            shaping::VisualRun run;
            run.codepoints = codepoints;
            run.startIndex = 0;
            run.length = codepoints.size();
            run.direction = shaping::TextDirection::LeftToRight;
            visualRuns.push_back(run);
        }

        struct LineState {
            std::vector<PositionedGlyph> glyphs;
            float width = 0.0f;
            float maxAscent = 0.0f;
            float maxDescent = 0.0f;
            float lineHeight = 0.0f;
            size_t startIndex = 0;
            size_t endIndex = 0;
        };

        std::vector<LineState> lineStates;
        lineStates.emplace_back();
        lineStates.back().startIndex = 0;

        float penX = 0.0f;
        size_t codepointIndex = 0;
        const float displaySize = style.sizePx * constraints.dpiScale;

        for (const shaping::VisualRun& visualRun : visualRuns) {
            shaping::ShapeOptions shapeOptions;
            shapeOptions.fontSize = displaySize;
            shapeOptions.direction = visualRun.direction;
            const auto shaped = m_Shaper.Shape(visualRun.codepoints, primaryFont, shapeOptions);
            if (!shaped.ok || shaped.value.empty()) {
                codepointIndex += visualRun.length;
                continue;
            }

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

                    const assets::ResolvedGlyph resolved =
                        m_GlyphResolver.Resolve(stack, shapedGlyph.codepoint);
                    const float geometryScale = resolved.EffectiveGeometryScale();
                    const float quadScale = displaySize / geometryScale;

                    // Face metrics come from FreeType at bake size; scale to display size.
                    // Never invent ascent/descent — fall back only to measured glyph bounds.
                    const float ascent = resolved.ascender > 0.0f
                        ? resolved.ascender * quadScale
                        : std::max(0.0f, -resolved.metrics.bounds.y * quadScale);
                    const float descent = resolved.descender != 0.0f
                        ? std::abs(resolved.descender) * quadScale
                        : std::max(0.0f, (resolved.metrics.bounds.y + resolved.metrics.bounds.height) * quadScale);
                    const float faceLineHeight = resolved.lineHeight > 0.0f
                        ? resolved.lineHeight * quadScale
                        : (ascent + descent);

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
                    // Plane bounds are y-up; UI is y-down. Position relative to baseline (y=0 here).
                    const float planeTop = resolved.metrics.bounds.y + resolved.metrics.bounds.height;
                    positioned.x = penX + shapedGlyph.xOffset + resolved.metrics.bounds.x * quadScale;
                    positioned.y = shapedGlyph.yOffset - planeTop * quadScale;
                    positioned.width = resolved.metrics.bounds.width * quadScale;
                    positioned.height = resolved.metrics.bounds.height * quadScale;
                    positioned.lineIndex = static_cast<uint32_t>(lineStates.size() - 1);
                    positioned.color = style.color;
                    positioned.color.a *= style.opacity;
                    // Atlas distance range in texels — screenPxRange uses fwidth for scale.
                    positioned.msdfPixelRange = resolved.msdfPixelRange;
                    // Outline/shadow are not used by KindUI measure/draw today; keep POD defaults
                    // to avoid reading std::optional across the Text DLL boundary.

                    lineStates.back().glyphs.push_back(positioned);
                    lineStates.back().width = penX + advance;
                    lineStates.back().maxAscent = std::max(lineStates.back().maxAscent, ascent);
                    lineStates.back().maxDescent = std::max(lineStates.back().maxDescent, descent);
                    lineStates.back().lineHeight = std::max(
                        lineStates.back().lineHeight,
                        std::max(faceLineHeight, ascent + descent) * constraints.lineSpacing);
                    lineStates.back().endIndex = codepointIndex;

                    penX += advance;
                    ++codepointIndex;
                }
            }
        }

        float y = 0.0f;
        float totalHeight = 0.0f;
        float maxLineWidth = 0.0f;

        for (uint32_t lineIndex = 0; lineIndex < lineStates.size(); ++lineIndex) {
            LineState& line = lineStates[lineIndex];
            const float lineHeight = line.lineHeight > 0.0f
                ? line.lineHeight
                : std::max(line.maxAscent + line.maxDescent, displaySize) * constraints.lineSpacing;
            const float ascent = line.maxAscent > 0.0f ? line.maxAscent : displaySize;

            float alignOffset = 0.0f;
            if (constraints.horizontalAlign == TextAlign::Center) {
                alignOffset = std::max((constraints.maxWidth - line.width) * 0.5f, 0.0f);
            } else if (constraints.horizontalAlign == TextAlign::Right) {
                alignOffset = std::max(constraints.maxWidth - line.width, 0.0f);
            }

            // Snap only the line origin / baseline. Preserve fractional HarfBuzz advances
            // so per-glyph SnapPixel does not introduce uneven spacing or soft edges.
            const float baselineY = SnapPixel(y + ascent);
            const float snappedAlign = SnapPixel(alignOffset);

            LineMetrics metrics;
            metrics.lineIndex = lineIndex;
            metrics.width = line.width;
            metrics.height = lineHeight;
            metrics.baselineY = baselineY;
            metrics.startIndex = line.startIndex;
            metrics.endIndex = line.endIndex;
            result.lines.push_back(metrics);

            CaretPosition startCaret;
            startCaret.codepointIndex = line.startIndex;
            startCaret.x = snappedAlign;
            startCaret.y = y;
            startCaret.lineIndex = lineIndex;
            result.caretMap.push_back(startCaret);

            for (PositionedGlyph& glyph : line.glyphs) {
                glyph.x = glyph.x + snappedAlign;
                glyph.y = glyph.y + baselineY;
                if (!(constraints.clip && constraints.maxWidth > 0.0f
                        && (glyph.x > constraints.maxWidth || glyph.y > constraints.maxHeight))) {
                    result.glyphs.push_back(glyph);
                }
            }

            CaretPosition caret;
            caret.codepointIndex = line.endIndex + 1;
            caret.x = line.width + snappedAlign;
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
    shaping::IBidiProcessor* m_BidiProcessor = nullptr;
};

std::unique_ptr<ITextLayoutEngine> CreateTextLayoutEngine(
    shaping::ITextShaper& shaper,
    assets::IGlyphResolver& glyphResolver,
    assets::IFallbackResolver& fallbackResolver,
    shaping::IBidiProcessor* bidiProcessor)
{
    return std::make_unique<TextLayoutEngine>(shaper, glyphResolver, fallbackResolver, bidiProcessor);
}

} // namespace we::runtime::text::layout
