#include "Widgets/Image.h"
#include "Core/PaintContext.h"

namespace we::UI {

Image::Image(VkDescriptorSet textureId) : m_TextureId(textureId) {}

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

    if (m_TextureId != VK_NULL_HANDLE) {
        context.DrawTexture(m_Geometry, m_TextureId, m_TintColor, m_TintBottom);
    } else {
        // Placeholder checkboard or solid color
        context.DrawRect(m_Geometry, Color{ 0.15f, 0.15f, 0.18f, 1.0f });
    }
}

} // namespace we::editor::application::UI
