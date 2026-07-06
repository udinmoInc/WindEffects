#pragma once

#include "Application/Export.hpp"
#include "Core/Widget.hpp"
#include "Layout/ScrollLayout.hpp"
#include "Widgets/Label.hpp"

namespace we::runtime::renderer {
struct ForensicFrameReport;
}

namespace we::UI {

class APPLICATION_API RenderDiagnosticsPanel : public Widget {
public:
    RenderDiagnosticsPanel();
    ~RenderDiagnosticsPanel() override = default;

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;

    void UpdateFromReport(const we::runtime::renderer::ForensicFrameReport& report);

    // Must be called after the widget is owned by std::shared_ptr.
    void Construct();

private:
    void RebuildText(const we::runtime::renderer::ForensicFrameReport& report);

    std::shared_ptr<ScrollLayout> m_Scroll;
    std::shared_ptr<Label> m_ContentLabel;
    std::string m_DisplayText;
};

} // namespace we::UI
