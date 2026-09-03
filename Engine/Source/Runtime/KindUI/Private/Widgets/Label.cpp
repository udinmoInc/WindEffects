#include "KindUI/Widgets/Label.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Core/TextMetrics.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Tokens/TypographySpec.h"
#include "KindUI/Core/Style.h"
#include "KindUI/Theming/ThemeAccess.h"
#include "Text/Layout/TextStyle.h"

#include <algorithm>
#include <cmath>
#include <sstream>

namespace we::runtime::kindui {
namespace {

std::string EllipsizeToWidth(
    const std::string& text,
    const float fontSize,
    const bool bold,
    const float maxWidth) {
    if (maxWidth <= 0.0f || text.empty()) {
        return text;
    }
    if (TextMetrics::MeasureWidth(text, fontSize, bold) <= maxWidth) {
        return text;
    }

    constexpr const char* kEllipsis = "...";
    std::string truncated = text;
    while (truncated.size() > 1
        && TextMetrics::MeasureWidth(truncated + kEllipsis, fontSize, bold) > maxWidth) {
        truncated.pop_back();
    }
    truncated += kEllipsis;
    return truncated;
}

} // namespace

Label::Label(const std::string& text, TypographyToken role)
    : m_Text(text)
    , m_Style(TextStyle::FromRole(role))
{
}

Label::Label(const std::string& text, const Color& color, float fontSize)
    : m_Text(text)
    , m_Style(TextStyle::Body())
{
    // Default ctor args (White / 14) map to theme Body role instead of hardcoded white.
    const bool useThemeColor =
        color.r >= 0.999f && color.g >= 0.999f && color.b >= 0.999f && color.a >= 0.999f;
    const bool useThemeSize = fontSize <= 14.001f && fontSize >= 13.999f;
    if (!useThemeColor) {
        m_Style.color = color;
    }
    if (!useThemeSize) {
        m_Style.size = fontSize;
    }
}

float Label::LineHeight() const
{
    const TypographySpec spec = ResolveTypography(m_Style.role);
    if (spec.lineHeightPx > 0.0f && spec.sizePx > 0.0f) {
        if (std::abs(m_Style.size - spec.sizePx) < 0.01f) {
            return spec.lineHeightPx;
        }
        return m_Style.size * (spec.lineHeightPx / spec.sizePx);
    }
    return m_Style.size * 1.25f;
}

Size Label::Measure(const Size& availableSize) {
    m_WrappedLines.clear();

    if (m_WrapText && availableSize.width > 0.0f) {
        std::istringstream words(m_Text);
        std::string word;
        std::string currentLine;

        while (words >> word) {
            const std::string candidate = currentLine.empty() ? word : currentLine + " " + word;
            if (currentLine.empty()
                || TextMetrics::MeasureWidth(candidate, m_Style.size, m_Style.bold) <= availableSize.width) {
                currentLine = candidate;
            } else {
                m_WrappedLines.push_back(currentLine);
                currentLine = word;
            }
        }
        if (!currentLine.empty()) {
            m_WrappedLines.push_back(currentLine);
        }
    } else {
        std::istringstream stream(m_Text);
        std::string rawLine;
        while (std::getline(stream, rawLine, '\n')) {
            m_WrappedLines.push_back(rawLine);
        }
        if (availableSize.width > 0.0f && !m_WrappedLines.empty()) {
            for (auto& wrapped : m_WrappedLines) {
                wrapped = EllipsizeToWidth(wrapped, m_Style.size, m_Style.bold, availableSize.width);
            }
        }
    }

    float maxWidth = 0.0f;
    for (const auto& line : m_WrappedLines) {
        const float lineWidth = TextMetrics::MeasureWidth(line, m_Style.size, m_Style.bold);
        if (lineWidth > maxWidth) {
            maxWidth = lineWidth;
        }
    }

    const float lineHeight = LineHeight();
    const float height = static_cast<float>(std::max<size_t>(m_WrappedLines.size(), 1)) * lineHeight;
    if (availableSize.width > 0.0f && !m_WrapText) {
        maxWidth = std::min(maxWidth, availableSize.width);
    }
    m_DesiredSize = Size{ maxWidth, height };
    return m_DesiredSize;
}

void Label::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
    if (!m_WrapText && allottedRect.width > 0.0f && !m_WrappedLines.empty()) {
        // Re-ellipsize against final arranged width so labels never overflow neighbors.
        std::istringstream stream(m_Text);
        std::string rawLine;
        m_WrappedLines.clear();
        while (std::getline(stream, rawLine, '\n')) {
            m_WrappedLines.push_back(
                EllipsizeToWidth(rawLine, m_Style.size, m_Style.bold, allottedRect.width));
        }
    }
}

void Label::Paint(PaintContext& context) {
    if (!m_Visible) {
        return;
    }
    const float lineHeight = LineHeight();
    const float contentH = static_cast<float>(std::max<size_t>(m_WrappedLines.size(), 1)) * lineHeight;
    float currentY = m_Geometry.y + std::max(0.0f, (m_Geometry.height - contentH) * 0.5f);

    context.PushClipRect(m_Geometry);
    for (const auto& line : m_WrappedLines) {
        context.DrawText(
            line,
            Point{ m_Geometry.x, currentY + (lineHeight - m_Style.size) * 0.5f },
            m_Style.color,
            m_Style.size,
            static_cast<we::runtime::text::layout::FontWeight>(
                m_Style.weight > 0 ? m_Style.weight : (m_Style.bold ? 600 : 400)),
            m_Style.italic);
        currentY += lineHeight;
    }
    context.PopClipRect();
}

} // namespace we::runtime::kindui
