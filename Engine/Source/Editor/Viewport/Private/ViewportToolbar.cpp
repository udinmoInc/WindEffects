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

#include "KindUI/Core/Icon.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Theming/ThemeAccess.h"

#include <functional>
#include <memory>
#include <string>

namespace we::programs::editor {
namespace {

using ::we::editor::toolbar::ToolButton;
using ::we::editor::toolbar::ToolButtonStyle;
using ::we::editor::toolbar::Toolbar;
using ::we::editor::toolbar::ToolbarAlignment;
using ::we::editor::toolbar::ToolbarBuilder;
using ::we::editor::toolbar::ToolbarGroupStyle;
using ::we::editor::toolspanel::EditorToolsRegistry;
using ::we::editor::viewportedit::ViewportEditSession;
namespace Icons = ::we::runtime::kindui::Icons;

std::shared_ptr<ToolButton> MakeViewportChip(
    const char* icon,
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
    const char* icon,
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
    const char* iconName,
    std::string_view toolId)
{
    if (toolbarHolder && *toolbarHolder) {
        (*toolbarHolder)->SetActiveTool(iconName);
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

} // namespace

std::shared_ptr<::we::runtime::kindui::Widget> CreateViewportToolbar() {
    auto toolbarHolder = std::make_shared<std::shared_ptr<Toolbar>>();
    std::shared_ptr<ToolButton> cameraSpeedButton;

    ToolbarBuilder builder;
    builder.Floating().Height(we::runtime::kindui::ResolveMetric(
        we::runtime::kindui::MetricToken::ViewportToolbarHeight));

    auto perspectiveButton = MakeViewportChip(
        Icons::PerspectiveName,
        "Perspective",
        nullptr,
        "Viewport Projection",
        true);
    perspectiveButton->SetOnClicked([perspectiveButton]() {
        ShowPerspectiveMenu(perspectiveButton->GetGeometry());
    });

    auto litButton = MakeViewportChip(
        Icons::LitName,
        "Lit",
        nullptr,
        "Viewport Lighting Mode",
        true);
    litButton->SetOnClicked([litButton]() {
        ShowLitMenu(litButton->GetGeometry());
    });

    auto showButton = MakeViewportChip(
        Icons::EyeName,
        "Show",
        nullptr,
        "Show Viewport Options",
        true);
    showButton->SetOnClicked([showButton]() {
        ShowShowMenu(showButton->GetGeometry());
    });

    builder.AddWidget(perspectiveButton);
    builder.AddWidget(litButton);
    builder.AddWidget(showButton);
    builder.Separator();

    builder.Group(ToolbarAlignment::Left, ToolbarGroupStyle::Transparent, [&](ToolbarBuilder& tools) {
        tools.IconItem(
            Icons::CursorName,
            "Select (Q)",
            [toolbarHolder]() { ActivateViewportTool(toolbarHolder, Icons::CursorName, "SelectTool"); },
            [](const std::shared_ptr<ToolButton>& btn) {
                btn->SetButtonStyle(ToolButtonStyle::ViewportChip);
            });
        tools.IconItem(
            Icons::MoveName,
            "Move (W)",
            [toolbarHolder]() { ActivateViewportTool(toolbarHolder, Icons::MoveName, "MoveTool"); },
            [](const std::shared_ptr<ToolButton>& btn) {
                btn->SetButtonStyle(ToolButtonStyle::ViewportChip);
            });
        tools.IconItem(
            Icons::RotateName,
            "Rotate (E)",
            [toolbarHolder]() { ActivateViewportTool(toolbarHolder, Icons::RotateName, "RotateTool"); },
            [](const std::shared_ptr<ToolButton>& btn) {
                btn->SetButtonStyle(ToolButtonStyle::ViewportChip);
            });
        tools.IconItem(
            Icons::ScaleName,
            "Scale (R)",
            [toolbarHolder]() { ActivateViewportTool(toolbarHolder, Icons::ScaleName, "ScaleTool"); },
            [](const std::shared_ptr<ToolButton>& btn) {
                btn->SetButtonStyle(ToolButtonStyle::ViewportChip);
            });
    });

    builder.Separator();

    builder.AddWidget(MakeViewportIconChip(
        Icons::GridName,
        []() {
            if (auto* editor = ViewportEditSession::Editor()) {
                auto& grid = editor->Grid();
                grid.SetVisible(!grid.IsVisible());
            }
        },
        "Toggle Grid"));

    builder.AddWidget(MakeViewportIconChip(
        Icons::SnapName,
        []() { ToggleGridSnap(); },
        "Toggle Grid Snap"));

    builder.AddWidget(MakeViewportIconChip(
        Icons::RotateName,
        []() { ToggleRotationSnap(); },
        "Toggle Rotation Snap"));

    builder.AddWidget(MakeViewportIconChip(
        Icons::ScaleName,
        []() { ToggleScaleSnap(); },
        "Toggle Scale Snap"));

    builder.Separator();

    auto cameraButton = MakeViewportChip(
        Icons::CameraName,
        "Camera",
        []() { ShowViewportCameraSpeedPopup(); },
        "Camera Speed",
        true);
    cameraButton->SetOnMouseWheel([](float wheelDeltaY) {
        AdjustViewportCameraSpeedFromWheel(wheelDeltaY);
    });
    cameraSpeedButton = cameraButton;

    builder.AddWidget(MakeViewportIconChip(
        Icons::SettingsName,
        []() { ShowViewportNavigationPreferences(); },
        "Viewport Settings"));

    builder.AddWidget(cameraButton, ToolbarAlignment::Right);

    auto toolbar = builder.Build();
    *toolbarHolder = toolbar;
    toolbar->SetActiveTool(Icons::CursorName);

    if (cameraSpeedButton) {
        SetViewportCameraSpeedIndicator(cameraSpeedButton);
    }

    return toolbar;
}

} // namespace we::programs::editor
