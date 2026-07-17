#pragma once

#include "MainFrame/Export.h"

#include "KindUI/Core/Widget.h"
#include <memory>
#include "KindUI/Core/Style.h"

namespace we::editor::shell {
using ::we::runtime::kindui::Widget;
using ::we::runtime::kindui::Size;
using ::we::runtime::kindui::Rect;
using ::we::runtime::kindui::Point;
using ::we::runtime::kindui::Color;
using ::we::runtime::kindui::PaintContext;
using ::we::runtime::kindui::MouseEvent;
using ::we::runtime::kindui::WidgetStyle;


// Draws a square border around the full application client area.
class MAINFRAME_API WindowShell : public Widget {
public:
    WindowShell();
    ~WindowShell() override = default;

    void SetContent(const std::shared_ptr<Widget>& content);
    const std::shared_ptr<Widget>& GetContent() const { return m_Content; }

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;

private:
    std::shared_ptr<Widget> m_Content;
};

} // namespace we::editor::shell