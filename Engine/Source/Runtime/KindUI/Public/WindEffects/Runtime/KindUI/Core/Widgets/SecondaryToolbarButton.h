#pragma once

#include "WindEffects/Runtime/UI/Export.h"
#include "WindEffects/Runtime/UI/Core/Widgets/DesignSystemControls.h"

#include <functional>
#include <string>

namespace WindEffects::Editor::UI {

/// Secondary toolbar action — delegates to design-system SecondaryButton.
class UI_API SecondaryToolbarButton : public SecondaryButton {
public:
    SecondaryToolbarButton(const std::string& label, const std::string& iconName = {});

    void SetOnClicked(std::function<void()> callback) { SecondaryButton::SetOnClicked(std::move(callback)); }
    void SetEnabled(bool enabled) { SecondaryButton::SetEnabled(enabled); }
    [[nodiscard]] bool IsEnabled() const { return SecondaryButton::IsEnabled(); }

private:
    std::string m_IconStorage;
};

} // namespace WindEffects::Editor::UI
