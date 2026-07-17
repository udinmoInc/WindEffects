#pragma once

#include "KindUI/Export.h"
#include "KindUI/Core/Widget.h"
#include "KindUI/Tokens/DesignToken.h"
#include "Text/Rich/RichTextParser.h"

#include <memory>
#include <string>

namespace we::runtime::kindui {

/// Renders markdown-ish rich text via Runtime/Text rich parser + DrawText.
class KINDUI_API RichTextView : public Widget {
public:
    explicit RichTextView(std::string source, TypographyToken role = TypographyToken::Body);

    void SetSource(std::string source);
    [[nodiscard]] const std::string& Source() const { return m_Source; }

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;

private:
    void Rebuild(const we::runtime::text::layout::TextStyle& base);

    std::string m_Source;
    TypographyToken m_Role = TypographyToken::Body;
    we::runtime::text::rich::RichDocument m_Document;
    std::unique_ptr<we::runtime::text::rich::IRichTextParser> m_Parser;
};

} // namespace we::runtime::kindui
