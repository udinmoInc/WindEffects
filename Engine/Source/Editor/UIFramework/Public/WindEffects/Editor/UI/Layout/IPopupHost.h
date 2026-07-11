#pragma once

#include "WindEffects/Editor/UI/Export.h"
#include "Core/Widget.h"

#include <memory>
#include <vector>

namespace WindEffects::Editor::UI {

class UIFRAMEWORK_API IPopupHost {
public:
    virtual ~IPopupHost() = default;

    virtual void ShowPopup(const std::shared_ptr<Widget>& popup, const Point& position) = 0;
    virtual void ShowFullscreenPopup(const std::shared_ptr<Widget>& popup) = 0;
    virtual void CloseTopPopup() = 0;
    virtual void CloseAllPopups() = 0;
    virtual void ExecutePendingCallbacks() = 0;
    [[nodiscard]] virtual bool HasOpenPopups() const = 0;
    [[nodiscard]] virtual bool IsWidgetInPopup(const std::shared_ptr<Widget>& widget) const = 0;
};

} // namespace WindEffects::Editor::UI
