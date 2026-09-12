// ==============================================================================
// WindEffects — PropertyEditor — DetailsPanel
// Internal implementation for the PropertyEditor module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "WindEffects/Editor/EditorSDK.h"
#include "WindEffects/Editor/UI/Shell/EditorWorkspaceController.h"
#include "PropertyEditor/PropertyEditorSession.h"
#include "PropertyEditorInternal.h"
#include "PropertyEditor/IDetailsView.h"
#include "Core/Localization.h"
#include "KindUI/EditorWidgets.h"

namespace we::programs::editor {
namespace WindIcons = ::we::runtime::kindui::WindIcons;

using namespace ::we::runtime::kindui;
using ::we::runtime::kindui::panels::Panel;
using ::we::runtime::kindui::panels::PanelBuilder;
using ::we::editor::docking::DockZone;
using ::we::editor::property::PropertyEditorSession;
using ::we::editor::property::detail::PopulateDetailsPanelRegions;

std::shared_ptr<Panel> CreateDetailsPanel() {
    const auto title = we::core::Localization::Get().GetString("Panel_Details", "Inspector");

    return we::editor::dsl::Panel(std::string(title), [&](we::editor::dsl::PanelContext& p) {
        p.TabIcon(WindIcons::PaperPencile16)
         .WithCloseButton([]() {
             EditorWorkspaceController::Get().SetPanelVisible("Details", false);
         });

        if (auto details = PropertyEditorSession::DetailsShared()) {
            PopulateDetailsPanelRegions(p, details->GetWidget(), details.get());
        }
    });
}

REGISTER_UI_PANEL(Details,
    WE_PANEL(Details).Title("Inspector").Icon("paper-pencile").Zone(DockZone::Right).WindowMenu("Inspector").SortOrder(3),
    CreateDetailsPanel)

}
