#include "WindEffects/Editor/UI/Widgets/ScreenDebugOverlay.h"
#include "WindEffects/Editor/UI/Core/ScreenRecorder.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Theming/StyleRole.h"

using ::we::runtime::kindui::ColorToken;
using ::we::runtime::kindui::MetricToken;
using ::we::runtime::kindui::Point;

namespace we::editor::panels {

ScreenDebugOverlay::ScreenDebugOverlay() {
    SetVisible(services::ScreenRecorder::IsRecordingEnabled());
}

ScreenDebugOverlay::~ScreenDebugOverlay() = default;

void ScreenDebugOverlay::Tick(float deltaTime) {
    Widget::Tick(deltaTime);
    m_RefreshTimer += deltaTime;
    if (m_RefreshTimer < 1.0f) {
        return;
    }
    m_RefreshTimer = 0.0f;
    // Change-gated: an unchanged overlay must not request a global paint.
    // Display lines are quantized to whole units so idle jitter never counts.
    const std::vector<std::string> fresh =
        services::ScreenRecorder::Get().BuildOverlayLines();
    if (fresh != m_Lines) {
        m_Lines = fresh;
        InvalidatePaint();
    }
}

Size ScreenDebugOverlay::Measure(const Size& availableSize) {
    m_DesiredSize = Size{ 460.0f, 140.0f };
    (void)availableSize;
    return m_DesiredSize;
}

void ScreenDebugOverlay::Arrange(const Rect& allottedRect) {
    // Pin to top-left regardless of the allotted slot.
    m_Geometry = Rect{ allottedRect.x + 12.0f, allottedRect.y + 12.0f, 460.0f, 140.0f };
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

} // namespace we::editor::panels
