#include "Widgets/RenderTargetPreviewWidget.hpp"
#include "Core/Theme.hpp"
#include "Core/PaintContext.hpp"

#include <algorithm>

namespace we::UI {

namespace {
constexpr float kPadding = 12.0f;
constexpr float kHeader = 28.0f;
constexpr float kCloseSize = 20.0f;
} // namespace

RenderTargetPreviewWidget::RenderTargetPreviewWidget(
    const std::string& title,
    const std::vector<uint8_t>& rgba,
    uint32_t width,
    uint32_t height)
    : m_Title(title), m_Rgba(rgba), m_Width(width), m_Height(height) {}

Size RenderTargetPreviewWidget::Measure(const Size& availableSize) {
    const float maxPreview = (std::min)(availableSize.width, availableSize.height) - kPadding * 2.0f - kHeader;
    m_DesiredSize = Size{ availableSize.width, availableSize.height };
    (void)maxPreview;
    return m_DesiredSize;
}

void RenderTargetPreviewWidget::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
    m_CloseRect = Rect{
        allottedRect.x + allottedRect.width - kPadding - kCloseSize,
        allottedRect.y + 4.0f,
        kCloseSize,
        kCloseSize
    };

    const float innerW = allottedRect.width - kPadding * 2.0f;
    const float innerH = allottedRect.height - kHeader - kPadding * 2.0f;
    float drawW = innerW;
    float drawH = innerH;
    if (m_Width > 0 && m_Height > 0) {
        const float aspect = static_cast<float>(m_Width) / static_cast<float>(m_Height);
        if (drawW / drawH > aspect) drawW = drawH * aspect;
        else drawH = drawW / aspect;
    }
    m_PreviewRect = Rect{
        allottedRect.x + (allottedRect.width - drawW) * 0.5f,
        allottedRect.y + kHeader + (innerH - drawH) * 0.5f,
        drawW,
        drawH
    };
}

void RenderTargetPreviewWidget::Paint(PaintContext& context) {
    const auto& theme = Theme::Get();
    context.DrawRect(m_Geometry, theme.PanelBackground, 4.0f);
    context.DrawText(m_Title, Point{ m_Geometry.x + kPadding, m_Geometry.y + 6.0f }, theme.TextPrimary, theme.TextSizeProperty);
    context.DrawRect(m_CloseRect, theme.SelectedAccent, 2.0f);

    if (m_Rgba.empty() || m_Width == 0 || m_Height == 0) {
        context.DrawText("No preview data (enable GPU readback)", Point{ m_PreviewRect.x, m_PreviewRect.y },
            theme.TextSecondary, theme.TextSizeProperty - 1.0f);
        return;
    }

    const float pxW = m_PreviewRect.width / static_cast<float>(m_Width);
    const float pxH = m_PreviewRect.height / static_cast<float>(m_Height);
    for (uint32_t y = 0; y < m_Height; ++y) {
        for (uint32_t x = 0; x < m_Width; ++x) {
            const size_t idx = (static_cast<size_t>(y) * m_Width + x) * 4;
            if (idx + 2 >= m_Rgba.size()) continue;
            Color c{};
            c.r = m_Rgba[idx] / 255.0f;
            c.g = m_Rgba[idx + 1] / 255.0f;
            c.b = m_Rgba[idx + 2] / 255.0f;
            c.a = 1.0f;
            context.DrawRect(Rect{
                m_PreviewRect.x + x * pxW,
                m_PreviewRect.y + y * pxH,
                (std::max)(pxW, 1.0f),
                (std::max)(pxH, 1.0f)
            }, c);
        }
    }
}

void RenderTargetPreviewWidget::OnMouseDown(const MouseEvent& event) {
    if (m_CloseRect.Contains(event.position) && m_OnClose) {
        m_OnClose();
    }
}

} // namespace we::UI
