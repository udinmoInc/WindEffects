#pragma once

#include "WindEffects/Editor/UI/Export.h"
#include "KindUI/Core/Widget.h"
#include "KindUI/Layout/ScrollLayout.h"
#include "KindUI/Widgets/Label.h"

#include "WindEffects/Editor/UI/Renderer/EditorRenderDebugStub.h"

namespace we::editor::panels {
using ::we::runtime::kindui::Widget;
using ::we::runtime::kindui::Size;
using ::we::runtime::kindui::Rect;
using ::we::runtime::kindui::PaintContext;
using ::we::runtime::kindui::Label;
using ::we::runtime::kindui::ScrollLayout;

class UIFRAMEWORK_API RenderDiagnosticsPanel : public Widget {
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

} // namespace we::editor::panels
