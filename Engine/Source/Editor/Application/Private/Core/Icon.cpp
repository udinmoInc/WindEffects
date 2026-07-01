#include "Core/Icon.hpp"
#include "Core/PaintContext.hpp"
#include <sstream>
#include <algorithm>

namespace we::UI {

IconRegistry& IconRegistry::Get() {
    static IconRegistry instance;
    return instance;
}

void IconRegistry::InitializeDefaultIcons() {
    // No longer using SVG path icons.
    // Material icons are now rendered directly via FontAtlas codepoints.
}

void IconPainter::DrawIcon(PaintContext& context, const std::string& iconName, 
                           const Point& position, float size, const Color& color) {
    int codepoint = Icons::GetCodepoint(iconName);
    if (codepoint != 0) {
        context.DrawIcon(codepoint, Point{ position.x, position.y }, color, size);
    }
}

void IconPainter::DrawIcon(PaintContext& context, const std::string& iconName,
                           const Rect& bounds, const Color& color) {
    int codepoint = Icons::GetCodepoint(iconName);
    if (codepoint != 0) {
        float size = std::min(bounds.width, bounds.height);
        context.DrawIcon(codepoint, Point{ bounds.x, bounds.y }, color, size);
    }
}

void IconPainter::DrawIcon(PaintContext& context, int codepoint, const Rect& bounds, const Color& color) {
    if (codepoint != 0) {
        float size = std::min(bounds.width, bounds.height);
        context.DrawIcon(codepoint, Point{ bounds.x, bounds.y }, color, size);
    }
}

void IconPainter::DrawVerticalMoreMenu(PaintContext& context, const Rect& bounds, const Color& color) {
    constexpr float kDotRadius = 1.1f;
    constexpr float kDotSpacing = 2.8f;
    const float dotDiameter = kDotRadius * 2.0f;
    const float totalHeight = dotDiameter * 3.0f + kDotSpacing * 2.0f;
    const float x = bounds.x + (bounds.width - dotDiameter) * 0.5f;
    float y = bounds.y + (bounds.height - totalHeight) * 0.5f;

    for (int i = 0; i < 3; ++i) {
        context.DrawRect(Rect{ x, y, dotDiameter, dotDiameter }, color, kDotRadius);
        y += dotDiameter + kDotSpacing;
    }
}

} // namespace we::editor::application::UI
