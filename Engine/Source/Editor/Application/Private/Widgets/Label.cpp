#include "Label.hpp"
#include "../Core/PaintContext.hpp"
#include "../Core/Theme.hpp"
#include "../Core/Style.hpp"
#include "../Core/DPIContext.hpp"

namespace we::UI {

Label::Label(const std::string& text, const Color& color, float fontSize)
    : m_Text(text)
    , m_Style(TextStyle::Body())
{
    m_Style.color = color;
    m_Style.size = fontSize;
}

Size Label::Measure(const Size& availableSize) {
    (void)availableSize;
    float width = static_cast<float>(m_Text.length() * (m_Style.size * 0.6f));
    float height = m_Style.size + 4.0f;
    m_DesiredSize = Size{ width, height };
    return m_DesiredSize;
}

void Label::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
}

void Label::Paint(PaintContext& context) {
    if (!m_Visible) return;
    context.DrawText(m_Text, Point{ m_Geometry.x, m_Geometry.y }, m_Style.color, m_Style.size, m_Style.bold, m_Style.italic);
}

} // namespace we::editor::application::UI
