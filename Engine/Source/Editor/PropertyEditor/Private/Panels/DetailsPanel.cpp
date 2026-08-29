#include "WindEffects/Editor/EditorSDK.h"
#include "PropertyEditor/PropertyEditorSession.h"
#include "PropertyEditorInternal.h"
#include "PropertyEditor/IDetailsView.h"
#include "Core/Localization.h"
#include "KindUI/Core/Icon.h"

namespace we::programs::editor {
namespace Icons = ::we::runtime::kindui::Icons;

using namespace ::we::runtime::kindui;
using ::we::editor::panels::Panel;
using ::we::editor::panels::PanelBuilder;
using ::we::editor::docking::DockZone;
using ::we::editor::property::PropertyEditorSession;
using ::we::editor::property::detail::CreateDetailsPanelInterior;

std::shared_ptr<Panel> CreateDetailsPanel() {
    const auto title = we::core::Localization::Get().GetString("Panel_Details", "Details");

    auto details = PropertyEditorSession::DetailsShared();
    std::shared_ptr<Widget> content;
    if (details) {
        content = CreateDetailsPanelInterior(details->GetWidget(), details.get());
    }

    return PanelBuilder(title)
        .TabIcon(Icons::PropertiesName)
        .WithCloseButton()
        .Content(content);
}

REGISTER_UI_PANEL(Details,
    WE_PANEL(Details).Title("Details").Icon("details").Zone(DockZone::Right).WindowMenu("Details").SortOrder(3),
    CreateDetailsPanel)

} // namespace we::programs::editor
