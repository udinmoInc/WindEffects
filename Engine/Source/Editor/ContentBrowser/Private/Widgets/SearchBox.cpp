#include "Platform/Platform.h"
#include "ContentBrowser/Widgets/SearchBox.h"
#include "KindUI/Core/EventSystem.h"
#include "KindUI/Core/PaintContext.h"
#include "WindEffects/Editor/UI/Panel/PanelChrome.h"
#include "KindUI/Core/LayoutMetrics.h"
#include "KindUI/Theming/ThemeAccess.h"
#include "KindUI/Core/DPIContext.h"
#include "KindUI/Rendering/IconMetrics.h"
#include "KindUI/Input/InputEvents.h"
#include <algorithm>

using ::we::runtime::kindui::ColorToken;
using ::we::runtime::kindui::MetricToken;
using ::we::runtime::kindui::PaddingToken;

namespace we::editor::widgets {
namespace LayoutMetrics = ::we::runtime::kindui::LayoutMetrics;
using ::we::runtime::kindui::MouseButton;
using ::we::runtime::kindui::KeyEventType;
using ::we::runtime::kindui::DPIContext;
using ::we::runtime::kindui::IconPainter;
namespace WindIcons = ::we::runtime::kindui::WindIcons;
using ::we::runtime::kindui::kWindIconNone;
namespace IconMetrics = ::we::runtime::kindui::IconMetrics;
using ::we::runtime::kindui::KeyCodeToChar;
namespace PanelChrome = ::we::editor::panels::PanelChrome;

SearchBox::SearchBox()
    : m_Style(WidgetStyle::TextBox())
{
    LayoutMetrics::ApplyInputMinSize(*this);
    SetFocusable(true);
}

Size SearchBox::Measure(const Size& availableSize) {
    float w = m_FillWidth ? availableSize.width : m_Width;
    const float uiScale = (std::max)(1.0f, DPIContext::GetScale());
    const float h = ThemeMetric(MetricToken::SearchBoxHeight) * uiScale;
    m_DesiredSize = Size{ w, h };
    return m_DesiredSize;
}

void SearchBox::Arrange(const Rect& allottedRect) {
    const float uiScale = (std::max)(1.0f, DPIContext::GetScale());
    const float h = std::min(ThemeMetric(MetricToken::SearchBoxHeight) * uiScale, allottedRect.height);
    m_Geometry = Rect{
        allottedRect.x,
        allottedRect.y + (allottedRect.height - h) * 0.5f,
        allottedRect.width,
        h
    };
}

void SearchBox::Paint(PaintContext& context) {
    PanelChrome::PaintSearchField(context, m_Geometry, m_Placeholder, m_Text, IsFocused(), m_ShowCaret);
}

void SearchBox::OnMouseDown(const MouseEvent& event) {
    if (event.button == MouseButton::Left) {
        if (!m_Text.empty()) {
            Rect clearRect = GetClearButtonRect();
            if (clearRect.Contains(event.position)) {
                SetText("");
                return;
            }
        }

        Rect textRect = GetTextRect();
        float clickX = event.position.x - textRect.x;
        const float fontSize = LayoutMetrics::SearchInputFontSize();

        float minDist = FLT_MAX;
        size_t closestPos = 0;

        for (size_t i = 0; i <= m_Text.length(); ++i) {
            float charX = fontSize * 0.58f * static_cast<float>(i);
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

void SearchBox::OnTextInput(const std::string& utf8) {
    if (!IsFocused() || utf8.empty()) {
        return;
    }

    m_Text.insert(m_CaretPosition, utf8);
    m_CaretPosition += utf8.size();
    if (m_OnTextChanged) {
        m_OnTextChanged(m_Text);
    }
    InvalidatePaint();
}

void SearchBox::OnKeyDown(const KeyEvent& event) {
    if (!IsFocused()) return;

    if (event.type == KeyEventType::KeyDown) {
        if (const char ch = KeyCodeToChar(event.key, event.shiftDown); ch != '\0') {
            m_Text.insert(m_CaretPosition, 1, ch);
            m_CaretPosition++;
            if (m_OnTextChanged) {
                m_OnTextChanged(m_Text);
            }
        } else if (event.key == we::platform::KeyCode::Backspace) {
            if (m_CaretPosition > 0) {
                m_Text.erase(m_CaretPosition - 1, 1);
                m_CaretPosition--;
                if (m_OnTextChanged) {
                    m_OnTextChanged(m_Text);
                }
            }
        } else if (event.key == we::platform::KeyCode::Delete) {
            if (m_CaretPosition < m_Text.length()) {
                m_Text.erase(m_CaretPosition, 1);
                if (m_OnTextChanged) {
                    m_OnTextChanged(m_Text);
                }
            }
        } else if (event.key == we::platform::KeyCode::Left) {
            if (m_CaretPosition > 0) {
                m_CaretPosition--;
            }
        } else if (event.key == we::platform::KeyCode::Right) {
            if (m_CaretPosition < m_Text.length()) {
                m_CaretPosition++;
            }
        } else if (event.key == we::platform::KeyCode::Home) {
            m_CaretPosition = 0;
        } else if (event.key == we::platform::KeyCode::End) {
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
    const float padH = LayoutMetrics::SearchInputPaddingH();
    const float iconSize = LayoutMetrics::SearchInputIconSize();
    const float fontSize = LayoutMetrics::SearchInputFontSize();
    const float iconWidth = padH + iconSize + ThemeMetric(MetricToken::Space1);
    const float clearW = m_Text.empty() ? 0.0f : (iconSize + padH);
    return Rect{
        m_Geometry.x + iconWidth,
        LayoutMetrics::AlignTextTopY(m_Geometry, fontSize),
        std::max(0.0f, m_Geometry.width - iconWidth - clearW),
        fontSize
    };
}

Rect SearchBox::GetClearButtonRect() const {
    const float clearSize = LayoutMetrics::SearchInputIconSize();
    const float padH = LayoutMetrics::SearchInputPaddingH();
    return Rect{
        m_Geometry.x + m_Geometry.width - clearSize - padH,
        m_Geometry.y + (m_Geometry.height - clearSize) * 0.5f,
        clearSize,
        clearSize
    };
}

void SearchBox::Tick(float deltaTime) {
    UpdateCaretBlink(deltaTime);
}

void SearchBox::SetText(const std::string& text) {
    m_Text = text;
    m_CaretPosition = text.length();
    if (m_OnTextChanged) {
        m_OnTextChanged(m_Text);
    }
}

} // namespace we::editor::widgets