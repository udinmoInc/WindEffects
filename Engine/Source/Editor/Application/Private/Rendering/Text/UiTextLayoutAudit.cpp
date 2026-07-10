#include "Rendering/Text/UiTextLayoutAudit.h"

#include "Core/Logger.h"
#include "Rendering/FontAtlas.h"

#include <cstdio>
#include <string>

namespace we::UI::Text {

namespace {

std::string CodepointLabel(uint32_t codepoint) {
    char buf[24];
    if (codepoint < 0x80 && codepoint >= 0x20) {
        std::snprintf(buf, sizeof(buf), "'%c' U+%04X", static_cast<char>(codepoint), codepoint);
    } else {
        std::snprintf(buf, sizeof(buf), "U+%04X", codepoint);
    }
    return buf;
}

} // namespace

void AuditTextLayout(
    std::string_view utf8Text,
    const FontAtlas* fontAtlas,
    const std::vector<ShapedGlyph>& shapedGlyphs,
    float layoutScale,
    uint32_t maxGlyphsToLog) {
    if (!fontAtlas || shapedGlyphs.empty()) {
        return;
    }

    HE_INFO(std::string("[UI LayoutAudit] text=\"") + std::string(utf8Text) + "\" scale=" + std::to_string(layoutScale)
            + " glyphs=" + std::to_string(shapedGlyphs.size()));

    float penBefore = 0.0f;
    const uint32_t count = std::min(maxGlyphsToLog, static_cast<uint32_t>(shapedGlyphs.size()));
    for (uint32_t i = 0; i < count; ++i) {
        const ShapedGlyph& g = shapedGlyphs[i];
        GlyphInfo quad{};
        const bool drawable = fontAtlas->GetGlyphQuadAt(g.codepoint, penBefore + g.xOffset, g.yOffset, quad);
        const float msdfAdvance = fontAtlas->GetGlyphAdvance(g.codepoint);
        const float penAfter = penBefore + g.hbAdvanceX;

        const float bearingX = drawable ? quad.x0 - (penBefore + g.xOffset) : 0.0f;
        const float width = drawable ? (quad.x1 - quad.x0) : 0.0f;
        const float height = drawable ? (quad.y1 - quad.y0) : 0.0f;

        HE_INFO(std::string("[UI LayoutAudit] #") + std::to_string(i) + " char=" + CodepointLabel(g.codepoint)
                + " glyphId=" + std::to_string(g.glyphId)
                + " msdfAdvance=" + std::to_string(msdfAdvance)
                + " hbAdvance=" + std::to_string(g.hbAdvanceX)
                + " xOffset=" + std::to_string(g.xOffset)
                + " yOffset=" + std::to_string(g.yOffset)
                + " bearingX=" + std::to_string(bearingX)
                + " width=" + std::to_string(width)
                + " height=" + std::to_string(height)
                + " penBefore=" + std::to_string(penBefore)
                + " penAfter=" + std::to_string(penAfter)
                + (drawable ? " quad=(" + std::to_string(quad.x0 * layoutScale) + ","
                    + std::to_string(quad.y0 * layoutScale) + ")-("
                    + std::to_string(quad.x1 * layoutScale) + ","
                    + std::to_string(quad.y1 * layoutScale) + ")" : " no-quad"));

        penBefore = penAfter;
    }
}

} // namespace we::UI::Text
