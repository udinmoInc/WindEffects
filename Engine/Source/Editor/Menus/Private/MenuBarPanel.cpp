#include "EditorRegistry.h"
#include "Widgets/Panel.h"
#include "Widgets/MenuBar.h"
#include "Core/Icon.h"

namespace we::programs::editor {

namespace {

// Helper to build a MenuItem with label, optional shortcut, and callback
std::shared_ptr<we::UI::MenuItem> MakeItem(
    const std::string& label,
    const std::string& shortcut = "",
    std::function<void()> onClick = nullptr)
{
    auto item = std::make_shared<we::UI::MenuItem>();
    item->label    = label;
    item->shortcut = shortcut;
    item->onClick  = onClick ? std::move(onClick) : []() {};
    return item;
}

// Separator represented as a disabled, empty-label menu item
std::shared_ptr<we::UI::MenuItem> MakeSeparator() {
    auto item = std::make_shared<we::UI::MenuItem>();
    item->label   = "";
    item->enabled = false;
    return item;
}

} // anonymous namespace

std::shared_ptr<we::UI::Panel> CreateMenuBarPanel() {
    auto panel = std::make_shared<we::UI::Panel>("MenuBar");
    panel->SetHeaderHeight(0.0f);
    panel->SetCollapsible(false);

    auto menuBar = std::make_shared<we::UI::MenuBar>();

    // --- File menu ---
    menuBar->AddMenu("File", {
        MakeItem("New",      "Ctrl+N"),
        MakeItem("Open",     "Ctrl+O"),
        MakeItem("Save",     "Ctrl+S"),
        MakeItem("Save As",  "Ctrl+Shift+S"),
        MakeSeparator(),
        MakeItem("Exit",     "Alt+F4")
    });

    // --- Edit menu ---
    menuBar->AddMenu("Edit", {
        MakeItem("Undo",  "Ctrl+Z"),
        MakeItem("Cut",   "Ctrl+X"),
        MakeItem("Copy",  "Ctrl+C"),
        MakeItem("Paste", "Ctrl+V")
    });

    // --- View menu ---
    menuBar->AddMenu("View", {
        MakeItem("Toggle Panels")
    });

    // --- Help menu ---
    menuBar->AddMenu("Help", {
        MakeItem("About")
    });

    panel->SetContent(menuBar);

    return panel;
}

REGISTER_EDITOR_PANEL(MenuBar, CreateMenuBarPanel)

} // namespace we::programs::editor
