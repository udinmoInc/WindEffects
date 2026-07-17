#pragma once

#include "Text/Core/Types.h"
#include "Text/Export.h"

#include <cstdint>
#include <optional>
#include <string>

namespace we::runtime::text::layout {

enum class FontWeight : uint16_t {
    Thin = 100,
    Regular = 400,
    SemiBold = 600,
    Bold = 700,
    Black = 900,
};

enum class TextAlign : uint8_t {
    Left,
    Center,
    Right,
    Justified,
};

enum class VerticalAlign : uint8_t {
    Top,
    Middle,
    Bottom,
    Baseline,
};

struct OutlineStyle {
    ColorRGBA color{};
    float width = 1.0f;
};

struct ShadowStyle {
    ColorRGBA color{};
    float offsetX = 1.0f;
    float offsetY = 1.0f;
    float blur = 2.0f;
};

struct GradientStyle {
    ColorRGBA start{};
    ColorRGBA end{};
};

struct SelectionHighlight {
    ColorRGBA color{};
    size_t start = 0;
    size_t end = 0;
};

struct TextStyle {
    std::string family = "Inter";
    FontWeight weight = FontWeight::Regular;
    bool italic = false;
    float sizePx = 18.0f;
    ColorRGBA color{};
    float opacity = 1.0f;
    float letterSpacing = 0.0f;
    /// Future-ready writing mode (0 = horizontal LTR/RTL, 1 = vertical).
    uint8_t writingMode = 0;
    bool underline = false;
    bool strikethrough = false;
    std::optional<OutlineStyle> outline;
    std::optional<ShadowStyle> shadow;
    std::optional<GradientStyle> gradient;
    std::optional<SelectionHighlight> selection;
};

} // namespace we::runtime::text::layout
