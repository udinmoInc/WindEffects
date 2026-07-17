#include "KindUI/Core/Widgets/PrimaryToolbarButton.h"

namespace we::runtime::kindui {

PrimaryToolbarButton::PrimaryToolbarButton(const std::string& label, const std::string& iconName)
    : PrimaryButton(label, nullptr)
{
    SetFocusable(false);
    if (!iconName.empty()) {
        m_IconStorage = iconName;
        SetIcon(m_IconStorage.c_str());
    }
}

} // namespace we::runtime::kindui
