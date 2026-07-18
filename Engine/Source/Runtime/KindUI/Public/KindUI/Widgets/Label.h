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

    void SetText(const std::string& text) { m_Text = text; }
    const std::string& GetText() const { return m_Text; }
    void SetStyle(const TextStyle& style) { m_Style = style; }
    const TextStyle& GetStyle() const { return m_Style; }
    void SetRole(TypographyToken role) { m_Style = TextStyle::FromRole(role); }
    void SetWrapText(bool wrap) { m_WrapText = wrap; }
    bool GetWrapText() const { return m_WrapText; }

private:
    [[nodiscard]] float LineHeight() const;

    std::string m_Text;
    TextStyle m_Style;
    bool m_WrapText = false;
    std::vector<std::string> m_WrappedLines;
};

} // namespace we::runtime::kindui
