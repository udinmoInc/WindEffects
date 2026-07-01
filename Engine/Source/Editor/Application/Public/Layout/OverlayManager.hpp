#pragma once

#include "Core/Widget.hpp"
#include <memory>
#include <vector>

namespace we::UI {

class OverlayManager : public Widget {
public:
    OverlayManager();
    virtual ~OverlayManager();

    // Singleton access for easy global popups
    static OverlayManager* Get();

    // Set the base root widget (e.g. Editor Layout)
    void SetBaseWidget(const std::shared_ptr<Widget>& baseWidget);

    // Show a floating popup at the specified position
    void ShowPopup(const std::shared_ptr<Widget>& popup, const Point& position);
    
    // Close the most recent popup, or clear all
    void CloseTopPopup();
    void CloseAllPopups();

    bool HasOpenPopups() const { return !m_Popups.empty(); }
    bool IsWidgetInPopup(const std::shared_ptr<Widget>& widget) const;

    // Overrides
    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;
    void OnMouseDown(const MouseEvent& event) override;

private:
    std::shared_ptr<Widget> m_BaseWidget;
    std::vector<std::shared_ptr<Widget>> m_Popups;
    
    static OverlayManager* s_Instance;
};

} // namespace we::editor::application::UI
