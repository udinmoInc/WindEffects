#include "WindEffects/Editor/UI/Widgets/RenderTargetPreviewWidget.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Theming/StyleRole.h"
#include "KindUI/Theming/ThemeAccess.h"
#include "KindUI/Core/PaintContext.h"

#include <algorithm>

using ::we::runtime::kindui::PaddingToken;

using ::we::runtime::kindui::ColorToken;
using ::we::runtime::kindui::MetricToken;
using ::we::runtime::kindui::Point;
using ::we::runtime::kindui::Color;

namespace we::editor::viewport {

namespace {
float PreviewPadding() {
    return we::runtime::kindui::ResolveMetric(MetricToken::CardPadding);
}
float PreviewHeader() {
    return we::runtime::kindui::ResolveMetric(MetricToken::PanelToolbarHeight);
}
float PreviewCloseSize() {
    return we::runtime::kindui::ResolveMetric(MetricToken::ScrollbarThumbMinHeight);
}
} // namespace

RenderTargetPreviewWidget::RenderTargetPreviewWidget(
    const std::string& title,
    const std::vector<uint8_t>& rgba,
    uint32_t width,
    uint32_t height)
    : m_Title(title), m_Rgba(rgba), m_Width(width), m_Height(height) {}

Size RenderTargetPreviewWidget::Measure(const Size& availableSize) {
    const float padding = PreviewPadding();
    const float header = PreviewHeader();
    const float maxPreview = (std::min)(availableSize.width, availableSize.height) - padding * 2.0f - header;
    m_DesiredSize = Size{ availableSize.width, availableSize.height };
    (void)maxPreview;
    return m_DesiredSize;
}

void RenderTargetPreviewWidget::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
    const float padding = PreviewPadding();
    const float header = PreviewHeader();
    const float closeSize = PreviewCloseSize();
    m_CloseRect = Rect{
        allottedRect.x + allottedRect.width - padding - closeSize,
        allottedRect.y + we::runtime::kindui::ResolveMetric(MetricToken::Space1),
        closeSize,
        closeSize
    };

    const float innerW = allottedRect.width - padding * 2.0f;
    const float innerH = allottedRect.height - header - padding * 2.0f;
    float drawW = innerW;
    float drawH = innerH;
    if (m_Width > 0 && m_Height > 0) {
        const float aspect = static_cast<float>(m_Width) / static_cast<float>(m_Height);
        if (drawW / drawH > aspect) drawW = drawH * aspect;
        else drawH = drawW / aspect;
    }
    m_PreviewRect = Rect{
        allottedRect.x + (allottedRect.width - drawW) * 0.5f,
        allottedRect.y + header + (innerH - drawH) * 0.5f,
        drawW,
        drawH
    };
}

void RenderTargetPreviewWidget::Paint(PaintContext& context) {
    const float padding = PreviewPadding();
    context.DrawRect(m_Geometry, ThemeColor(ColorToken::PanelBackground), ThemeMetric(MetricToken::CornerRadiusSmall));
    context.DrawText(m_Title, Point{ m_Geometry.x + padding, m_Geometry.y + ThemeMetric(MetricToken::Space2) }, ThemeColor(ColorToken::TextPrimary), ThemeMetric(MetricToken::TextSizeProperty));
    context.DrawRect(m_CloseRect, ThemeColor(ColorToken::AccentPrimary), ThemeMetric(MetricToken::CornerRadiusSmall) * 0.5f);

    if (m_Rgba.empty() || m_Width == 0 || m_Height == 0) {
        context.DrawText("No preview data (enable GPU readback)", Point{ m_PreviewRect.x, m_PreviewRect.y },
            ThemeColor(ColorToken::TextSecondary), ThemeMetric(MetricToken::TextSizeProperty) - 1.0f);
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

} // namespace we::editor::viewport
