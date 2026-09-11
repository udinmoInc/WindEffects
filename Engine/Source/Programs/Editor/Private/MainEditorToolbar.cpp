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

#include "Core/DiagnosticMacros.h"
#include "Core/LogCategory.h"
#include "KindUI/Core/WindIcon.h"
#include "KindUI/Profiling/ScreenRecorder.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Theming/StyleRole.h"
#include "KindUI/Theming/ThemeAccess.h"

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
using ::we::runtime::kindui::kWindIconNone;
namespace WindIcons = ::we::runtime::kindui::WindIcons;

void LogStubClick(const char* label) {
    WE_LOG_INFO(we::LogCategory::Editor.data(),
        std::string("[Toolbar] ") + label + " clicked (no handler yet)");
    if (we::runtime::kindui::ScreenRecorder::IsRecordingEnabled()) {
        we::runtime::kindui::ScreenRecorder::Get().RecordInput(
            "StubClick", 0.0f, 0.0f, label, "Toolbar", "empty-handler");
    }
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
        left.Group(ToolbarAlignment::Left, ToolbarGroupStyle::ExecutionCluster, [&](ToolbarBuilder& file) {
            file.IconItem(WindIcons::Save16, "Save Level (Ctrl+S)", []() {
                LogStubClick("Save Level (Ctrl+S)");
            });
        });
        left.AddWidget(modeSelector);
        left.Separator();
        left.Group(ToolbarAlignment::Left, ToolbarGroupStyle::ExecutionCluster, [&](ToolbarBuilder& file) {
            file.DropdownItem(WindIcons::Blueprint16, "", []() {
                LogStubClick("Open Blueprints");
            }, "Open Blueprints");
            file.DropdownItem(WindIcons::Clapperboard16, "", []() {
                LogStubClick("Cinematics & Sequencer");
            }, "Cinematics & Sequencer");
            file.DropdownItem(WindIcons::Collab16, "", []() {
                LogStubClick("Collaboration");
            }, "Collaboration");
            file.DropdownItem(WindIcons::Profiler16, "", []() {
                LogStubClick("Profiler");
            }, "Profiler");
            file.DropdownItem(WindIcons::Accessibility16, "", []() {
                LogStubClick("Accessibility");
            }, "Accessibility");
            file.DropdownItem(WindIcons::PlaySettings16, "", []() {
                LogStubClick("Play Settings");
            }, "Play Settings");
        });
    });

    // Center transport: click is recorded; no fake PIE state until real playback exists.
    builder.Center([&](ToolbarBuilder& center) {
        center.Group(ToolbarAlignment::Center, ToolbarGroupStyle::ExecutionCluster, [&](ToolbarBuilder& transport) {
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
        right.Group(ToolbarAlignment::Right, ToolbarGroupStyle::ExecutionCluster, [&](ToolbarBuilder& tools) {
            tools.DropdownItem(WindIcons::Construct16, "Build", []() {
                LogStubClick("Build");
            }, "Build Options");
            tools.DropdownItem(WindIcons::Settings16, "Settings", []() {
                LogStubClick("Settings");
            }, "Settings Options");
            tools.DropdownItem(WindIcons::Window16, "Platform", []() {
                LogStubClick("Platform");
            }, "Target Platform Options");
        });
    });

    auto toolbar = builder.Build();
    toolbar->SetSurfaceRole(we::runtime::kindui::SurfaceRole::Workspace);
    toolbar->SetContext(widgetContext);
    return toolbar;
}

} // namespace we::programs::editor
