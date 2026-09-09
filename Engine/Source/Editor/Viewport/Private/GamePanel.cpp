// ==============================================================================
// WindEffects — Viewport — GamePanel
// Internal implementation for the Viewport module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "WindEffects/Editor/EditorSDK.h"
#include "Widgets/ToolbarBuilder.h"
#include "KindUI/Widgets/Label.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Theming/ThemeAccess.h"

// Re-compiled for PanelBuilder API updates
namespace we::programs::editor {
using namespace ::we::runtime::kindui;
using ::we::runtime::kindui::panels::Panel;
using ::we::runtime::kindui::panels::PanelBuilder;
using ::we::editor::docking::DockZone;
using ::we::editor::toolbar::ToolbarBuilder;
namespace WindIcons = ::we::runtime::kindui::WindIcons;
using ::we::runtime::kindui::kWindIconNone;

std::shared_ptr<Panel> CreateGamePanel() {
    auto toolbar = ToolbarBuilder()
        .Height(we::runtime::kindui::ResolveMetric(we::runtime::kindui::MetricToken::PanelToolbarHeight))
        .IconSize(we::runtime::kindui::ResolveMetric(we::runtime::kindui::MetricToken::IconSizeToolbar))
        .Dropdown(WindIcons::PlayForward16, "Game", {}, "Game View Options")
        .Separator()
        .Dropdown(WindIcons::ToolbarCamera16, "Display 1", {}, "Select Display")
        .Separator()
        .Dropdown(WindIcons::ToolbarScaling16, "Auto Resolution", {}, "Select Resolution")
        .Separator()
        .Dropdown(WindIcons::AdjustHorizon16, "Free Aspect", {}, "Aspect Ratio")
        .Separator()
        .Dropdown(WindIcons::Plus16, "1x", {}, "View Scale")
        .Separator()
        .Dropdown(WindIcons::Eye16, "Play Focus", {}, "Play Focus Mode")
        .Separator()
        .Item(WindIcons::Console16, "Stats", {}, "Toggle Stats")
        .Separator()
        .Dropdown(WindIcons::Grid16, "Gizmos", {}, "Toggle Gizmos")
        .Build();

    return PanelBuilder("Game")
        .WithCloseButton()
        .Toolbar(toolbar)
        .Content(std::make_shared<Label>(""));
}

REGISTER_UI_PANEL(Game,
    WE_PANEL(Game).Title("Game").Zone(DockZone::Floating).Hidden(),
    CreateGamePanel)

} // namespace we::programs::editor
