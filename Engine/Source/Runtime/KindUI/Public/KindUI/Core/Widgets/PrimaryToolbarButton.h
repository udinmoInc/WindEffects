#pragma once

#include "KindUI/Export.h"
#include "KindUI/Core/Widgets/DesignSystemControls.h"

#include <functional>
#include <string>

namespace we::runtime::kindui {

/// Primary toolbar action — delegates to design-system PrimaryButton.
class KINDUI_API PrimaryToolbarButton : public PrimaryButton {
public:
    PrimaryToolbarButton(const std::string& label, const std::string& iconName = {});

    void SetOnClicked(std::function<void()> callback) { PrimaryButton::SetOnClicked(std::move(callback)); }
    void SetEnabled(bool enabled) { PrimaryButton::SetEnabled(enabled); }
    [[nodiscard]] bool IsEnabled() const { return PrimaryButton::IsEnabled(); }

private:
    std::string m_IconStorage;
};

} // namespace we::runtime::kindui
