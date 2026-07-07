#pragma once

#include "ToolsPanel/Export.h"

#include "Core/Widget.h"
#include <string>

namespace we::programs::editor {

/// Compact toolbar control: shows active editor mode and opens a registry-driven mode menu.
class TOOLSPANEL_API EditorModeSelector : public we::UI::Widget {
public:
    EditorModeSelector();

    we::UI::Size Measure(const we::UI::Size& availableSize) override;
    void Arrange(const we::UI::Rect& allottedRect) override;
    void Paint(we::UI::PaintContext& context) override;

    void OnMouseDown(const we::UI::MouseEvent& event) override;
    void OnMouseMove(const we::UI::MouseEvent& event) override;

    void Refresh();

private:
    void OpenModeMenu();

    bool m_Hovered = false;
    bool m_Pressed = false;
    std::string m_Label;
    std::string m_IconName = "cursor";
};

} // namespace we::programs::editor
