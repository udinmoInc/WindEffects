#pragma once

#include "WindEffects/Editor/UI/Export.h"

#include "Core/Widget.h"
#include "WindEffects/Editor/UI/Layout/IPopupHost.h"

#include <memory>
#include <vector>

namespace WindEffects::Editor::UI {

class UIFRAMEWORK_API OverlayHost final : public Widget, public IPopupHost {
public:
    OverlayHost();
    ~OverlayHost() override;

    void SetBaseWidget(const std::shared_ptr<Widget>& baseWidget);

    void ShowPopup(const std::shared_ptr<Widget>& popup, const Point& position) override;
    void ShowFullscreenPopup(const std::shared_ptr<Widget>& popup) override;
    void CloseTopPopup() override;
    void CloseAllPopups() override;
    void ExecutePendingCallbacks() override;
    [[nodiscard]] bool HasOpenPopups() const override { return !m_Popups.empty(); }
    [[nodiscard]] bool IsWidgetInPopup(const std::shared_ptr<Widget>& widget) const override;

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;
    void OnMouseDown(const MouseEvent& event) override;

private:
    std::shared_ptr<Widget> m_BaseWidget;
    std::vector<std::shared_ptr<Widget>> m_Popups;
    std::vector<bool> m_FullscreenPopups;
};

using OverlayManager = OverlayHost;

} // namespace WindEffects::Editor::UI
