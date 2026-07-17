#pragma once

#include "Text/Export.h"
#include "Text/Layout/TextStyle.h"

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace we::runtime::text::rich {

enum class SpanKind : uint8_t {
    Plain,
    Bold,
    Italic,
    Code,
    Link,
    Heading,
};

struct TextSpan {
    SpanKind kind = SpanKind::Plain;
    std::string text;
    layout::TextStyle style{};
    std::string linkTarget;
};

struct RichDocument {
    std::vector<TextSpan> spans;
};

class TEXT_API IRichTextParser {
public:
    virtual ~IRichTextParser() = default;
    [[nodiscard]] virtual RichDocument Parse(
        std::string_view source,
        const layout::TextStyle& baseStyle) const = 0;
};

/// Lightweight markdown-ish parser: **bold**, *italic*, `code`, [link](url), # headings.
[[nodiscard]] TEXT_API std::unique_ptr<IRichTextParser> CreateMarkdownRichTextParser();

} // namespace we::runtime::text::rich
