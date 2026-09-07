#include "WindEffects/Editor/EditorSDK.h"
#include "PropertyEditor/PropertyEditorSession.h"
#include "PropertyEditorInternal.h"
#include "PropertyEditor/IDetailsView.h"
#include "Core/Localization.h"
#include "KindUI/Core/WindIcon.h"
#include "KindUI/Core/Icon.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Theming/ThemeAccess.h"
#include "WindEffects/Editor/UI/Widgets/PanelBuilder.h"

// Re-compiled for PanelBuilder API updates
namespace we::programs::editor {
namespace WindIcons = ::we::runtime::kindui::WindIcons;
using ::we::runtime::kindui::kWindIconNone;

using namespace ::we::runtime::kindui;
using ::we::editor::panels::Panel;
using ::we::editor::panels::PanelBuilder;
using ::we::editor::docking::DockZone;
using ::we::editor::property::PropertyEditorSession;
using ::we::editor::property::detail::PopulateDetailsPanelRegions;

std::shared_ptr<Panel> CreateDetailsPanel() {
    const auto title = we::core::Localization::Get().GetString("Panel_Details", "Inspector");

    auto builder = PanelBuilder::Create(std::string(title))
        .Collapsible(false)
        .TabIcon(WindIcons::PaperPencile16);

    if (auto details = PropertyEditorSession::DetailsShared()) {
        auto panel = builder.Build();
        PopulateDetailsPanelRegions(panel, details->GetWidget(), details.get());
        return panel;
    }

    return builder.Build();
}

REGISTER_UI_PANEL(Details,
    WE_PANEL(Details).Title("Inspector").Icon("paper-pencile").Zone(DockZone::Right).WindowMenu("Inspector").SortOrder(3),
    CreateDetailsPanel)

} // namespace we::programs::editor
