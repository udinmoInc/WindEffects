#include "WindEffects/Editor/EditorSDK.h"
#include "PropertyEditor/PropertyEditorSession.h"
#include "PropertyEditor/IDetailsView.h"
#include "ContentBrowser/Widgets/SearchBox.h"
#include "Core/Localization.h"
#include "KindUI/Core/Icon.h"

namespace we::programs::editor {
namespace Icons = ::we::runtime::kindui::Icons;

using namespace ::we::runtime::kindui;
using ::we::editor::panels::Panel;
using ::we::editor::panels::PanelBuilder;
using ::we::editor::docking::DockZone;
using ::we::editor::widgets::SearchBox;
using ::we::editor::property::PropertyEditorSession;

std::shared_ptr<Panel> CreateDetailsPanel() {
    const auto title = we::core::Localization::Get().GetString("Panel_Details", "Details");
    const auto placeholder = we::core::Localization::Get().GetString("UI_SearchPlaceholder", "Search...");

    auto details = PropertyEditorSession::DetailsShared();
    std::shared_ptr<Widget> content =
        details ? details->GetWidget() : std::shared_ptr<Widget>{};

    auto searchBox = std::make_shared<SearchBox>();
    searchBox->SetFillWidth(true);
    searchBox->SetPlaceholder(std::string(placeholder));
    searchBox->SetOnTextChanged([](const std::string& text) {
        if (auto* view = PropertyEditorSession::Details()) {
            view->SetSearchText(text);
        }
    });

    return PanelBuilder(title)
        .TabIcon(Icons::PropertiesName)
        .WithCloseButton()
        .ToolbarBox([searchBox](Row& toolbar) {
            toolbar.AddChild(searchBox);
        })
        .Content(content);
}

REGISTER_UI_PANEL(Details,
    WE_PANEL(Details).Title("Details").Icon("details").Zone(DockZone::Right).WindowMenu("Details").SortOrder(3),
    CreateDetailsPanel)

} // namespace we::programs::editor
