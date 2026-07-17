#include "Platform/Platform.h"
#include "KindUI/Widgets/TextBox.h"
#include "KindUI/Core/ControlChrome.h"
#include "KindUI/Core/EventSystem.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Theming/ThemeAccess.h"
#include "KindUI/Theming/ThemeManager.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Theming/StyleRole.h"
#include "KindUI/Core/Animator.h"
#include "KindUI/Core/TextMetrics.h"
#include "Text/Unicode/Grapheme.h"

namespace we::runtime::kindui {

TextBox::TextBox(const std::string& initialText, std::function<void(const std::string&)> onTextChanged)
    : m_Session(we::runtime::text::editing::CreateTextEditSession(initialText))
    , m_OnTextChanged(std::move(onTextChanged))
{
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
    if (!m_Visible || !m_Session) {
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

    const auto& text = m_Session->Text();
    const auto sel = m_Session->Caret().Selection();
    if (!sel.Empty()) {
        const auto codepoints = we::runtime::text::unicode::DecodeUtf8(text);
        const size_t start = std::min(sel.Start(), codepoints.size());
        const size_t end = std::min(sel.End(), codepoints.size());
        const std::string before = we::runtime::text::unicode::EncodeUtf8(
            std::span(codepoints.data(), start));
        const std::string selected = we::runtime::text::unicode::EncodeUtf8(
            std::span(codepoints.data() + start, end - start));
        const float selX0 = textX + context.GetTextWidth(before, style.fontSize);
        const float selW = context.GetTextWidth(selected, style.fontSize);
        context.DrawRect(
            Rect{ selX0, textY, selW, style.fontSize },
            ResolveColor(ColorToken::SelectionHighlight));
    }

    context.DrawText(text, Point{ textX, textY }, style.foreground, style.fontSize);

    if (m_Session->HasComposition()) {
        const float compX = textX + context.GetTextWidth(text, style.fontSize);
        context.DrawText(
            std::string(m_Session->Composition()),
            Point{ compX, textY },
            ResolveColor(ColorToken::TextSecondary),
            style.fontSize);
    }

    if (m_Focused) {
        const size_t caret = m_Session->Caret().Offset();
        const auto codepoints = we::runtime::text::unicode::DecodeUtf8(text);
        const size_t clamped = std::min(caret, codepoints.size());
        const std::string before = we::runtime::text::unicode::EncodeUtf8(
            std::span(codepoints.data(), clamped));
        const float cursorX = textX + context.GetTextWidth(before, style.fontSize) + 2.0f;
        context.DrawLine(
            Point{ cursorX, textY },
            Point{ cursorX, textY + style.fontSize },
            ResolveColor(ColorToken::AccentPrimary),
            1.5f);
    }
}

void TextBox::OnMouseDown(const MouseEvent& event) {
    if (!m_Session) {
        return;
    }
    const ResolvedStyle style = ThemeManager::Get().Resolve(StyleRole::Input);
    const float pad = ResolveMetric(MetricToken::Space2);
    const float localX = event.position.x - (m_Geometry.x + pad);
    const auto& text = m_Session->Text();

    // Approximate hit test by scanning grapheme widths.
    const auto codepoints = we::runtime::text::unicode::DecodeUtf8(text);
    size_t hit = codepoints.size();
    float x = 0.0f;
    for (size_t i = 0; i <= codepoints.size(); ++i) {
        const std::string prefix = we::runtime::text::unicode::EncodeUtf8(
            std::span(codepoints.data(), i));
        // Use character count fallback through PaintContext unavailable here — TextMetrics.
        const float w = TextMetrics::MeasureWidth(prefix, style.fontSize);
        if (localX <= w) {
            hit = i;
            break;
        }
        x = w;
    }
    (void)x;

    ++m_ClickCount;
    if (m_ClickCount >= 3) {
        m_Session->SelectionEngine().SelectAll(codepoints.size());
        m_ClickCount = 0;
        m_Dragging = false;
    } else if (m_ClickCount == 2) {
        m_Session->SelectionEngine().SelectWordAt(codepoints, hit);
        m_Dragging = false;
    } else {
        m_Session->SelectionEngine().BeginDrag(hit);
        m_Dragging = true;
    }
}

void TextBox::OnMouseMove(const MouseEvent& event) {
    if (!m_Session || !m_Dragging) {
        return;
    }
    const ResolvedStyle style = ThemeManager::Get().Resolve(StyleRole::Input);
    const float pad = ResolveMetric(MetricToken::Space2);
    const float localX = event.position.x - (m_Geometry.x + pad);
    const auto codepoints = we::runtime::text::unicode::DecodeUtf8(m_Session->Text());
    size_t hit = codepoints.size();
    for (size_t i = 0; i <= codepoints.size(); ++i) {
        const std::string prefix = we::runtime::text::unicode::EncodeUtf8(
            std::span(codepoints.data(), i));
        if (localX <= TextMetrics::MeasureWidth(prefix, style.fontSize)) {
            hit = i;
            break;
        }
    }
    m_Session->SelectionEngine().DragTo(hit);
}

void TextBox::OnMouseUp(const MouseEvent&) {
    m_Dragging = false;
    if (m_Session) {
        m_Session->SelectionEngine().EndDrag();
    }
}

void TextBox::OnKeyDown(const KeyEvent& event) {
    if (!m_Focused || !m_Session) {
        return;
    }

    const auto codepoints = we::runtime::text::unicode::DecodeUtf8(m_Session->Text());
    const bool shift = event.shiftDown;
    const bool ctrl = event.ctrlDown;
    bool changed = false;

    if (ctrl && event.key == we::platform::KeyCode::A) {
        m_Session->SelectionEngine().SelectAll(codepoints.size());
        return;
    }
    if (ctrl && event.key == we::platform::KeyCode::C) {
        m_Session->Copy();
        return;
    }
    if (ctrl && event.key == we::platform::KeyCode::X) {
        m_Session->Cut();
        changed = true;
    } else if (ctrl && event.key == we::platform::KeyCode::V) {
        m_Session->Paste();
        changed = true;
    } else if (ctrl && event.key == we::platform::KeyCode::Z) {
        changed = m_Session->Undo();
    } else if (ctrl && event.key == we::platform::KeyCode::Y) {
        changed = m_Session->Redo();
    } else if (event.key == we::platform::KeyCode::Backspace) {
        m_Session->DeleteBackward();
        changed = true;
    } else if (event.key == we::platform::KeyCode::Delete) {
        m_Session->DeleteForward();
        changed = true;
    } else if (event.key == we::platform::KeyCode::Left) {
        if (ctrl) {
            m_Session->Caret().MoveWordLeft(codepoints, shift);
        } else {
            m_Session->Caret().MoveLeft(codepoints, shift);
        }
    } else if (event.key == we::platform::KeyCode::Right) {
        if (ctrl) {
            m_Session->Caret().MoveWordRight(codepoints, shift);
        } else {
            m_Session->Caret().MoveRight(codepoints, shift);
        }
    } else if (event.key == we::platform::KeyCode::Home) {
        m_Session->Caret().MoveHome(codepoints, shift);
    } else if (event.key == we::platform::KeyCode::End) {
        m_Session->Caret().MoveEnd(codepoints, shift);
    } else if (event.key == we::platform::KeyCode::Enter || event.key == we::platform::KeyCode::Escape) {
        m_Focused = false;
    } else if (event.key == we::platform::KeyCode::Space) {
        m_Session->Insert(" ");
        changed = true;
    } else {
        const char typedChar = KeyCodeToChar(event.key, event.shiftDown);
        if (typedChar != 0) {
            m_Session->Insert(std::string(1, typedChar));
            changed = true;
        }
    }

    if (changed && m_OnTextChanged) {
        m_OnTextChanged(m_Session->Text());
    }
}

void TextBox::OnTextInput(const std::string& utf8) {
    if (!m_Session || utf8.empty()) {
        return;
    }
    m_Session->Insert(utf8);
    if (m_OnTextChanged) {
        m_OnTextChanged(m_Session->Text());
    }
}

} // namespace we::runtime::kindui
