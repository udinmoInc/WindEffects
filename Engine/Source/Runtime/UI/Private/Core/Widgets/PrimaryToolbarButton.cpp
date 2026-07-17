#include "WindEffects/Runtime/UI/Core/Widgets/PrimaryToolbarButton.h"

namespace WindEffects::Editor::UI {

PrimaryToolbarButton::PrimaryToolbarButton(const std::string& label, const std::string& iconName)
    : PrimaryButton(label, nullptr)
{
    SetFocusable(false);
    if (!iconName.empty()) {
        m_IconStorage = iconName;
        SetIcon(m_IconStorage.c_str());
    }
}

} // namespace WindEffects::Editor::UI
