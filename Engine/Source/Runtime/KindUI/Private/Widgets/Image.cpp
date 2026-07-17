#include "KindUI/Widgets/Image.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Theming/ThemeAccess.h"

namespace we::runtime::kindui {

Image::Image(we::rhi::RHIDescriptorSetHandle textureId) : m_TextureId(textureId) {}

Size Image::Measure(const Size& availableSize) {
    (void)availableSize;
    if (m_UseCustomSize) {
        m_DesiredSize = m_CustomSize;
    } else {
        m_DesiredSize = Size{ 64.0f, 64.0f };
    }
    return m_DesiredSize;
}

void Image::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
}

void Image::Paint(PaintContext& context) {
    if (!m_Visible) return;

    if ((m_TextureId != we::rhi::RHIDescriptorSetHandle::Invalid)) {
        context.DrawTexture(m_Geometry, m_TextureId, m_TintColor, m_TintBottom);
    } else {
        // Placeholder checkboard or solid color
        context.DrawRect(m_Geometry, ResolveThemeColor(ThemeToken::DisabledBackground));
    }
}

} // namespace we::runtime::kindui
