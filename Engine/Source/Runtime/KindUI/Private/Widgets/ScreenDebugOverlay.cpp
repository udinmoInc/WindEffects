#include "KindUI/Widgets/ScreenDebugOverlay.h"
#include "KindUI/Profiling/ScreenRecorder.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Theming/StyleRole.h"

using ::we::runtime::kindui::ColorToken;
using ::we::runtime::kindui::MetricToken;
using ::we::runtime::kindui::Point;

namespace we::runtime::kindui {

ScreenDebugOverlay::ScreenDebugOverlay() {
    SetVisible(ScreenRecorder::IsRecordingEnabled());
}

void ScreenDebugOverlay::Tick(float deltaTime) {
    Widget::Tick(deltaTime);
    m_RefreshTimer += deltaTime;
    if (m_RefreshTimer < 0.25f) {
        return;
    }
    m_RefreshTimer = 0.0f;
    const std::vector<std::string> fresh =
        ScreenRecorder::Get().BuildOverlayLines();
    if (fresh != m_Lines) {
        m_Lines = fresh;
        InvalidatePaint();
    }
}

Size ScreenDebugOverlay::Measure(const Size& availableSize) {
    m_DesiredSize = Size{ 460.0f, 165.0f };
    (void)availableSize;
    return m_DesiredSize;
}

void ScreenDebugOverlay::Arrange(const Rect& allottedRect) {
    m_Geometry = Rect{ allottedRect.x + 12.0f, allottedRect.y + 12.0f, 460.0f, 165.0f };
}

void ScreenDebugOverlay::Paint(PaintContext& context) {
    if (!m_Visible || m_Lines.empty()) return;

    const float fontSize = ThemeMetric(MetricToken::TextSizeCaption);
    const float lineHeight = fontSize + 6.0f;
    const Rect box{ m_Geometry.x, m_Geometry.y, m_Geometry.width,
        lineHeight * static_cast<float>(m_Lines.size()) + 16.0f };
    context.DrawRoundedRect(box, ThemeColor(ColorToken::TooltipBackground), 6.0f);
    context.DrawRoundedRectOutline(box, ThemeColor(ColorToken::BorderDefault), 1.0f, 6.0f);

    float y = box.y + 8.0f;
    for (const auto& line : m_Lines) {
        context.DrawText(line, Point{ box.x + 10.0f, y },
            ThemeColor(ColorToken::TextPrimary), fontSize);
        y += lineHeight;
    }
}

} // namespace we::runtime::kindui
