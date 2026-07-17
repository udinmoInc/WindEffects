#include "Platform/Platform.h"
#include "KindUI/Widgets/TextBox.h"
#include "KindUI/Core/ControlChrome.h"
#include "KindUI/Core/EventSystem.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Core/TextMetrics.h"
#include "KindUI/Theming/ThemeAccess.h"
#include "KindUI/Theming/ThemeManager.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Theming/StyleRole.h"
#include "KindUI/Core/Animator.h"


namespace we::runtime::kindui {

TextBox::TextBox(const std::string& initialText, std::function<void(const std::string&)> onTextChanged)
    : m_Text(initialText)
    , m_OnTextChanged(onTextChanged) {
    SetFocusable(true);
}

Size TextBox::Measure(const Size& availableSize) {
    (void)availableSize;
    const ResolvedStyle role = ThemeManager::Get().Resolve(StyleRole::Input);
    m_DesiredSize = Size{
        ResolveMetric(MetricToken::Space6) * 5.0f,
        role.height > 0.0f ? role.height : ResolveMetric(MetricToken::SearchBoxHeight)
    };
    return m_DesiredSize;
}

void TextBox::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
}

void TextBox::Paint(PaintContext& context) {
    if (!m_Visible) {
        return;
    }

    m_HoverAnim = Animator::Damp(m_HoverAnim, m_Hovered ? 1.0f : 0.0f, ControlChrome::HoverDamping());
    m_FocusAnim = Animator::Damp(m_FocusAnim, m_Focused ? 1.0f : 0.0f, ControlChrome::HoverDamping());

    ControlChrome::InteractionState state{ m_HoverAnim, 0.0f, false, m_Focused, false };
    ControlChrome::PaintInputFrame(context, m_Geometry, state);

    const ResolvedStyle style = ThemeManager::Get().Resolve(StyleRole::Input);
    const float pad = ResolveMetric(MetricToken::Space2);
    const float textX = m_Geometry.x + pad;
    const float textY = m_Geometry.y + (m_Geometry.height - style.fontSize) * 0.5f;
    context.DrawText(m_Text, Point{ textX, textY }, style.foreground, style.fontSize);

    if (m_Focused) {
        const float cursorX = textX + TextMetrics::EstimateWidth(m_Text, style.fontSize) + 2.0f;
        context.DrawLine(
            Point{ cursorX, textY },
            Point{ cursorX, textY + style.fontSize },
            ResolveColor(ColorToken::AccentPrimary),
            1.5f);
    }
}

void TextBox::OnMouseDown(const MouseEvent& event) {
    (void)event;
}

void TextBox::OnKeyDown(const KeyEvent& event) {
    if (!m_Focused) {
        return;
    }

    bool changed = false;

    if (event.key == we::platform::KeyCode::Backspace) {
        if (!m_Text.empty()) {
            m_Text.pop_back();
            changed = true;
        }
    } else if (event.key == we::platform::KeyCode::Enter || event.key == we::platform::KeyCode::Escape) {
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

} // namespace we::runtime::kindui