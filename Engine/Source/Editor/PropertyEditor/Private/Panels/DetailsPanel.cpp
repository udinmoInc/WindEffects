#include "WindEffects/Editor/EditorSDK.h"
#include "PropertyEditor/PropertyEditorSession.h"
#include "PropertyEditorInternal.h"
#include "PropertyEditor/IDetailsView.h"
#include "Core/Localization.h"
#include "KindUI/Core/WindIcon.h"
#include "KindUI/Core/Icon.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Theming/ThemeAccess.h"

namespace we::programs::editor {
namespace WindIcons = ::we::runtime::kindui::WindIcons;
using ::we::runtime::kindui::kWindIconNone;

using namespace ::we::runtime::kindui;
using ::we::editor::panels::Panel;
using ::we::editor::docking::DockZone;
using ::we::editor::property::PropertyEditorSession;
using ::we::editor::property::detail::PopulateDetailsPanelRegions;

std::shared_ptr<Panel> CreateDetailsPanel() {
    const auto title = we::core::Localization::Get().GetString("Panel_Details", "Details");

    auto panel = std::make_shared<Panel>(std::string(title));
    panel->AttachBodyLayout();
    panel->SetHeaderHeight(we::runtime::kindui::ResolveMetric(MetricToken::PanelHeaderHeight));
    panel->SetCollapsible(false);
    panel->SetTabIcon(we::runtime::kindui::kWindIconNone);

    if (auto details = PropertyEditorSession::DetailsShared()) {
        PopulateDetailsPanelRegions(panel, details->GetWidget(), details.get());
    }

    return panel;
}

REGISTER_UI_PANEL(Details,
    WE_PANEL(Details).Title("Details").Icon("details").Zone(DockZone::Right).WindowMenu("Details").SortOrder(3),
    CreateDetailsPanel)

} // namespace we::programs::editor
