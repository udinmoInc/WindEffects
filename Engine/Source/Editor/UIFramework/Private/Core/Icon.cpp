#include "Core/Icon.h"
#include "Core/PaintContext.h"
#include "Rendering/IconMetrics.h"
#include <algorithm>

namespace WindEffects::Editor::UI {

IconRegistry& IconRegistry::Get() {
    static IconRegistry instance;
    return instance;
}

void IconRegistry::InitializeDefaultIcons() {
    // Icon overrides are baked into icons.weiconmeta by we-icon-compile.
}

void IconPainter::DrawIcon(PaintContext& context, const std::string& iconName,
                           const Point& position, float size, const Color& color) {
    context.DrawIcon(Icons::ResolveLucideName(iconName), position, color, size);
}

void IconPainter::DrawIcon(PaintContext& context, const std::string& iconName,
                           const Rect& bounds, const Color& color) {
    const uint32_t tierPx = IconMetrics::SnapToAtlasTier(std::min(bounds.width, bounds.height));
    context.DrawIcon(
        Icons::ResolveLucideName(iconName),
        Point{ IconMetrics::SnapPx(bounds.x), IconMetrics::SnapPx(bounds.y) },
        color,
        static_cast<float>(tierPx));
}

void IconPainter::DrawVerticalMoreMenu(PaintContext& context, const Rect& bounds, const Color& color) {
    DrawIcon(context, Icons::MoreName, bounds, color);
}

} // namespace WindEffects::Editor::UI
