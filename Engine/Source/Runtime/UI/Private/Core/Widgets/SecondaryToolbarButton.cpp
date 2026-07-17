#include "WindEffects/Runtime/UI/Core/Widgets/SecondaryToolbarButton.h"

namespace WindEffects::Editor::UI {

SecondaryToolbarButton::SecondaryToolbarButton(const std::string& label, const std::string& iconName)
    : SecondaryButton(label, nullptr)
{
    SetFocusable(false);
    if (!iconName.empty()) {
        m_IconStorage = iconName;
        SetIcon(m_IconStorage.c_str());
    }
}

} // namespace WindEffects::Editor::UI
