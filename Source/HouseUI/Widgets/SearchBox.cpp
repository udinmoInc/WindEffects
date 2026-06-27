#include "SearchBox.hpp"
#include "../Core/PaintContext.hpp"
#include "../Core/Theme.hpp"
#include <SDL3/SDL.h>
#include <algorithm>

namespace HouseEngine::UI {

SearchBox::SearchBox()
    : m_Style(WidgetStyle::TextBox())
{}

Size SearchBox::Measure(const Size& availableSize) {
    return Size{ 200.0f, m_Height };
}

void SearchBox::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
}

void SearchBox::Paint(PaintContext& context) {
    // Draw background
    Color bgColor = m_Style.background.color;
    if (IsFocused()) {
        bgColor = Theme::Get().HoverOverlay;
    }
    
    context.DrawRoundedRect(m_Geometry, bgColor, m_Style.background.cornerRadius);
    
    // Draw border
    Color borderColor = m_Style.border.color;
    if (IsFocused()) {
        borderColor = Theme::Get().SelectedAccent;
    }
    
    context.DrawRoundedRectOutline(m_Geometry, borderColor, m_Style.border.width, m_Style.background.cornerRadius);
    
    // Draw search icon
    float iconSize = 16.0f;
    float iconX = m_Geometry.x + 8.0f;
    float iconY = m_Geometry.y + (m_Height - iconSize) / 2.0f;
    
    Rect iconRect{ iconX, iconY, iconSize, iconSize };
    IconPainter::DrawIcon(context, Icons::Search, iconRect, Theme::Get().TextSecondary);
    
    // Draw text or placeholder
    Rect textRect = GetTextRect();
    
    if (m_Text.empty()) {
        context.DrawText(m_Placeholder, Point{ textRect.x, textRect.y }, Theme::Get().TextSecondary * 0.6f, m_Style.text.size);
    } else {
        context.DrawText(m_Text, Point{ textRect.x, textRect.y }, m_Style.text.color, m_Style.text.size);
        
        // Draw caret if focused
        if (IsFocused() && m_ShowCaret) {
            float caretX = textRect.x + context.GetTextWidth(m_Text.substr(0, m_CaretPosition), m_Style.text.size);
            float caretY = textRect.y;
            float caretHeight = m_Style.text.size;
            
            Rect caretRect{ caretX, caretY, 1.5f, caretHeight };
            context.DrawRect(caretRect, Theme::Get().TextPrimary);
        }
    }
    
    // Draw clear button if text exists
    if (!m_Text.empty()) {
        Rect clearRect = GetClearButtonRect();
        IconPainter::DrawIcon(context, Icons::X, clearRect, Theme::Get().TextSecondary);
    }
}

void SearchBox::OnMouseDown(const MouseEvent& event) {
    if (event.button == MouseButton::Left) {
        // Check if clicked on clear button
        if (!m_Text.empty()) {
            Rect clearRect = GetClearButtonRect();
            if (event.position.x >= clearRect.x && event.position.x <= clearRect.x + clearRect.width &&
                event.position.y >= clearRect.y && event.position.y <= clearRect.y + clearRect.height) {
                SetText("");
                return;
            }
        }
        
        // Set focus and move caret
        Rect textRect = GetTextRect();
        float clickX = event.position.x - textRect.x;
        
        // Find closest character position
        float minDist = FLT_MAX;
        size_t closestPos = 0;
        
        for (size_t i = 0; i <= m_Text.length(); ++i) {
            float charX = m_Style.text.size * 0.6f * i; // Approximate character width
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
    // Hover state handled by EventSystem
}

void SearchBox::OnKeyDown(const KeyEvent& event) {
    if (!IsFocused()) return;
    
    if (event.type == KeyEventType::KeyDown) {
        // Handle character input
        if (event.keycode >= 32 && event.keycode <= 126) {
            m_Text.insert(m_CaretPosition, 1, static_cast<char>(event.keycode));
            m_CaretPosition++;
            if (m_OnTextChanged) {
                m_OnTextChanged(m_Text);
            }
        }
        // Handle backspace
        else if (event.keycode == SDLK_BACKSPACE) {
            if (m_CaretPosition > 0) {
                m_Text.erase(m_CaretPosition - 1, 1);
                m_CaretPosition--;
                if (m_OnTextChanged) {
                    m_OnTextChanged(m_Text);
                }
            }
        }
        // Handle delete
        else if (event.keycode == SDLK_DELETE) {
            if (m_CaretPosition < m_Text.length()) {
                m_Text.erase(m_CaretPosition, 1);
                if (m_OnTextChanged) {
                    m_OnTextChanged(m_Text);
                }
            }
        }
        // Handle left arrow
        else if (event.keycode == SDLK_LEFT) {
            if (m_CaretPosition > 0) {
                m_CaretPosition--;
            }
        }
        // Handle right arrow
        else if (event.keycode == SDLK_RIGHT) {
            if (m_CaretPosition < m_Text.length()) {
                m_CaretPosition++;
            }
        }
        // Handle home
        else if (event.keycode == SDLK_HOME) {
            m_CaretPosition = 0;
        }
        // Handle end
        else if (event.keycode == SDLK_END) {
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
    float iconSize = 16.0f;
    float iconWidth = iconSize + 8.0f; // icon + padding
    float clearWidth = m_Text.empty() ? 0.0f : 24.0f;
    
    return Rect{
        m_Geometry.x + iconWidth,
        m_Geometry.y + (m_Height - m_Style.text.size) / 2.0f,
        m_Geometry.width - iconWidth - clearWidth - 8.0f,
        m_Style.text.size
    };
}

Rect SearchBox::GetClearButtonRect() const {
    float clearSize = 16.0f;
    return Rect{
        m_Geometry.x + m_Geometry.width - clearSize - 8.0f,
        m_Geometry.y + (m_Height - clearSize) / 2.0f,
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

} // namespace HouseEngine::UI
