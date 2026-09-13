// ==============================================================================
// WindEffects — KindUI — TextBox
// Public API surface for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "KindUI/Export.h"
#include "KindUI/Core/Widget.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Theming/ResolvedStyle.h"
#include "Text/Editing/TextEditing.h"
#include "Text/Core/Types.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace we::runtime::kindui {

class KINDUI_API TextBox : public Widget {
public:
    TextBox(const std::string& initialText = "", std::function<void(const std::string&)> onTextChanged = nullptr);
    ~TextBox() override = default;

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;
    void Tick(float deltaTime) override;

    void OnMouseDown(const MouseEvent& event) override;
    void OnMouseMove(const MouseEvent& event) override;
    void OnMouseUp(const MouseEvent& event) override;
    [[nodiscard]] bool IsFocusable() const override { return true; }

    void OnKeyDown(const KeyEvent& event) override;
    void OnTextInput(const std::string& utf8) override;

    void SetText(const std::string& text) {
        if (m_Session) {
            m_Session->SetText(text);
            InvalidateTextLayoutCache();
        }
    }
    [[nodiscard]] const std::string& GetText() const {
        static const std::string kEmpty;
        return m_Session ? m_Session->Text() : kEmpty;
    }

private:
    void InvalidateTextLayoutCache();
    void EnsureTextLayoutCache(float fontSize);

    std::unique_ptr<we::runtime::text::editing::ITextEditSession> m_Session;
    std::function<void(const std::string&)> m_OnTextChanged;
    float m_HoverAnim = 0.0f;
    float m_FocusAnim = 0.0f;
    int m_ClickCount = 0;
    bool m_Dragging = false;

    // Cache resolved style to avoid repeated theme lookups
    ResolvedStyle m_CachedStyle;
    bool m_StyleCacheValid = false;

    // Cached UTF-8 decode + cumulative glyph advances for caret/selection hit-testing and paint.
    std::vector<we::runtime::text::Codepoint> m_CachedCodepoints;
    std::vector<float> m_CachedAdvances; // width of prefix [0..i]
    std::string m_CachedTextSnapshot;
    float m_CachedAdvanceFontSize = -1.0f;
    bool m_TextLayoutCacheValid = false;
};

} // namespace we::runtime::kindui
