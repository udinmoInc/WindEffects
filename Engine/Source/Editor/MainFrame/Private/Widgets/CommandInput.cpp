#include "Widgets/CommandInput.h"
#include "Core/PaintContext.h"
#include "WindEffects/Editor/UI/Theming/ThemeToken.h"
#include "Core/Icon.h"
#include <SDL3/SDL.h>
#include <algorithm>

namespace WindEffects::Editor::UI {

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
    context.DrawRoundedRect(m_Geometry, ThemeColor(ThemeToken::InputBackground), cornerRadius);

    Color borderColor = ThemeColor(ThemeToken::BorderDefault);
    if (IsFocused()) {
        borderColor = ThemeColor(ThemeToken::AccentPrimary);
    }
    context.DrawRoundedRectOutline(m_Geometry, borderColor, 1.0f, cornerRadius);

    const float iconSize = 14.0f;
    const float iconX = m_Geometry.x + 10.0f;
    const float iconY = m_Geometry.y + (m_Geometry.height - iconSize) / 2.0f;
    IconPainter::DrawIcon(context, Icons::ConsoleName, Rect{ iconX, iconY, iconSize, iconSize }, ThemeColor(ThemeToken::TextSecondary));

    const float textX = m_Geometry.x + 10.0f + iconSize + 8.0f;
    const float fontSize = 12.0f;
    const float textY = m_Geometry.y + (m_Geometry.height - fontSize) / 2.0f;

    if (m_Text.empty() && !IsFocused()) {
        context.DrawText(m_Placeholder, Point{ textX, textY }, ThemeColor(ThemeToken::SearchPlaceholder), fontSize);
        return;
    }

    context.DrawText(m_Text, Point{ textX, textY }, ThemeColor(ThemeToken::TextPrimary), fontSize);
    if (IsFocused() && m_ShowCaret) {
        const float caretX = textX + context.GetTextWidth(m_Text.substr(0, m_CaretPosition), fontSize);
        context.DrawRect(Rect{ caretX, textY, 1.5f, fontSize }, ThemeColor(ThemeToken::TextPrimary));
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

    if (event.keycode == SDLK_RETURN) {
        if (!m_Text.empty() && m_OnCommandSubmitted) {
            m_OnCommandSubmitted(m_Text);
        }
        m_Text.clear();
        m_CaretPosition = 0;
        return;
    }

    if (event.keycode == SDLK_ESCAPE) {
        m_Text.clear();
        m_CaretPosition = 0;
        return;
    }

    if (event.keycode == SDLK_BACKSPACE) {
        if (m_CaretPosition > 0) {
            m_Text.erase(m_CaretPosition - 1, 1);
            --m_CaretPosition;
        }
        return;
    }

    if (event.keycode == SDLK_LEFT && m_CaretPosition > 0) {
        --m_CaretPosition;
        return;
    }

    if (event.keycode == SDLK_RIGHT && m_CaretPosition < m_Text.length()) {
        ++m_CaretPosition;
        return;
    }

    if (event.keycode >= 32 && event.keycode <= 126 && m_Text.length() < 128) {
        m_Text.insert(m_CaretPosition, 1, static_cast<char>(event.keycode));
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

} // namespace WindEffects::Editor::UI
