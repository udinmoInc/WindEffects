#pragma once

#include "WindEffects/Runtime/UI/Export.h"
#include "WindEffects/Runtime/UI/Core/Widgets/DesignSystemControls.h"

#include <functional>
#include <string>

namespace WindEffects::Editor::UI {

/// Primary toolbar action — delegates to design-system PrimaryButton.
class UI_API PrimaryToolbarButton : public PrimaryButton {
public:
    PrimaryToolbarButton(const std::string& label, const std::string& iconName = {});

    void SetOnClicked(std::function<void()> callback) { PrimaryButton::SetOnClicked(std::move(callback)); }
    void SetEnabled(bool enabled) { PrimaryButton::SetEnabled(enabled); }
    [[nodiscard]] bool IsEnabled() const { return PrimaryButton::IsEnabled(); }

private:
    std::string m_IconStorage;
};

} // namespace WindEffects::Editor::UI
