#pragma once

#include "KindUI/Export.h"
#include "KindUI/Core/Widgets/DesignSystemControls.h"

#include <functional>
#include <string>

namespace we::runtime::kindui {

/// Secondary toolbar action — delegates to design-system SecondaryButton.
class KINDUI_API SecondaryToolbarButton : public SecondaryButton {
public:
    SecondaryToolbarButton(const std::string& label, const std::string& iconName = {});

    void SetOnClicked(std::function<void()> callback) { SecondaryButton::SetOnClicked(std::move(callback)); }
    void SetEnabled(bool enabled) { SecondaryButton::SetEnabled(enabled); }
    [[nodiscard]] bool IsEnabled() const { return SecondaryButton::IsEnabled(); }

private:
    std::string m_IconStorage;
};

} // namespace we::runtime::kindui
