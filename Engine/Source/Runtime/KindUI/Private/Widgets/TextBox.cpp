// ==============================================================================
// WindEffects — KindUI — TextBox
// UI widget used by the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "Platform/Platform.h"
#include "KindUI/Widgets/TextBox.h"
#include "KindUI/Core/ControlChrome.h"
#include "KindUI/Core/EventSystem.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Theming/ThemeAccess.h"
#include "KindUI/Core/LayoutMetrics.h"
#include "KindUI/Theming/ThemeManager.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Theming/StyleRole.h"
#include "KindUI/Core/Animator.h"
#include "KindUI/Core/TextMetrics.h"
#include "KindUI/Input/InputEvents.h"
#include "Text/Unicode/Grapheme.h"

namespace we::runtime::kindui {

TextBox::TextBox(const std::string& initialText, std::function<void(const std::string&)> onTextChanged)
    : m_Session(we::runtime::text::editing::CreateTextEditSession(initialText))
    , m_OnTextChanged(std::move(onTextChanged))
    , m_StyleCacheValid(false)
{
    SetFocusable(true);
    LayoutMetrics::ApplyInputMinSize(*this);
}

void TextBox::InvalidateTextLayoutCache() {
    m_TextLayoutCacheValid = false;
    InvalidatePaint();
}

void TextBox::EnsureTextLayoutCache(float fontSize) {
    if (!m_Session) {
        return;
    }
    const auto& text = m_Session->Text();
    if (m_TextLayoutCacheValid &&
        m_CachedAdvanceFontSize == fontSize &&
        m_CachedTextSnapshot == text) {
        return;
    }

    m_CachedTextSnapshot = text;
    m_CachedAdvanceFontSize = fontSize;
    m_CachedCodepoints = we::runtime::text::unicode::DecodeUtf8(text);
    m_CachedAdvances.resize(m_CachedCodepoints.size() + 1);
    m_CachedAdvances[0] = 0.0f;
    for (size_t i = 0; i < m_CachedCodepoints.size(); ++i) {
        const std::string prefix = we::runtime::text::unicode::EncodeUtf8(
            std::span(m_CachedCodepoints.data(), i + 1));
        m_CachedAdvances[i + 1] = TextMetrics::MeasureWidth(prefix, fontSize);
    }
    m_TextLayoutCacheValid = true;
}

Size TextBox::Measure(const Size& availableSize) {
    if (!m_StyleCacheValid) {
        m_CachedStyle = ThemeManager::Get().Resolve(StyleRole::Input);
        m_StyleCacheValid = true;
    }
    const float minW = ResolveMetric(MetricToken::Space6) * 5.0f;
    const float w = availableSize.width < 1.0e8f ? availableSize.width : minW;
    m_DesiredSize = Size{
        w,
        m_CachedStyle.height > 0.0f ? m_CachedStyle.height : ResolveMetric(MetricToken::SearchBoxHeight)
    };
    return m_DesiredSize;
}

void TextBox::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
}

void TextBox::Tick(float deltaTime) {
    (void)deltaTime;
    const float targetHover = m_Hovered ? 1.0f : 0.0f;
    const float targetFocus = m_Focused ? 1.0f : 0.0f;
    m_HoverAnim = Animator::Damp(m_HoverAnim, targetHover, ControlChrome::HoverDamping());
    m_FocusAnim = Animator::Damp(m_FocusAnim, targetFocus, ControlChrome::HoverDamping());
    Widget::Tick(deltaTime);
}

