#pragma once

#include "MainFrame/Export.h"

#include "Core/Widget.h"
#include <memory>

namespace we::UI {

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

} // namespace we::UI
