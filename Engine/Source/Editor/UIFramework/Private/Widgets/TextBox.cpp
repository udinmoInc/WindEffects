#include "Platform/Platform.h"
#include "Widgets/TextBox.h"
#include "Core/EventSystem.h"
#include "Core/PaintContext.h"
#include "WindEffects/Editor/UI/Theming/ThemeToken.h"
#include "Core/Style.h"
#include "Core/DPIContext.h"
#include "Core/Animator.h"

namespace WindEffects::Editor::UI {

TextBox::TextBox(const std::string& initialText, std::function<void(const std::string&)> onTextChanged)
    : m_Text(initialText)
    , m_OnTextChanged(onTextChanged)
    , m_Style(WidgetStyle::TextBox())
{}

Size TextBox::Measure(const Size& availableSize) {
    (void)availableSize;
    float height = m_Style.text.size + m_Style.padding.top + m_Style.padding.bottom;
    m_DesiredSize = Size{ 150.0f, height };
    return m_DesiredSize;
}

void TextBox::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
}

void TextBox::Paint(PaintContext& context) {
    if (!m_Visible) return;

    m_HoverAnim = Animator::Damp(m_HoverAnim, m_Hovered ? 1.0f : 0.0f, 15.0f);
    m_FocusAnim = Animator::Damp(m_FocusAnim, m_Focused ? 1.0f : 0.0f, 20.0f);

    // Interpolate background color
    Color bgColor = m_Style.background.color;
    if (m_HoverAnim > 0.01f) {
        bgColor = Color::Lerp(bgColor, ThemeColor(ThemeToken::HoverBackground), m_HoverAnim);
    }

    // Draw background
    context.DrawRoundedRect(m_Geometry, bgColor, m_Style.background.cornerRadius);

    // Draw border
    Color borderColor = m_Style.border.color;
    if (m_FocusAnim > 0.01f) {
        borderColor = Color::Lerp(borderColor, m_Style.borderFocused.color, m_FocusAnim);
    }
    context.DrawRoundedRectOutline(m_Geometry, borderColor, m_Style.border.width, m_Style.background.cornerRadius);

    // Draw text
    float textX = m_Geometry.x + m_Style.padding.left;
    float textY = m_Geometry.y + (m_Geometry.height - m_Style.text.size) * 0.5f;
    context.DrawText(m_Text, Point{ textX, textY }, m_Style.text.color, m_Style.text.size);

    // Draw cursor if focused
    if (m_Focused) {
        float cursorX = textX + m_Text.length() * m_Style.text.size * 0.6f + 2.0f;
        float cursorY = textY;
        context.DrawLine(Point{ cursorX, cursorY }, Point{ cursorX, cursorY + m_Style.text.size }, ThemeColor(ThemeToken::AccentPrimary), 1.5f);
    }
}

void TextBox::OnMouseDown(const MouseEvent& event) {
    (void)event;
    // Handled by EventSystem focus router
}

void TextBox::OnKeyDown(const KeyEvent& event) {
    if (!m_Focused) return;

    bool changed = false;

    if (event.key == we::platform::KeyCode::Backspace) {
        if (!m_Text.empty()) {
            m_Text.pop_back();
            changed = true;
        }
    } else if (event.key == we::platform::KeyCode::Enter || event.key == we::platform::KeyCode::Escape) {
        // Blur text box on commit/cancel
        m_Focused = false;
    } else if (event.key == we::platform::KeyCode::Space) {
        m_Text += ' ';
        changed = true;
    } else {
        const char typedChar = KeyCodeToChar(event.key, event.shiftDown);
        if (typedChar != 0 && m_Text.length() < 32) {
            m_Text += typedChar;
            changed = true;
        }
    }

    if (changed && m_OnTextChanged) {
        m_OnTextChanged(m_Text);
    }
}

} // namespace WindEffects::Editor::UI
