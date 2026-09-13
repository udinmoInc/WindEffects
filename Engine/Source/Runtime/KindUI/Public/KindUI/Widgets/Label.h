// ==============================================================================
// WindEffects — KindUI — Label
// Public API surface for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "KindUI/Export.h"

#include "KindUI/Core/Widget.h"
#include "KindUI/Core/Style.h"
#include "KindUI/Tokens/DesignToken.h"

namespace we::runtime::kindui {

class KINDUI_API Label : public Widget {
public:
    /// Prefer semantic roles — concrete size/weight/color come from the active theme.
    explicit Label(const std::string& text, TypographyToken role = TypographyToken::Body);
    /// Legacy overload: White/14 defaults map to Body; other values override theme.
    Label(const std::string& text, const Color& color, float fontSize = 14.0f);
    virtual ~Label() = default;

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;

    void SetText(const std::string& text) {
        if (m_Text == text) {
            return;
        }
        m_Text = text;
        m_LinesCacheValid = false;
        InvalidateLayout();
        InvalidatePaint();
    }
    const std::string& GetText() const { return m_Text; }
    void SetStyle(const TextStyle& style) {
        m_Style = style;
        m_LinesCacheValid = false;
        InvalidateLayout();
        InvalidatePaint();
    }
    const TextStyle& GetStyle() const { return m_Style; }
    void SetRole(TypographyToken role) {
        m_Style = TextStyle::FromRole(role);
        m_LinesCacheValid = false;
        InvalidateLayout();
        InvalidatePaint();
    }
    void SetWrapText(bool wrap) {
        if (m_WrapText == wrap) {
            return;
        }
        m_WrapText = wrap;
        m_LinesCacheValid = false;
        InvalidateLayout();
    }
    bool GetWrapText() const { return m_WrapText; }

private:
    [[nodiscard]] float LineHeight() const;
    void RebuildLines(float availableWidth);

    std::string m_Text;
    TextStyle m_Style;
    bool m_WrapText = false;
    std::vector<std::string> m_WrappedLines;
    float m_CachedLayoutWidth = -1.0f;
    bool m_LinesCacheValid = false;
};

} // namespace we::runtime::kindui