void TextBox::Paint(PaintContext& context) {
    if (!m_Visible || !m_Session) {
        return;
    }

    ControlChrome::InteractionState state{ m_HoverAnim, 0.0f, false, m_Focused, false };
    ControlChrome::PaintInputFrame(context, m_Geometry, state);

    if (!m_StyleCacheValid) {
        m_CachedStyle = ThemeManager::Get().Resolve(StyleRole::Input);
        m_StyleCacheValid = true;
    }
    const float pad = ResolveMetric(MetricToken::Space2);
    const float textX = m_Geometry.x + pad;
    const float textY = LayoutMetrics::AlignTextTopY(m_Geometry, m_CachedStyle.fontSize);

    EnsureTextLayoutCache(m_CachedStyle.fontSize);
    const auto& text = m_Session->Text();
    const auto sel = m_Session->Caret().Selection();
    if (!sel.Empty()) {
        const size_t start = std::min(sel.Start(), m_CachedCodepoints.size());
        const size_t end = std::min(sel.End(), m_CachedCodepoints.size());
        const float selX0 = textX + m_CachedAdvances[start];
        const float selW = m_CachedAdvances[end] - m_CachedAdvances[start];
        context.DrawRect(
            Rect{ selX0, textY, selW, m_CachedStyle.fontSize },
            ResolveColor(ColorToken::SelectionHighlight));
    }

    context.DrawText(text, Point{ textX, textY }, m_CachedStyle.foreground, m_CachedStyle.fontSize);

    if (m_Session->HasComposition()) {
        const float compX = textX + m_CachedAdvances.back();
        context.DrawText(
            std::string(m_Session->Composition()),
            Point{ compX, textY },
            ResolveColor(ColorToken::TextSecondary),
            m_CachedStyle.fontSize);
    }

    if (m_Focused) {
        const size_t caret = std::min(m_Session->Caret().Offset(), m_CachedCodepoints.size());
        const float cursorX = textX + m_CachedAdvances[caret] + 2.0f;
        context.DrawLine(
            Point{ cursorX, textY },
            Point{ cursorX, textY + m_CachedStyle.fontSize },
            ResolveColor(ColorToken::AccentPrimary),
            1.5f);
    }
}

void TextBox::OnMouseDown(const MouseEvent& event) {
    if (!m_Session) {
        return;
    }
    if (!m_StyleCacheValid) {
        m_CachedStyle = ThemeManager::Get().Resolve(StyleRole::Input);
        m_StyleCacheValid = true;
    }
    EnsureTextLayoutCache(m_CachedStyle.fontSize);
    const float pad = ResolveMetric(MetricToken::Space2);
    const float localX = event.position.x - (m_Geometry.x + pad);

    size_t hit = m_CachedCodepoints.size();
    for (size_t i = 0; i < m_CachedAdvances.size(); ++i) {
        if (localX <= m_CachedAdvances[i]) {
            hit = i;
            break;
        }
    }

    ++m_ClickCount;
    if (m_ClickCount >= 3) {
        m_Session->SelectionEngine().SelectAll(m_CachedCodepoints.size());
        m_ClickCount = 0;
        m_Dragging = false;
    } else if (m_ClickCount == 2) {
        m_Session->SelectionEngine().SelectWordAt(m_CachedCodepoints, hit);
        m_Dragging = false;
    } else {
        m_Session->SelectionEngine().BeginDrag(hit);
        m_Dragging = true;
    }
    InvalidatePaint();
}

void TextBox::OnMouseMove(const MouseEvent& event) {
    if (!m_Session || !m_Dragging) {
        return;
    }
    if (!m_StyleCacheValid) {
        m_CachedStyle = ThemeManager::Get().Resolve(StyleRole::Input);
        m_StyleCacheValid = true;
    }
    EnsureTextLayoutCache(m_CachedStyle.fontSize);
    const float pad = ResolveMetric(MetricToken::Space2);
    const float localX = event.position.x - (m_Geometry.x + pad);
    size_t hit = m_CachedCodepoints.size();
    for (size_t i = 0; i < m_CachedAdvances.size(); ++i) {
        if (localX <= m_CachedAdvances[i]) {
            hit = i;
            break;
        }
    }
    m_Session->SelectionEngine().DragTo(hit);
    InvalidatePaint();
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

    EnsureTextLayoutCache(m_StyleCacheValid ? m_CachedStyle.fontSize : 13.0f);
    const auto& codepoints = m_CachedCodepoints;
    const bool shift = event.shiftDown;
    const bool ctrl = event.ctrlDown;
    bool changed = false;

    if (ctrl && event.key == we::platform::KeyCode::A) {
        m_Session->SelectionEngine().SelectAll(codepoints.size());
        InvalidatePaint();
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

    if (changed) {
        InvalidateTextLayoutCache();
        if (m_OnTextChanged) {
            m_OnTextChanged(m_Session->Text());
        }
    } else {
        InvalidatePaint();
    }
}

void TextBox::OnTextInput(const std::string& utf8) {
    if (!m_Session || utf8.empty()) {
        return;
    }
    m_Session->Insert(utf8);
    InvalidateTextLayoutCache();
    if (m_OnTextChanged) {
        m_OnTextChanged(m_Session->Text());
    }
}

} // namespace we::runtime::kindui
