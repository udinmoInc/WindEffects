#include "UI/Dialogs/RenameProject/RenameDialogView.h"

#include "UI/Dialogs/DialogStyles.h"
#include "UI/LauncherHelpers.h"

#include "KindUI/Core/Widgets/PrimaryToolbarButton.h"
#include "KindUI/Core/Widgets/SecondaryToolbarButton.h"
#include "KindUI/Core/Icon.h"
#include "KindUI/Declarative/UI.h"
#include "KindUI/Theming/ThemeToken.h"
#include "KindUI/Widgets/TextBox.h"

namespace we::programs::welauncher {
namespace UI = we::runtime::kindui::UI;

we::runtime::kindui::Element BuildRenameDialogView(const RenameDialogState& state) {
    const std::string name = state.name;
    auto onCancel = state.onCancel;
    auto onRename = state.onRename;
    auto onNameChanged = state.onNameChanged;
    const float s = LScale();
    const float pad = LMetric(we::runtime::kindui::ThemeToken::Space6) * s;
    const float gap = LMetric(we::runtime::kindui::ThemeToken::Space3) * s;

    return UI::Style(
        UI::Padding(
            UI::Column({
                UI::Section("Rename Project", "Enter a new display name."),
                UI::Host([name, onNameChanged]() {
                    auto box = std::make_shared<we::runtime::kindui::TextBox>(
                        name,
                        [onNameChanged](const std::string& text) {
                            if (onNameChanged) {
                                onNameChanged(text);
                            }
                        });
                    return box;
                }),
                UI::Row({
                    UI::Spacer(),
                    UI::Host([onCancel]() {
                        auto btn = std::make_shared<we::runtime::kindui::SecondaryToolbarButton>("Cancel", "");
                        btn->SetOnClicked(onCancel);
                        return btn;
                    }),
                    UI::Host([onRename]() {
                        auto btn = std::make_shared<we::runtime::kindui::PrimaryToolbarButton>(
                            "Rename", we::runtime::kindui::Icons::CheckName);
                        btn->SetOnClicked(onRename);
                        return btn;
                    }),
                }),
            }),
            pad,
            pad,
            pad,
            pad),
        dialogs::Styles::Panel);
}

} // namespace we::programs::welauncher
