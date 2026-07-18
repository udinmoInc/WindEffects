#include "KindUI/Core/Icon.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Rendering/IconMetrics.h"
#include <algorithm>

namespace we::runtime::kindui {

IconRegistry& IconRegistry::Get() {
    static IconRegistry instance;
    return instance;
}

void IconRegistry::RegisterIcon(const std::string& name, const std::string& svgPath) {
    std::scoped_lock lock(m_Mutex);
    m_Icons[name] = svgPath;
}

std::string IconRegistry::GetIconPath(const std::string& name) const {
    std::scoped_lock lock(m_Mutex);
    auto it = m_Icons.find(name);
    return it != m_Icons.end() ? it->second : "";
}

bool IconRegistry::HasIcon(const std::string& name) const {
    std::scoped_lock lock(m_Mutex);
    return m_Icons.find(name) != m_Icons.end();
}

bool IconRegistry::IsEditorSvgIcon(const std::string& name) const {
    return HasIcon(name);
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

} // namespace we::runtime::kindui
