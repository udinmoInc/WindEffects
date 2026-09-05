#include "Platform/Platform.h"
#include "Widgets/CommandInput.h"
#include "KindUI/Core/EventSystem.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Core/LayoutMetrics.h"
#include "KindUI/Core/DPIContext.h"
#include "KindUI/Rendering/IconMetrics.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Theming/ThemeAccess.h"
#include "KindUI/Rendering/IconMetrics.h"
#include "KindUI/Core/ControlChrome.h"
#include "KindUI/Input/InputEvents.h"
#include "KindUI/Core/WindIcon.h"
#include "KindUI/Core/Icon.h"
#include <algorithm>

using ::we::runtime::kindui::ColorToken;
using ::we::runtime::kindui::MetricToken;
using ::we::runtime::kindui::PaddingToken;

namespace we::editor::shell {
using ::we::runtime::kindui::MouseButton;
using ::we::runtime::kindui::KeyEventType;
using ::we::runtime::kindui::IconPainter;
using ::we::runtime::kindui::DPIContext;
namespace WindIcons = ::we::runtime::kindui::WindIcons;
using ::we::runtime::kindui::kWindIconNone;
namespace IconMetrics = ::we::runtime::kindui::IconMetrics;
namespace LayoutMetrics = ::we::runtime::kindui::LayoutMetrics;
namespace ControlChrome = ::we::runtime::kindui::ControlChrome;
using ::we::runtime::kindui::KeyCodeToChar;

CommandInput::CommandInput()
    : m_Height(20.0f)
    , m_Width(200.0f)
{
}

Size CommandInput::Measure(const Size& availableSize) {
    (void)availableSize;
    const float uiScale = std::max(1.0f, DPIContext::GetScale());
    const float width = (m_Width > 0.0f ? m_Width : 200.0f) * uiScale;
    const float height = (m_Height > 0.0f ? m_Height : 20.0f) * uiScale;
    m_DesiredSize = Size{ width, height };
    return m_DesiredSize;
}

void CommandInput::Arrange(const Rect& allottedRect) {
    const float uiScale = std::max(1.0f, DPIContext::GetScale());
    const float h = std::min(m_Height > 0.0f ? m_Height * uiScale : 20.0f * uiScale, allottedRect.height);
    const float y = allottedRect.y + (allottedRect.height - h) * 0.5f;
    m_Geometry = Rect{ allottedRect.x, y, allottedRect.width, h };
}

void CommandInput::Paint(PaintContext& context) {
    ControlChrome::InteractionState state{};
    state.focused = IsFocused();
    state.hoverAnim = IsHovered() ? 1.0f : 0.0f;

    if (m_FlatChrome) {
        ControlChrome::PaintStatusBarCommandField(context, m_Geometry, state);
    } else {
        ControlChrome::PaintSearchField(
            context,
            m_Geometry,
            m_Placeholder,
            m_Text,
            state,
            IsFocused() && m_ShowCaret);
        return;
    }

    const float iconSize = ThemeMetric(MetricToken::IconSizeSearch);
    const float padH = ThemeMetric(MetricToken::SpaceMD);
    Rect iconBand{ m_Geometry.x + padH, m_Geometry.y, iconSize, m_Geometry.height };
    IconPainter::Draw(context, WindIcons::Console16, iconBand, static_cast<uint32_t>(iconSize));

    const float textX = m_Geometry.x + padH + iconSize + ThemeMetric(MetricToken::Space1);
    const float fontSize = ThemeMetric(MetricToken::TextSizeSmall);
    const float textY = LayoutMetrics::AlignTextTopY(m_Geometry, fontSize);

    if (m_Text.empty() && !IsFocused()) {
        context.DrawText(m_Placeholder, Point{ textX, textY }, ThemeColor(ColorToken::SearchPlaceholder), fontSize);
        return;
    }

    context.DrawText(m_Text, Point{ textX, textY }, ThemeColor(ColorToken::TextPrimary), fontSize);
    if (IsFocused() && m_ShowCaret) {
        const float caretX = textX + context.GetTextWidth(m_Text.substr(0, m_CaretPosition), fontSize);
        context.DrawRect(Rect{ caretX, textY, 1.5f, fontSize }, ThemeColor(ColorToken::TextPrimary));
    }
}

void CommandInput::OnMouseDown(const MouseEvent& event) {
    if (event.button != MouseButton::Left) {
        return;
    }

    const float iconSize = ThemeMetric(MetricToken::IconSizeSearch);
    const float padH = ThemeMetric(MetricToken::SpaceMD);
    const float textX = m_Geometry.x + padH + iconSize + ThemeMetric(MetricToken::Space1);
    const float clickX = std::max(0.0f, event.position.x - textX);
    const float charWidth = ThemeMetric(MetricToken::TextSizeSmall) * ThemeMetric(MetricToken::TextCharWidthRatio);
    size_t closestPos = 0;
    float minDist = FLT_MAX;
    for (size_t i = 0; i <= m_Text.length(); ++i) {
        const float charX = charWidth * static_cast<float>(i);
        const float dist = std::abs(clickX - charX);
        if (dist < minDist) {
            minDist = dist;
            closestPos = i;
        }
    }
    m_CaretPosition = closestPos;
}

void CommandInput::OnKeyDown(const KeyEvent& event) {
    if (!IsFocused() || event.type != KeyEventType::KeyDown) {
        return;
    }

    if (event.key == we::platform::KeyCode::Enter) {
        if (!m_Text.empty() && m_OnCommandSubmitted) {
            m_OnCommandSubmitted(m_Text);
        }
        m_Text.clear();
        m_CaretPosition = 0;
        return;
    }

    if (event.key == we::platform::KeyCode::Escape) {
        m_Text.clear();
        m_CaretPosition = 0;
        return;
    }

    if (event.key == we::platform::KeyCode::Backspace) {
        if (m_CaretPosition > 0) {
            m_Text.erase(m_CaretPosition - 1, 1);
            --m_CaretPosition;
        }
        return;
    }

    if (event.key == we::platform::KeyCode::Left && m_CaretPosition > 0) {
        --m_CaretPosition;
        return;
    }

    if (event.key == we::platform::KeyCode::Right && m_CaretPosition < m_Text.length()) {
        ++m_CaretPosition;
        return;
    }

    if (const char ch = KeyCodeToChar(event.key, event.shiftDown); ch != '\0' && m_Text.length() < 128) {
        m_Text.insert(m_CaretPosition, 1, ch);
        ++m_CaretPosition;
    }
}

void CommandInput::OnFocus() {
    Widget::OnFocus();
    m_ShowCaret = true;
}

void CommandInput::OnBlur() {
    Widget::OnBlur();
    m_ShowCaret = false;
}

} // namespace we::editor::shell