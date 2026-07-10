#pragma once

#include "Rendering/FontAtlas.h"
#include "Rendering/Text/ShapedGlyph.h"

#include <string_view>
#include <vector>

namespace we::UI::Text {

// Logs per-glyph layout metrics (FT vs HarfBuzz vs MSDF) for one UTF-8 string.
void AuditTextLayout(
    std::string_view utf8Text,
    const FontAtlas* fontAtlas,
    const std::vector<ShapedGlyph>& shapedGlyphs,
    float layoutScale,
    uint32_t maxGlyphsToLog = 32);

} // namespace we::UI::Text
