// ==============================================================================
// WindEffects — Editor — MainEditorToolbar
// Internal implementation for the Editor module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "MainEditorToolbar.h"

#include "EditorShellBuilder.h"
#include "ViewportNavigationPreferences.h"
#include "Widgets/EditorModeSelector.h"

#include "Widgets/Toolbar.h"
#include "Widgets/ToolbarBuilder.h"
#include "Widgets/ToolbarItem.h"
#include "Widgets/ToolButton.h"
#include "Widgets/WindowsPanelMenuButton.h"
#include "Widgets/DropdownMenu.h"
#include "WindEffects/Editor/UI/Shell/EditorWorkspaceController.h"

#include "Core/DiagnosticMacros.h"
#include "Core/LogCategory.h"
#include <KindUI/EditorUI.h>
#include "KindUI/Diagnostics/ScreenRecorder.h"
#include <functional>
#include <memory>
#include <string>
#include <string_view>

namespace we::programs::editor {
namespace {

using ::we::editor::toolbar::Toolbar;
using ::we::editor::toolbar::ToolbarAlignment;
using ::we::editor::toolbar::ToolbarBuilder;
using ::we::editor::toolbar::ToolbarGroupStyle;
using ::we::editor::toolbar::ToolButton;
using ::we::editor::toolbar::ToolButtonStyle;
using ::we::editor::menus::MenuItem;
using ::we::editor::menus::DropdownMenu;
using ::we::runtime::kindui::kWindIconNone;
namespace WindIcons = ::we::runtime::kindui::WindIcons;

void LogStubClick(const char* label) {
    WE_LOG_INFO(we::LogCategory::Editor.data(),
        std::string("[Toolbar] ") + label + " clicked");
    if (we::runtime::kindui::ScreenRecorder::IsRecordingEnabled()) {
        we::runtime::kindui::ScreenRecorder::Get().RecordInput(
            "StubClick", 0.0f, 0.0f, label, "Toolbar", "handler");
    }
}

std::shared_ptr<MenuItem> MakeMenuItem(
    const std::string& label,
    std::function<void()> onClick,
    const std::string& shortcut = "",
    bool checked = false,
    bool enabled = true)
{
    auto item = std::make_shared<MenuItem>();
    item->label = label;
    item->shortcut = shortcut;
    item->onClick = std::move(onClick);
    item->checked = checked;
    item->enabled = enabled;
    return item;
}

void ShowAnchoredMenu(
    const std::shared_ptr<ToolButton>& button,
    const std::vector<std::shared_ptr<MenuItem>>& items)
{
    if (!button) return;
    auto menu = std::make_shared<DropdownMenu>(items);
    auto* overlay = button->GetPopupHost();
    if (!overlay) {
        overlay = ::we::programs::editor::GetEditorPopupHost();
    }
    if (!overlay) return;

    overlay->CloseAllPopups();
    overlay->ShowAnchoredPopup(
        menu,
        button->GetGeometry(),
        ::we::runtime::kindui::PopupPlacementMode::BottomPreferred);
}

} // namespace

std::shared_ptr<::we::runtime::kindui::Widget> BuildMainEditorToolbar(
    const EditorShellDependencies& deps,
    const std::shared_ptr<::we::runtime::kindui::IWidgetContext>& widgetContext,
    float toolbarHeight,
    float leftInset,
    float rightInset,
    float edgePadding)
{
    (void)deps;
    auto modeSelector = std::make_shared<EditorModeSelector>();
    modeSelector->SetContext(widgetContext);
    modeSelector->InitializeCallbacks(modeSelector);
    modeSelector->Refresh();

    ToolbarBuilder builder;
    builder.Height(toolbarHeight)
        .LeftInset(leftInset)
        .RightInset(rightInset)
        .EdgePadding(edgePadding);

    builder.Left([&](ToolbarBuilder& left) {
        left.Group(ToolbarAlignment::Left, ToolbarGroupStyle::Transparent, [&](ToolbarBuilder& file) {
            file.IconItem(WindIcons::Save16, "Save Level (Ctrl+S)", []() {
                LogStubClick("Save Level (Ctrl+S)");
            });
        });
        left.AddWidget(modeSelector);
        left.Group(ToolbarAlignment::Left, ToolbarGroupStyle::Transparent, [&](ToolbarBuilder& file) {
            file.DropdownItem(WindIcons::Blueprint16, "", nullptr, "Open Blueprints", [](const std::shared_ptr<ToolButton>& btn) {
                btn->SetOnClicked([btn]() {
                    std::vector<std::shared_ptr<MenuItem>> items;
                    items.push_back(MakeMenuItem("Open Level Blueprint", []() { LogStubClick("Open Level Blueprint"); }, "Ctrl+Alt+B"));
                    items.push_back(MakeMenuItem("New Blueprint Class...", []() { LogStubClick("New Blueprint Class"); }));
                    items.push_back(MakeMenuItem("Blueprint Debugger", []() { LogStubClick("Blueprint Debugger"); }));
                    items.push_back(MakeMenuItem("Macro & Function Library", []() { LogStubClick("Macro Library"); }));
                    ShowAnchoredMenu(btn, items);
                });
            });

            file.DropdownItem(WindIcons::Clapperboard16, "", nullptr, "Cinematics & Sequencer", [](const std::shared_ptr<ToolButton>& btn) {
                btn->SetOnClicked([btn]() {
                    std::vector<std::shared_ptr<MenuItem>> items;
                    items.push_back(MakeMenuItem("Add Level Sequence", []() { LogStubClick("Add Level Sequence"); }));
                    items.push_back(MakeMenuItem("Add Master Sequence", []() { LogStubClick("Add Master Sequence"); }));
                    items.push_back(MakeMenuItem("Open Sequencer Panel", []() {
                        auto& ws = we::programs::editor::EditorWorkspaceController::Get();
                        ws.SetPanelVisible("Sequencer", true);
                        ws.FocusPanel("Sequencer");
                    }));
                    items.push_back(MakeMenuItem("Cinematic Viewport Mode", []() { LogStubClick("Cinematic Viewport Mode"); }));
                    ShowAnchoredMenu(btn, items);
                });
            });

            file.DropdownItem(WindIcons::Collab16, "", nullptr, "Collaboration", [](const std::shared_ptr<ToolButton>& btn) {
                btn->SetOnClicked([btn]() {
                    std::vector<std::shared_ptr<MenuItem>> items;
                    items.push_back(MakeMenuItem("Source Control Settings...", []() { LogStubClick("Source Control Settings"); }));
                    items.push_back(MakeMenuItem("Multi-User Editing Session...", []() { LogStubClick("Multi-User Editing"); }));
                    items.push_back(MakeMenuItem("Submit Level Changes", []() { LogStubClick("Submit Level Changes"); }, "Ctrl+Shift+S"));
                    items.push_back(MakeMenuItem("Sync Project Assets", []() { LogStubClick("Sync Project Assets"); }));
                    ShowAnchoredMenu(btn, items);
                });
            });

            file.DropdownItem(WindIcons::Profiler16, "", nullptr, "Profiler", [](const std::shared_ptr<ToolButton>& btn) {
                btn->SetOnClicked([btn]() {
                    std::vector<std::shared_ptr<MenuItem>> items;
                    items.push_back(MakeMenuItem("GPU Profiler", []() { LogStubClick("GPU Profiler"); }, "Ctrl+Shift+,"));
                    items.push_back(MakeMenuItem("CPU Performance Profiler", []() { LogStubClick("CPU Profiler"); }));
                    items.push_back(MakeMenuItem("Memory Usage Insights", []() { LogStubClick("Memory Insights"); }));
                    items.push_back(MakeMenuItem("UI Input Latency Audit", []() { LogStubClick("UI Latency Audit"); }));
                    ShowAnchoredMenu(btn, items);
                });
            });

            file.DropdownItem(WindIcons::Accessibility16, "", nullptr, "Accessibility", [](const std::shared_ptr<ToolButton>& btn) {
                btn->SetOnClicked([btn]() {
                    std::vector<std::shared_ptr<MenuItem>> items;
                    items.push_back(MakeMenuItem("UI Scale Factor 1.0x", []() { LogStubClick("UI Scale 1.0x"); }, "", true));
                    items.push_back(MakeMenuItem("UI Scale Factor 1.25x", []() { LogStubClick("UI Scale 1.25x"); }));
                    items.push_back(MakeMenuItem("UI Scale Factor 1.5x", []() { LogStubClick("UI Scale 1.5x"); }));
                    items.push_back(MakeMenuItem("High Contrast Theme", []() { LogStubClick("High Contrast Theme"); }));
                    items.push_back(MakeMenuItem("Color Blindness Vision Simulator", []() { LogStubClick("Vision Simulator"); }));
                    ShowAnchoredMenu(btn, items);
                });
            });

            file.DropdownItem(WindIcons::PlaySettings16, "", nullptr, "Play Settings", [](const std::shared_ptr<ToolButton>& btn) {
                btn->SetOnClicked([btn]() {
                    std::vector<std::shared_ptr<MenuItem>> items;
                    items.push_back(MakeMenuItem("Play in Selected Viewport (PIE)", []() { LogStubClick("Play in Viewport"); }, "", true));
                    items.push_back(MakeMenuItem("Play in New Window", []() { LogStubClick("Play in New Window"); }));
                    items.push_back(MakeMenuItem("Simulate World", []() { LogStubClick("Simulate World"); }));
                    items.push_back(MakeMenuItem("Network Mode: Standalone", []() { LogStubClick("Network Standalone"); }, "", true));
                    items.push_back(MakeMenuItem("Mute Audio in PIE", []() { LogStubClick("Mute Audio in PIE"); }));
                    ShowAnchoredMenu(btn, items);
                });
            });
        });
    });

    // Center transport
    builder.Center([&](ToolbarBuilder& center) {
        center.Group(ToolbarAlignment::Center, ToolbarGroupStyle::Transparent, [&](ToolbarBuilder& transport) {
            transport.IconItem(WindIcons::Play16, "Play (PIE)", []() {
                LogStubClick("Play (PIE)");
            }, [](const std::shared_ptr<ToolButton>& btn) {
                btn->SetButtonStyle(ToolButtonStyle::PlayButton);
            });
            transport.IconItem(WindIcons::Pause16, "Pause (PIE)", []() {
                LogStubClick("Pause (PIE)");
            });
            transport.IconItem(WindIcons::Stop16, "Stop (PIE)", []() {
                LogStubClick("Stop (PIE)");
            });
        });
    });

    builder.Right([&](ToolbarBuilder& right) {
        right.Group(ToolbarAlignment::Right, ToolbarGroupStyle::Transparent, [&](ToolbarBuilder& tools) {
            tools.DropdownItem(WindIcons::Construct16, "Build", nullptr, "Build Options", [](const std::shared_ptr<ToolButton>& btn) {
                btn->SetOnClicked([btn]() {
                    std::vector<std::shared_ptr<MenuItem>> items;
                    items.push_back(MakeMenuItem("Compile", []() {
                        LogStubClick("Compile");
                    }, "Ctrl+Shift+B"));
                    items.push_back(MakeMenuItem("Build All Level Content", []() { LogStubClick("Build All"); }));
                    items.push_back(MakeMenuItem("Build Lighting", []() { LogStubClick("Build Lighting"); }));
                    items.push_back(MakeMenuItem("Build Geometry & BSP", []() { LogStubClick("Build Geometry"); }));
                    items.push_back(MakeMenuItem("Build Navigation Mesh", []() { LogStubClick("Build Navigation"); }));
                    items.push_back(MakeMenuItem("Package Project (Win64)", []() { LogStubClick("Package Project"); }));
                    items.push_back(MakeMenuItem("Cook Content", []() { LogStubClick("Cook Content"); }));
                    ShowAnchoredMenu(btn, items);
                });
            });

            tools.DropdownItem(WindIcons::Settings16, "Settings", nullptr, "Settings Options", [](const std::shared_ptr<ToolButton>& btn) {
                btn->SetOnClicked([btn]() {
                    std::vector<std::shared_ptr<MenuItem>> items;
                    items.push_back(MakeMenuItem("Project Settings...", []() { LogStubClick("Project Settings"); }));
                    items.push_back(MakeMenuItem("Editor Preferences...", []() { LogStubClick("Editor Preferences"); }));
                    items.push_back(MakeMenuItem("Engine Configurations...", []() { LogStubClick("Engine Configurations"); }));
                    items.push_back(MakeMenuItem("Plugin Manager...", []() { LogStubClick("Plugin Manager"); }));
                    items.push_back(MakeMenuItem("Focus World Settings", []() {
                        auto& ws = we::programs::editor::EditorWorkspaceController::Get();
                        ws.SetPanelVisible("WorldOutliner", true);
                        ws.FocusPanel("WorldOutliner");
                    }));
                    ShowAnchoredMenu(btn, items);
                });
            });

            tools.DropdownItem(WindIcons::Window16, "Platform", nullptr, "Target Platform Options", [](const std::shared_ptr<ToolButton>& btn) {
                btn->SetOnClicked([btn]() {
                    std::vector<std::shared_ptr<MenuItem>> items;
                    items.push_back(MakeMenuItem("Windows (64-bit)", []() { LogStubClick("Windows 64-bit"); }, "", true));
                    items.push_back(MakeMenuItem("DirectX 12 (RHI)", []() { LogStubClick("DirectX 12 RHI"); }, "", true));
                    items.push_back(MakeMenuItem("Vulkan RHI", []() { LogStubClick("Vulkan RHI"); }));
                    items.push_back(MakeMenuItem("Null RHI (Headless)", []() { LogStubClick("Null RHI"); }));
                    items.push_back(MakeMenuItem("Package for Windows", []() { LogStubClick("Package Windows"); }));
                    ShowAnchoredMenu(btn, items);
                });
            });
        });
    });

    auto toolbar = builder.Build();
    toolbar->SetSurfaceRole(we::runtime::kindui::SurfaceRole::Workspace);
    toolbar->SetContext(widgetContext);
    return toolbar;
}

} // namespace we::programs::editor
