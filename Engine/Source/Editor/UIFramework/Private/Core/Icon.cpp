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
    const std::string resolved = Icons::ResolveLucideName(iconName);
    const float displayPx = std::min(bounds.width, bounds.height);
    const uint32_t atlasTier = IconMetrics::TierPxForIcon(resolved, displayPx);
    const Rect drawRect = IconMetrics::PlaceGlyphCentered(bounds, static_cast<float>(atlasTier));
    context.DrawIcon(
        resolved,
        drawRect,
        color,
        static_cast<float>(atlasTier));
}

void IconPainter::DrawCompactIcon(PaintContext& context, const std::string& iconName,
                                  const Rect& bounds, const Color& color) {
    const std::string resolved = Icons::ResolveLucideName(iconName);
    const float displayPx = IconMetrics::CompactDisplayPx();
    const float atlasTier = static_cast<float>(IconMetrics::CompactSourceTierPx());
    const Rect drawRect = IconMetrics::PlaceGlyphCentered(bounds, displayPx);
    context.DrawIcon(resolved, drawRect, color, atlasTier);
}

void IconPainter::DrawVerticalMoreMenu(PaintContext& context, const Rect& bounds, const Color& color) {
    DrawIcon(context, Icons::MoreName, bounds, color);
}

} // namespace WindEffects::Editor::UI
