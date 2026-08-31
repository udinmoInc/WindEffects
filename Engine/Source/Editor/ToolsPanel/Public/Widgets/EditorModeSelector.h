#pragma once

#include "ToolsPanel/Export.h"

#include "KindUI/Core/Widget.h"
#include "KindUI/Core/WindIcon.h"

#include <memory>
#include <string>
namespace we::programs::editor {

/// Compact toolbar control: shows active editor mode and opens a registry-driven mode menu.
class TOOLSPANEL_API EditorModeSelector : public we::runtime::kindui::Widget {
public:
    EditorModeSelector();
    ~EditorModeSelector() override;

    EditorModeSelector(const EditorModeSelector&) = delete;
    EditorModeSelector& operator=(const EditorModeSelector&) = delete;

    we::runtime::kindui::Size Measure(const we::runtime::kindui::Size& availableSize) override;
    void Arrange(const we::runtime::kindui::Rect& allottedRect) override;
    void Paint(we::runtime::kindui::PaintContext& context) override;

    void OnMouseDown(const we::runtime::kindui::MouseEvent& event) override;
    void OnMouseMove(const we::runtime::kindui::MouseEvent& event) override;

    void Refresh();

    void InitializeCallbacks(const std::shared_ptr<EditorModeSelector>& self);

private:
    void OpenModeMenu();

    bool m_Hovered = false;
    bool m_Pressed = false;
    float m_HoverAnim = 0.0f;
    std::string m_Label;
    we::runtime::kindui::WindIconRef m_Icon = we::runtime::kindui::kWindIconNone;
};

} // namespace we::programs::editor
