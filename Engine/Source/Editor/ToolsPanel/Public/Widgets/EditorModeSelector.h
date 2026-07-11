#pragma once

#include "ToolsPanel/Export.h"

#include "Core/Widget.h"
#include <string>

namespace we::programs::editor {

/// Compact toolbar control: shows active editor mode and opens a registry-driven mode menu.
class TOOLSPANEL_API EditorModeSelector : public WindEffects::Editor::UI::Widget {
public:
    EditorModeSelector();

    WindEffects::Editor::UI::Size Measure(const WindEffects::Editor::UI::Size& availableSize) override;
    void Arrange(const WindEffects::Editor::UI::Rect& allottedRect) override;
    void Paint(WindEffects::Editor::UI::PaintContext& context) override;

    void OnMouseDown(const WindEffects::Editor::UI::MouseEvent& event) override;
    void OnMouseMove(const WindEffects::Editor::UI::MouseEvent& event) override;

    void Refresh();

private:
    void OpenModeMenu();

    bool m_Hovered = false;
    bool m_Pressed = false;
    float m_HoverAnim = 0.0f;
    std::string m_Label;
    std::string m_IconName = "cursor";
};

} // namespace we::programs::editor
