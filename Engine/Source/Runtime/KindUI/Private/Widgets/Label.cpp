#include "WindEffects/Runtime/UI/Widgets/Label.h"
#include "WindEffects/Runtime/UI/Core/PaintContext.h"
#include "WindEffects/Runtime/UI/Theming/ThemeToken.h"
#include "WindEffects/Runtime/UI/Core/Style.h"
#include <sstream>

namespace WindEffects::Editor::UI {

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
    float charWidth = m_Style.size * 0.55f;

    if (m_WrapText && availableSize.width > 0.0f) {
        int charsPerLine = static_cast<int>(availableSize.width / charWidth);
        if (charsPerLine < 1) {
            charsPerLine = 1;
        }

        std::istringstream words(m_Text);
        std::string word;
        std::string currentLine;

        while (words >> word) {
            if (currentLine.empty()) {
                currentLine = word;
            } else if ((currentLine.length() + 1 + word.length()) * charWidth <= availableSize.width) {
                currentLine += " " + word;
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
        float lineWidth = static_cast<float>(line.length()) * charWidth;
        if (lineWidth > maxWidth) {
            maxWidth = lineWidth;
        }
    }

    const float lineGap = ResolveThemeMetric(ThemeToken::Space1);
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
    const float lineGap = ResolveThemeMetric(ThemeToken::Space1);
    float currentY = m_Geometry.y;
    for (const auto& line : m_WrappedLines) {
        context.DrawText(line, Point{ m_Geometry.x, currentY }, m_Style.color, m_Style.size, m_Style.bold, m_Style.italic);
        currentY += (m_Style.size + lineGap);
    }
}

} // namespace WindEffects::Editor::UI
