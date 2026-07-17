#include "Platform/Platform.h"
#include "Widgets/CommandInput.h"
#include "KindUI/Core/EventSystem.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Core/DPIContext.h"
#include "KindUI/Rendering/IconMetrics.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Theming/StyleRole.h"
#include "KindUI/Theming/ThemeAccess.h"
#include "KindUI/Core/Icon.h"
#include <algorithm>

using we::runtime::kindui::ColorToken;
using we::runtime::kindui::MetricToken;
using we::runtime::kindui::PaddingToken;

namespace we::runtime::kindui {

CommandInput::CommandInput() = default;

Size CommandInput::Measure(const Size& availableSize) {
    (void)availableSize;
    m_DesiredSize = Size{ m_Width, m_Height };
    return m_DesiredSize;
}

void CommandInput::Arrange(const Rect& allottedRect) {
    const float h = std::min(m_Height, allottedRect.height);
    const float y = allottedRect.y + (allottedRect.height - h) * 0.5f;
    m_Geometry = Rect{ allottedRect.x, y, allottedRect.width, h };
}

void CommandInput::Paint(PaintContext& context) {
    const float cornerRadius = 4.0f;
    context.DrawRoundedRect(m_Geometry, ThemeColor(ColorToken::InputBackground), cornerRadius);

    Color borderColor = ThemeColor(ColorToken::BorderDefault);
    if (IsFocused()) {
        borderColor = ThemeColor(ColorToken::AccentPrimary);
    }
    context.DrawRoundedRectOutline(m_Geometry, borderColor, 1.0f, cornerRadius);

    const float iconSize = static_cast<float>(IconMetrics::NativeIconTierPx(ThemeMetric(MetricToken::IconSizeSearch)));
    const float iconX = m_Geometry.x + 10.0f;
    const float iconY = m_Geometry.y + (m_Geometry.height - iconSize) / 2.0f;
    IconPainter::DrawIcon(context, Icons::TerminalName, Rect{ iconX, iconY, iconSize, iconSize }, ThemeColor(ColorToken::IconSecondary));

    const float textX = m_Geometry.x + 10.0f + iconSize + 8.0f;
    const float fontSize = 12.0f;
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

    const float textX = m_Geometry.x + 32.0f;
    const float clickX = std::max(0.0f, event.position.x - textX);
    size_t closestPos = 0;
    float minDist = FLT_MAX;
    for (size_t i = 0; i <= m_Text.length(); ++i) {
        const float charX = 7.0f * static_cast<float>(i);
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

} // namespace we::runtime::kindui
