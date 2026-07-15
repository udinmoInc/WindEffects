#include "Widgets/Image.h"
#include "Core/PaintContext.h"
#include "WindEffects/Editor/UI/Theming/ThemeAccess.h"

namespace WindEffects::Editor::UI {

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

} // namespace WindEffects::Editor::UI
