#include "KindUI/Widgets/Label.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Core/TextMetrics.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Theming/StyleRole.h"
#include "KindUI/Core/Style.h"
#include <sstream>
#include "KindUI/Theming/ThemeAccess.h"


namespace we::runtime::kindui {

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
        std::string line;
        while (std::getline(stream, line, '\n')) {
            m_WrappedLines.push_back(line);
        }
    }

    float maxWidth = 0.0f;
    for (const auto& line : m_WrappedLines) {
        const float lineWidth = TextMetrics::MeasureWidth(line, m_Style.size, m_Style.bold);
        if (lineWidth > maxWidth) {
            maxWidth = lineWidth;
        }
    }

    const float lineGap = ResolveMetric(MetricToken::Space1);
    float height = static_cast<float>(m_WrappedLines.size()) * (m_Style.size + lineGap);
    m_DesiredSize = Size{ maxWidth, height };
    return m_DesiredSize;
}

void Label::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
}

void Label::Paint(PaintContext& context) {
    if (!m_Visible) {
        return;
    }
    const float lineGap = ResolveMetric(MetricToken::Space1);
    float currentY = m_Geometry.y;
    for (const auto& line : m_WrappedLines) {
        context.DrawText(line, Point{ m_Geometry.x, currentY }, m_Style.color, m_Style.size, m_Style.bold, m_Style.italic);
        currentY += (m_Style.size + lineGap);
    }
}

} // namespace we::runtime::kindui
