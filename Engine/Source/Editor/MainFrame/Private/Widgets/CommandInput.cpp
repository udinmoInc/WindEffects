#include "Platform/Platform.h"
#include "Widgets/CommandInput.h"
#include "KindUI/Core/EventSystem.h"
#include "KindUI/Core/PaintContext.h"
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
namespace WindIcons = ::we::runtime::kindui::WindIcons;
using ::we::runtime::kindui::kWindIconNone;
namespace IconMetrics = ::we::runtime::kindui::IconMetrics;
namespace ControlChrome = ::we::runtime::kindui::ControlChrome;
using ::we::runtime::kindui::KeyCodeToChar;

CommandInput::CommandInput()
    : m_Height(we::runtime::kindui::ResolveMetric(MetricToken::SearchBoxHeight))
    , m_Width(we::runtime::kindui::ResolveMetric(MetricToken::PropertyLabelColumnWidth) * 2.0f)
{
}

Size CommandInput::Measure(const Size& availableSize) {
    const float width = m_FlatChrome
        ? availableSize.width
        : (m_Width > 0.0f ? m_Width : availableSize.width);
    float height = m_Height;
    if (m_FlatChrome && availableSize.height > 0.0f && availableSize.height < 1.0e8f) {
        height = std::min(height, availableSize.height);
    }
    m_DesiredSize = Size{ width, height };
    return m_DesiredSize;
}

void CommandInput::Arrange(const Rect& allottedRect) {
    const float h = std::min(m_Height, allottedRect.height);
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
        ControlChrome::PaintInputFrame(context, m_Geometry, state);
    }

    const float iconSize = 16.0f;
    const float padH = ThemeMetric(MetricToken::Space2);
    const float iconX = m_Geometry.x + padH;
    const float iconY = m_Geometry.y + (m_Geometry.height - iconSize) / 2.0f;
    IconPainter::Draw(context, kWindIconNone, Rect{ iconX, iconY, iconSize, iconSize });

    const float textX = iconX + iconSize + ThemeMetric(MetricToken::Space1);
    const float fontSize = ThemeMetric(MetricToken::TextSizeSmall);
    const float textY = m_Geometry.y + (m_Geometry.height - fontSize) / 2.0f;

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

    const float iconSize = 16.0f;
    const float padH = ThemeMetric(MetricToken::Space2);
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