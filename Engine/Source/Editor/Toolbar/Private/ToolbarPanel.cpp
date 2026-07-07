#include "EditorRegistry.h"
#include "Widgets/Panel.h"
#include "Widgets/Toolbar.h"
#include "Core/Icon.h"

namespace we::programs::editor {

std::shared_ptr<we::UI::Panel> CreateToolbarPanel() {
    auto panel = std::make_shared<we::UI::Panel>("Toolbar");
    panel->SetHeaderHeight(0.0f);
    panel->SetCollapsible(false);

    auto toolbar = std::make_shared<we::UI::Toolbar>();
    toolbar->SetHeight(30.0f);
    toolbar->SetIconSize(16.0f);

    // ── Left: transform tools ─────────────────────────────────────────────────
    toolbar->AddTool(we::UI::Icons::CursorName, "", []() {}, "Select (Q)");
    toolbar->AddTool(we::UI::Icons::MoveName,   "", []() {}, "Move (W)");
    toolbar->AddTool(we::UI::Icons::RotateName, "", []() {}, "Rotate (E)");
    toolbar->AddTool(we::UI::Icons::ScaleName,  "", []() {}, "Scale (R)");

    // ── Center: transport controls (Play / Pause / Stop) ─────────────────────
    // All three use TransportButton style: equal 30×30, 20px icon, no label
    auto playBtn  = toolbar->AddTool(we::UI::Icons::PlayName,  "", []() {}, "Play (Alt+P)",  false, we::UI::ToolbarAlignment::Center);
    auto pauseBtn = toolbar->AddTool(we::UI::Icons::PauseName, "", []() {}, "Pause (Alt+P)", false, we::UI::ToolbarAlignment::Center);
    auto stopBtn  = toolbar->AddTool(we::UI::Icons::StopName,  "", []() {}, "Stop",          false, we::UI::ToolbarAlignment::Center);
    playBtn->SetButtonStyle(we::UI::ToolButtonStyle::TransportButton);
    pauseBtn->SetButtonStyle(we::UI::ToolButtonStyle::TransportButton);
    stopBtn->SetButtonStyle(we::UI::ToolButtonStyle::TransportButton);

    // ── Right: platform | settings (inline, title bar hosts these in Editor.cpp) ─
    auto platformBtn = toolbar->AddTool(we::UI::Icons::PackageName, "Platform", []() {}, "Platform", false, we::UI::ToolbarAlignment::Right);
    platformBtn->SetButtonStyle(we::UI::ToolButtonStyle::ToolbarInline);

    auto settingsBtn = toolbar->AddTool(we::UI::Icons::SettingsName, "Settings", []() {}, "Editor Settings", false, we::UI::ToolbarAlignment::Right);
    settingsBtn->SetButtonStyle(we::UI::ToolButtonStyle::ToolbarInline);

    panel->SetContent(toolbar);

    return panel;
}


REGISTER_EDITOR_PANEL(Toolbar, CreateToolbarPanel)

} // namespace we::programs::editor
