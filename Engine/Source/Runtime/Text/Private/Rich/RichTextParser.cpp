#include "Text/Rich/RichTextParser.h"

namespace we::runtime::text::rich {
namespace {

layout::TextStyle DeriveStyle(const layout::TextStyle& base, const SpanKind kind)
{
    layout::TextStyle style = base;
    switch (kind) {
    case SpanKind::Bold:
        style.weight = layout::FontWeight::SemiBold;
        break;
    case SpanKind::Italic:
        style.italic = true;
        break;
    case SpanKind::Code:
        style.family = "monospace";
        style.sizePx = base.sizePx * 0.95f;
        break;
    case SpanKind::Heading:
        style.weight = layout::FontWeight::Bold;
        style.sizePx = base.sizePx * 1.35f;
        break;
    case SpanKind::Link:
        style.color = {0.35f, 0.55f, 0.95f, 1.0f};
        break;
    default:
        break;
    }
    return style;
}

class MarkdownRichTextParser final : public IRichTextParser {
public:
    RichDocument Parse(const std::string_view source, const layout::TextStyle& baseStyle) const override
    {
        RichDocument doc;
        size_t i = 0;
        std::string plain;

        auto flushPlain = [&]() {
            if (plain.empty()) {
                return;
            }
            TextSpan span;
            span.kind = SpanKind::Plain;
            span.text = plain;
            span.style = baseStyle;
            doc.spans.push_back(std::move(span));
            plain.clear();
        };

        auto pushStyled = [&](SpanKind kind, std::string text, std::string link = {}) {
            flushPlain();
            TextSpan span;
            span.kind = kind;
            span.text = std::move(text);
            span.style = DeriveStyle(baseStyle, kind);
            span.linkTarget = std::move(link);
            doc.spans.push_back(std::move(span));
        };

        while (i < source.size()) {
            if (source[i] == '#' && (i == 0 || source[i - 1] == '\n')) {
                flushPlain();
                while (i < source.size() && source[i] == '#') {
                    ++i;
                }
                while (i < source.size() && source[i] == ' ') {
                    ++i;
                }
                size_t end = i;
                while (end < source.size() && source[end] != '\n') {
                    ++end;
                }
                pushStyled(SpanKind::Heading, std::string(source.substr(i, end - i)));
                i = end;
                continue;
            }
            if (i + 1 < source.size() && source[i] == '*' && source[i + 1] == '*') {
                const size_t close = source.find("**", i + 2);
                if (close != std::string_view::npos) {
                    pushStyled(SpanKind::Bold, std::string(source.substr(i + 2, close - (i + 2))));
                    i = close + 2;
                    continue;
                }
            }
            if (source[i] == '*') {
                const size_t close = source.find('*', i + 1);
                if (close != std::string_view::npos) {
                    pushStyled(SpanKind::Italic, std::string(source.substr(i + 1, close - (i + 1))));
                    i = close + 1;
                    continue;
                }
            }
            if (source[i] == '`') {
                const size_t close = source.find('`', i + 1);
                if (close != std::string_view::npos) {
                    pushStyled(SpanKind::Code, std::string(source.substr(i + 1, close - (i + 1))));
                    i = close + 1;
                    continue;
                }
            }
            if (source[i] == '[') {
                const size_t mid = source.find("](", i + 1);
                const size_t end = mid == std::string_view::npos
                    ? std::string_view::npos
                    : source.find(')', mid + 2);
                if (mid != std::string_view::npos && end != std::string_view::npos) {
                    pushStyled(
                        SpanKind::Link,
                        std::string(source.substr(i + 1, mid - (i + 1))),
                        std::string(source.substr(mid + 2, end - (mid + 2))));
                    i = end + 1;
                    continue;
                }
            }
            plain.push_back(source[i]);
            ++i;
        }
        flushPlain();
        return doc;
    }
};

} // namespace

std::unique_ptr<IRichTextParser> CreateMarkdownRichTextParser()
{
    return std::make_unique<MarkdownRichTextParser>();
}

} // namespace we::runtime::text::rich
