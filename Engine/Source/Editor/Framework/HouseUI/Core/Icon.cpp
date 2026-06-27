#include "Icon.hpp"
#include "PaintContext.hpp"
#include <sstream>
#include <algorithm>

namespace HouseEngine::UI {

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

} // namespace HouseEngine::UI
