#include "KindUI/Widgets/RichTextView.h"

#include "KindUI/Core/PaintContext.h"
#include "KindUI/Core/TextMetrics.h"
#include "KindUI/Theming/ThemeAccess.h"

#include <algorithm>

namespace we::runtime::kindui {

RichTextView::RichTextView(std::string source, const TypographyToken role)
    : m_Source(std::move(source))
    , m_Role(role)
    , m_Parser(we::runtime::text::rich::CreateMarkdownRichTextParser())
{
    we::runtime::text::layout::TextStyle base;
    base.sizePx = ResolveFontSize(m_Role);
    const auto color = ResolveColor(ColorToken::TextPrimary);
    base.color = {color.r, color.g, color.b, color.a};
    Rebuild(base);
}

void RichTextView::SetSource(std::string source) {
    m_Source = std::move(source);
    we::runtime::text::layout::TextStyle base;
    base.sizePx = ResolveFontSize(m_Role);
    const auto color = ResolveColor(ColorToken::TextPrimary);
    base.color = {color.r, color.g, color.b, color.a};
    Rebuild(base);
}

void RichTextView::Rebuild(const we::runtime::text::layout::TextStyle& base) {
    if (m_Parser) {
        m_Document = m_Parser->Parse(m_Source, base);
    }
}

Size RichTextView::Measure(const Size& availableSize) {
    float width = 0.0f;
    float height = 0.0f;
    float lineWidth = 0.0f;
    float lineHeight = ResolveFontSize(m_Role);

    for (const auto& span : m_Document.spans) {
        if (span.text == "\n" || span.kind == we::runtime::text::rich::SpanKind::Heading) {
            width = std::max(width, lineWidth);
            height += lineHeight + ResolveMetric(MetricToken::Space1);
            lineWidth = 0.0f;
            lineHeight = span.style.sizePx;
        }
        const float w = TextMetrics::MeasureWidth(
            span.text, span.style.sizePx, span.style.weight >= we::runtime::text::layout::FontWeight::SemiBold);
        lineWidth += w;
        lineHeight = std::max(lineHeight, span.style.sizePx);
        if (availableSize.width > 0.0f && lineWidth > availableSize.width) {
            width = std::max(width, availableSize.width);
            height += lineHeight + ResolveMetric(MetricToken::Space1);
            lineWidth = w;
        }
    }
    width = std::max(width, lineWidth);
    height += lineHeight;
    m_DesiredSize = Size{ width, height };
    return m_DesiredSize;
}

void RichTextView::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
}

void RichTextView::Paint(PaintContext& context) {
    if (!m_Visible) {
        return;
    }
    float x = m_Geometry.x;
    float y = m_Geometry.y;
    float lineHeight = ResolveFontSize(m_Role);

    for (const auto& span : m_Document.spans) {
        if (!span.text.empty() && span.text.back() == '\n') {
            std::string line = span.text;
            line.pop_back();
            if (!line.empty()) {
                const bool bold = span.style.weight >= we::runtime::text::layout::FontWeight::SemiBold;
                context.DrawText(
                    line,
                    Point{ x, y },
                    Color{ span.style.color.r, span.style.color.g, span.style.color.b, span.style.color.a },
                    span.style.sizePx,
                    bold,
                    span.style.italic);
            }
            y += std::max(lineHeight, span.style.sizePx) + ResolveMetric(MetricToken::Space1);
            x = m_Geometry.x;
            lineHeight = ResolveFontSize(m_Role);
            continue;
        }

        const bool bold = span.style.weight >= we::runtime::text::layout::FontWeight::SemiBold;
        context.DrawText(
            span.text,
            Point{ x, y },
            Color{ span.style.color.r, span.style.color.g, span.style.color.b, span.style.color.a },
            span.style.sizePx,
            bold,
            span.style.italic);

        if (span.style.underline || span.kind == we::runtime::text::rich::SpanKind::Link) {
            const float w = context.GetTextWidth(span.text, span.style.sizePx, bold);
            context.DrawLine(
                Point{ x, y + span.style.sizePx + 1.0f },
                Point{ x + w, y + span.style.sizePx + 1.0f },
                Color{ span.style.color.r, span.style.color.g, span.style.color.b, span.style.color.a },
                1.0f);
        }
        if (span.style.strikethrough) {
            const float w = context.GetTextWidth(span.text, span.style.sizePx, bold);
            context.DrawLine(
                Point{ x, y + span.style.sizePx * 0.55f },
                Point{ x + w, y + span.style.sizePx * 0.55f },
                Color{ span.style.color.r, span.style.color.g, span.style.color.b, span.style.color.a },
                1.0f);
        }

        x += context.GetTextWidth(span.text, span.style.sizePx, bold);
        lineHeight = std::max(lineHeight, span.style.sizePx);
    }
}

} // namespace we::runtime::kindui
