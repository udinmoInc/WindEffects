#pragma once

#include "Application/Export.h"

#include <cstdint>
#include <vector>

namespace we::UI::Text {

// One shaped glyph: HarfBuzz offsets + MSDF atlas advance for pen stepping.
struct APPLICATION_API ShapedGlyph {
    uint32_t codepoint = 0;
    uint32_t glyphId = 0;
    float xOffset = 0.0f;
    float yOffset = 0.0f;
    float hbAdvanceX = 0.0f;
};

struct APPLICATION_API TextLayoutMetrics {
    float ascender = 0.0f;
    float descender = 0.0f;
    float lineHeight = 0.0f;
    float underlinePosition = 0.0f;
    float underlineThickness = 0.0f;
};

} // namespace we::UI::Text
