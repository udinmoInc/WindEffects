#include "Widgets/WindowShell.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Theming/StyleRole.h"

using ::we::runtime::kindui::ColorToken;
using ::we::runtime::kindui::MetricToken;
using ::we::runtime::kindui::PaddingToken;

namespace we::editor::shell {

WindowShell::WindowShell() = default;

void WindowShell::SetContent(const std::shared_ptr<Widget>& content) {
    if (m_Content) {
        RemoveChild(m_Content);
    }
    m_Content = content;
    if (m_Content) {
        AddChild(m_Content);
    }
}

Size WindowShell::Measure(const Size& availableSize) {
    if (m_Content) {
        m_DesiredSize = m_Content->Measure(availableSize);
    } else {
        m_DesiredSize = availableSize;
    }
    return m_DesiredSize;
}

void WindowShell::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
    if (m_Content) {
        m_Content->Arrange(allottedRect);
    }
}

void WindowShell::Paint(PaintContext& context) {
    if (m_Content) {
        m_Content->Paint(context);
    }
}

} // namespace we::editor::shell