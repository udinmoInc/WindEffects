#include "Widgets/SearchBox.hpp"
#include "Core/PaintContext.hpp"
#include "Core/Theme.hpp"
#include <SDL3/SDL.h>
#include <algorithm>

namespace we::UI {

namespace {
constexpr float kSearchHeight = 29.0f;
constexpr float kCornerRadius = 5.0f;
constexpr float kPadH = 10.0f;
constexpr float kFontSize = 13.0f;
} // namespace

SearchBox::SearchBox()
    : m_Style(WidgetStyle::TextBox())
{
    m_Height = kSearchHeight;
}

Size SearchBox::Measure(const Size& availableSize) {
    float w = m_FillWidth ? availableSize.width : m_Width;
    m_DesiredSize = Size{ w, m_Height };
    return m_DesiredSize;
}

void SearchBox::Arrange(const Rect& allottedRect) {
    const float h = std::min(m_Height, allottedRect.height);
    const float y = allottedRect.y + (allottedRect.height - h) * 0.5f;
    m_Geometry = Rect{ allottedRect.x, y, allottedRect.width, h };
}

void SearchBox::Paint(PaintContext& context) {
    const auto& theme = Theme::Get();
    const Color bg = theme.ToolbarBackground;
    context.DrawRoundedRect(m_Geometry, bg, kCornerRadius);

    Color borderColor = theme.BorderDefault;
    if (IsFocused()) {
        borderColor = Color::Lerp(theme.BorderDefault, theme.SelectedAccent, 0.35f);
    }
    context.DrawRoundedRectOutline(m_Geometry, borderColor, 1.0f, kCornerRadius);

    const float iconSize = 13.0f;
    const float iconX = m_Geometry.x + kPadH;
    const float iconY = m_Geometry.y + (m_Geometry.height - iconSize) * 0.5f;
    IconPainter::DrawIcon(context, Icons::SearchName, Rect{ iconX, iconY, iconSize, iconSize }, theme.SearchIcon);

    Rect textRect = GetTextRect();

    if (m_Text.empty()) {
        context.DrawText(m_Placeholder, Point{ textRect.x, textRect.y }, theme.SearchPlaceholder, kFontSize);
    } else {
        context.DrawText(m_Text, Point{ textRect.x, textRect.y }, theme.TextPrimary, kFontSize, false);

        if (IsFocused() && m_ShowCaret) {
            const float caretX = textRect.x + context.GetTextWidth(m_Text.substr(0, m_CaretPosition), kFontSize);
            Rect caretRect{ caretX, textRect.y, 1.0f, kFontSize };
            context.DrawRect(caretRect, theme.TextPrimary);
        }
    }

    if (!m_Text.empty()) {
        Rect clearRect = GetClearButtonRect();
        IconPainter::DrawIcon(context, Icons::XName, clearRect, theme.SearchIcon);
    }
}

void SearchBox::OnMouseDown(const MouseEvent& event) {
    if (event.button == MouseButton::Left) {
        if (!m_Text.empty()) {
            Rect clearRect = GetClearButtonRect();
            if (event.position.x >= clearRect.x && event.position.x <= clearRect.x + clearRect.width &&
                event.position.y >= clearRect.y && event.position.y <= clearRect.y + clearRect.height) {
                SetText("");
                return;
            }
        }

        Rect textRect = GetTextRect();
        float clickX = event.position.x - textRect.x;

        float minDist = FLT_MAX;
        size_t closestPos = 0;

        for (size_t i = 0; i <= m_Text.length(); ++i) {
            float charX = kFontSize * 0.58f * static_cast<float>(i);
            float dist = std::abs(clickX - charX);

            if (dist < minDist) {
                minDist = dist;
                closestPos = i;
            }
        }

        m_CaretPosition = closestPos;
    }
}

void SearchBox::OnMouseMove(const MouseEvent& event) {
    (void)event;
}

void SearchBox::OnKeyDown(const KeyEvent& event) {
    if (!IsFocused()) return;

    if (event.type == KeyEventType::KeyDown) {
        if (event.keycode >= 32 && event.keycode <= 126) {
            m_Text.insert(m_CaretPosition, 1, static_cast<char>(event.keycode));
            m_CaretPosition++;
            if (m_OnTextChanged) {
                m_OnTextChanged(m_Text);
            }
        } else if (event.keycode == SDLK_BACKSPACE) {
            if (m_CaretPosition > 0) {
                m_Text.erase(m_CaretPosition - 1, 1);
                m_CaretPosition--;
                if (m_OnTextChanged) {
                    m_OnTextChanged(m_Text);
                }
            }
        } else if (event.keycode == SDLK_DELETE) {
            if (m_CaretPosition < m_Text.length()) {
                m_Text.erase(m_CaretPosition, 1);
                if (m_OnTextChanged) {
                    m_OnTextChanged(m_Text);
                }
            }
        } else if (event.keycode == SDLK_LEFT) {
            if (m_CaretPosition > 0) {
                m_CaretPosition--;
            }
        } else if (event.keycode == SDLK_RIGHT) {
            if (m_CaretPosition < m_Text.length()) {
                m_CaretPosition++;
            }
        } else if (event.keycode == SDLK_HOME) {
            m_CaretPosition = 0;
        } else if (event.keycode == SDLK_END) {
            m_CaretPosition = m_Text.length();
        }
    }
}

void SearchBox::OnFocus() {
    Widget::OnFocus();
    m_ShowCaret = true;
    m_CaretBlinkTime = 0.0f;
}

void SearchBox::OnBlur() {
    Widget::OnBlur();
    m_ShowCaret = false;
}

void SearchBox::UpdateCaretBlink(float deltaTime) {
    if (IsFocused()) {
        m_CaretBlinkTime += deltaTime;
        if (m_CaretBlinkTime >= 0.5f) {
            m_CaretBlinkTime = 0.0f;
            m_ShowCaret = !m_ShowCaret;
        }
    }
}

Rect SearchBox::GetTextRect() const {
    const float iconWidth = kPadH + 13.0f + 7.0f;
    const float clearW = m_Text.empty() ? 0.0f : 24.0f;
    const float rightPad = kPadH;
    return Rect{
        m_Geometry.x + iconWidth,
        m_Geometry.y + (m_Geometry.height - kFontSize) * 0.5f,
        std::max(0.0f, m_Geometry.width - iconWidth - clearW - rightPad),
        kFontSize
    };
}

Rect SearchBox::GetClearButtonRect() const {
    const float clearSize = 14.0f;
    return Rect{
        m_Geometry.x + m_Geometry.width - clearSize - kPadH,
        m_Geometry.y + (m_Geometry.height - clearSize) * 0.5f,
        clearSize,
        clearSize
    };
}

void SearchBox::SetText(const std::string& text) {
    m_Text = text;
    m_CaretPosition = text.length();
    if (m_OnTextChanged) {
        m_OnTextChanged(m_Text);
    }
}

} // namespace we::UI
