// ==============================================================================
// WindEffects — Viewport — ViewportToolbar
// Internal implementation for the Viewport module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "ViewportToolbar.h"
#include "ViewportToolbarState.h"
#include "ViewportNavigationPreferences.h"

#include "ViewportEdit/ViewportEditSession.h"
#include "ViewportEdit/ViewportEditTypes.h"
#include "ViewportEdit/IViewportCameraController.h"
#include "ViewportEdit/IViewportSnapProvider.h"
#include "WindEffects/Editor/UI/Shell/EditorToolsRegistry.h"

#include "Widgets/Toolbar.h"
#include "Widgets/ToolbarBuilder.h"
#include "Widgets/ToolButton.h"
#include "Widgets/DropdownMenu.h"
#include "WindEffects/Editor/UI/Shell/EditorWorkspaceController.h"

#include <KindUI/EditorUI.h>
#include <functional>
#include <memory>
#include <string>

namespace we::programs::editor {
using ::we::runtime::kindui::WindIconRef;
namespace {

using ::we::editor::toolbar::ToolButton;
using ::we::editor::toolbar::ToolButtonStyle;
using ::we::editor::toolbar::Toolbar;
using ::we::editor::toolbar::ToolbarAlignment;
using ::we::editor::toolbar::ToolbarBuilder;
using ::we::editor::toolbar::ToolbarGroupStyle;
using ::we::editor::toolspanel::EditorToolsRegistry;
using ::we::editor::viewportedit::ViewportEditSession;
namespace WindIcons = ::we::runtime::kindui::WindIcons;
using ::we::runtime::kindui::kWindIconNone;

std::shared_ptr<ToolButton> MakeViewportChip(
    we::runtime::kindui::WindIconRef icon,
    const std::string& label,
    std::function<void()> onClick,
    const char* tooltip,
    bool dropdown = false)
{
    auto button = std::make_shared<ToolButton>(icon, label, std::move(onClick), tooltip);
    button->SetButtonStyle(ToolButtonStyle::ViewportChip);
    button->SetIsDropdown(dropdown);
    button->SetVerticalAlignment(::we::runtime::kindui::VerticalAlignment::Center);
    return button;
}

std::shared_ptr<ToolButton> MakeViewportIconChip(
    we::runtime::kindui::WindIconRef icon,
    std::function<void()> onClick,
    const char* tooltip)
{
    return MakeViewportChip(icon, "", std::move(onClick), tooltip, false);
}

void RunRegistryTool(std::string_view toolId) {
    if (const auto* tool = EditorToolsRegistry::Get().FindTool(toolId)) {
        if (tool->onExecute) {
            tool->onExecute();
        }
    }
}

void ActivateViewportTool(
    const std::shared_ptr<std::shared_ptr<Toolbar>>& toolbarHolder,
    we::runtime::kindui::WindIconRef icon,
    std::string_view toolId)
{
    if (toolbarHolder && *toolbarHolder) {
        (*toolbarHolder)->SetActiveTool(icon);
    }
    if (auto* editor = ViewportEditSession::Editor()) {
        editor->SetActiveTool(toolId);
        return;
    }
    RunRegistryTool(toolId);
}

void ShowPopupMenu(
    const ::we::runtime::kindui::Rect& anchor,
    const std::vector<std::shared_ptr<::we::editor::menus::MenuItem>>& items)
{
    auto menu = std::make_shared<::we::editor::menus::DropdownMenu>(items);
    auto* overlay = GetEditorPopupHost();
    if (!overlay) {
        return;
    }
    overlay->CloseAllPopups();
    overlay->ShowPopup(
        menu,
        ::we::runtime::kindui::Point{ anchor.x, anchor.y + anchor.height + 2.0f });
}

void ShowPerspectiveMenu(const ::we::runtime::kindui::Rect& anchor) {
    auto* editor = ViewportEditSession::Editor();
    const auto current = editor
        ? editor->Camera().GetProjection()
        : ::we::editor::viewportedit::CameraProjection::Perspective;

    auto makeCheckedItem = [](const std::string& label, bool checked) {
        auto item = std::make_shared<::we::editor::menus::MenuItem>();
        item->label = label;
        item->checked = checked;
        item->enabled = checked;
        return item;
    };

    std::vector<std::shared_ptr<::we::editor::menus::MenuItem>> items;
    items.push_back(makeCheckedItem(
        "Perspective",
        current == ::we::editor::viewportedit::CameraProjection::Perspective));
    ShowPopupMenu(anchor, items);
}

void ShowShowMenu(const ::we::runtime::kindui::Rect& anchor) {
    auto* editor = ViewportEditSession::Editor();
    if (!editor) {
        return;
    }

    auto& grid = editor->Grid();
    auto snapSettings = editor->Snap().GetSettings();

    auto makeToggle = [](const std::string& label, bool checked, std::function<void()> onClick) {
        auto item = std::make_shared<::we::editor::menus::MenuItem>();
        item->label = label;
        item->checked = checked;
        item->onClick = std::move(onClick);
        return item;
    };

    std::vector<std::shared_ptr<::we::editor::menus::MenuItem>> items;
    items.push_back(makeToggle("Grid", grid.IsVisible(), [editor]() {
        auto& g = editor->Grid();
        g.SetVisible(!g.IsVisible());
    }));
    items.push_back(makeToggle("Grid Snap", snapSettings.gridEnabled, [editor]() {
        auto settings = editor->Snap().GetSettings();
        settings.gridEnabled = !settings.gridEnabled;
        editor->Snap().SetSettings(settings);
    }));
    items.push_back(makeToggle("Rotation Snap", snapSettings.rotationEnabled, [editor]() {
        auto settings = editor->Snap().GetSettings();
        settings.rotationEnabled = !settings.rotationEnabled;
        editor->Snap().SetSettings(settings);
    }));
    items.push_back(std::make_shared<::we::editor::menus::MenuItem>());
    auto navItem = std::make_shared<::we::editor::menus::MenuItem>();
    navItem->label = "Viewport Navigation Settings...";
    navItem->onClick = []() { ShowViewportNavigationPreferences(); };
    items.push_back(navItem);

    ShowPopupMenu(anchor, items);
}

void ToggleGridSnap() {
    auto* editor = ViewportEditSession::Editor();
    if (!editor) {
        return;
    }
    auto settings = editor->Snap().GetSettings();
    settings.gridEnabled = !settings.gridEnabled;
    editor->Snap().SetSettings(settings);
}

void ShowLitMenu(const ::we::runtime::kindui::Rect& anchor) {
    std::vector<std::shared_ptr<::we::editor::menus::MenuItem>> items;
    auto lit = std::make_shared<::we::editor::menus::MenuItem>();
    lit->label = "Lit";
    lit->checked = true;
    lit->enabled = true;
    items.push_back(lit);
    auto unlit = std::make_shared<::we::editor::menus::MenuItem>();
    unlit->label = "Unlit";
    items.push_back(unlit);
    ShowPopupMenu(anchor, items);
}

void ToggleRotationSnap() {
    auto* editor = ViewportEditSession::Editor();
    if (!editor) {
        return;
    }
    auto settings = editor->Snap().GetSettings();
    settings.rotationEnabled = !settings.rotationEnabled;
    editor->Snap().SetSettings(settings);
}

void ToggleScaleSnap() {
    auto* editor = ViewportEditSession::Editor();
    if (!editor) {
        return;
    }
    auto settings = editor->Snap().GetSettings();
    settings.scaleEnabled = !settings.scaleEnabled;
    editor->Snap().SetSettings(settings);
}

we::runtime::kindui::WindIconRef RegistryToolIcon(std::string_view toolId) {
    if (const auto* tool = EditorToolsRegistry::Get().FindTool(toolId)) {
        return tool->icon;
    }
    return kWindIconNone;
}

} // namespace

std::shared_ptr<::we::runtime::kindui::Widget> CreateViewportToolbar() {
    auto toolbarHolder = std::make_shared<std::shared_ptr<Toolbar>>();
    std::shared_ptr<ToolButton> cameraSpeedButton;

    ToolbarBuilder builder;
    builder.Height(we::runtime::kindui::ResolveMetric(
        we::runtime::kindui::MetricToken::ViewportToolbarHeight));

    // Flat left items — floating icons/labels, no rounded hover boxes.
    builder.Group(ToolbarAlignment::Left, ToolbarGroupStyle::Transparent, [&](ToolbarBuilder& tools) {
        const auto floatChrome = [](const std::shared_ptr<ToolButton>& btn) {
            if (btn) {
                btn->SetChromeless(true);
            }
        };

        const WindIconRef handIcon = WindIcons::ToolbarHand16;
        const WindIconRef moveIcon = WindIcons::MoveOutline16;
        const WindIconRef rotateIcon = WindIcons::ToolbarRotate16;
        const WindIconRef scaleIcon = WindIcons::ToolbarScaling16;
        tools.IconItem(
            handIcon,
            "Hand (Pan Viewport)",
            [toolbarHolder, handIcon]() { ActivateViewportTool(toolbarHolder, handIcon, "SelectTool"); },
            floatChrome);
        tools.IconItem(
            moveIcon,
            "Move (W)",
            [toolbarHolder, moveIcon]() { ActivateViewportTool(toolbarHolder, moveIcon, "MoveTool"); },
            floatChrome);
        tools.IconItem(
            rotateIcon,
            "Rotate (E)",
            [toolbarHolder, rotateIcon]() { ActivateViewportTool(toolbarHolder, rotateIcon, "RotateTool"); },
            floatChrome);
        tools.IconItem(
            scaleIcon,
            "Scale (R)",
            [toolbarHolder, scaleIcon]() { ActivateViewportTool(toolbarHolder, scaleIcon, "ScaleTool"); },
            floatChrome);

        tools.IconItem(
            WindIcons::Globe16,
            "Cycle Coordinate Space (World / Local)",
            []() {},
            floatChrome);
        tools.IconItem(
            WindIcons::Speaker16,
            "Toggle Viewport Audio",
            []() {},
            floatChrome);

        tools.DropdownItem(
            WindIcons::Grid16,
            "10",
            []() { ToggleGridSnap(); },
            "Toggle Grid Snap",
            floatChrome);
        tools.DropdownItem(
            WindIcons::ToolbarRotate16,
            "90°",
            []() { ToggleRotationSnap(); },
            "Toggle Rotation Snap",
            floatChrome);
        tools.DropdownItem(
            WindIcons::ToolbarScaling16,
            "0.25",
            []() { ToggleScaleSnap(); },
            "Toggle Scale Snap",
            floatChrome);

        tools.DropdownItem(
            WindIcons::ToolbarCamera16,
            "Perspective",
            []() {},
            "Viewport Projection",
            [floatChrome](const std::shared_ptr<ToolButton>& btn) {
                floatChrome(btn);
                btn->SetOnClicked([btn]() {
                    ShowPerspectiveMenu(btn->GetGeometry());
                });
            });
        tools.DropdownItem(
            WindIcons::Lit16,
            "Lit",
            []() {},
            "Viewport Lighting Mode",
            [floatChrome](const std::shared_ptr<ToolButton>& btn) {
                floatChrome(btn);
                btn->SetOnClicked([btn]() {
                    ShowLitMenu(btn->GetGeometry());
                });
            });
        tools.DropdownItem(
            WindIcons::Eye16,
            "Show",
            []() {},
            "Show Viewport Options",
            [floatChrome](const std::shared_ptr<ToolButton>& btn) {
                floatChrome(btn);
                btn->SetOnClicked([btn]() {
                    ShowShowMenu(btn->GetGeometry());
                });
            });
    });

    // Right-aligned group (Camera speed + Multi-Viewport Layout)
    builder.Group(ToolbarAlignment::Right, ToolbarGroupStyle::Transparent, [&](ToolbarBuilder& tools) {
        const auto floatChrome = [](const std::shared_ptr<ToolButton>& btn) {
            if (btn) {
                btn->SetChromeless(true);
            }
        };
        tools.DropdownItem(
            WindIcons::ToolbarVideocamera16,
            "1",
            []() { ShowViewportCameraSpeedPopup(); },
            "Camera Speed",
            [floatChrome, &cameraSpeedButton](const std::shared_ptr<ToolButton>& btn) {
                floatChrome(btn);
                btn->SetOnMouseWheel([](float wheelDeltaY) {
                    AdjustViewportCameraSpeedFromWheel(wheelDeltaY);
                });
                cameraSpeedButton = btn;
            });
        tools.IconItem(
            WindIcons::Grid16,
            "Multi-Viewport Layout",
            []() {
                if (auto* editor = ViewportEditSession::Editor()) {
                    auto& grid = editor->Grid();
                    grid.SetVisible(!grid.IsVisible());
                }
            },
            floatChrome);
    });

    auto toolbar = builder.Build();
    *toolbarHolder = toolbar;
    toolbar->SetActiveTool(we::runtime::kindui::kWindIconNone);

    if (cameraSpeedButton) {
        SetViewportCameraSpeedIndicator(cameraSpeedButton);
    }

    return toolbar;
}

} // namespace we::programs::editor
 
