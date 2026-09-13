// ==============================================================================
// WindEffects — KindUI — EditorUI
// ONE public entry point / facade for building editor UI.
//
// Usage:
//   #include <KindUI/EditorUI.h>
//   auto panel = EditorUI::Panel("Details");
//   auto search = EditorUI::SearchBox("Filter...");
//   auto btn = EditorUI::Button("Apply");
//
// Factories live in namespace EditorUI. Canonical widget types remain in
// we::runtime::kindui (and panels::) — this header includes them so one include
// is enough. No duplicate widget implementations.
//
// Not faked here (stay editor-local):
//   ContentBrowser::TreeView, Toolbar::ToolButton, PropertyEditor fields,
//   Viewport/MainFrame/PlaceActors domain shells.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "KindUI/Export.h"

// --- Core widget base and types ---
#include "KindUI/Core/Widget.h"
#include "KindUI/Core/Types.h"
#include "KindUI/Core/Geometry.h"
#include "KindUI/Core/Style.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Core/DPIContext.h"
#include "KindUI/Core/InteractionState.h"
#include "KindUI/Core/WidgetContext.h"
#include "KindUI/Core/Animator.h"
#include "KindUI/Core/Expansion.h"
#include "KindUI/Core/EventSystem.h"
#include "KindUI/Core/UIRepaintGate.h"

// --- App / services (editor shell bootstrap) ---
#include "KindUI/Core/IApplicationContext.h"
#include "KindUI/Core/ApplicationContext.h"
#include "KindUI/Core/ServiceContainer.h"
#include "KindUI/Core/IServiceProvider.h"
#include "KindUI/Theming/IKindUITheme.h"
#include "KindUI/Theming/DefaultTheme.h"
#include "KindUI/Theming/ThemeManager.h"
#include "KindUI/Theming/GraphiteDarkTheme.h"
#include "KindUI/Resources/IResourceRegistry.h"
#include "KindUI/Events/IEventBus.h"
#include "KindUI/Commands/ICommandRegistry.h"

// --- Icons / theming / tokens ---
#include "KindUI/Core/WindIcon.h"
#include "KindUI/Core/Icon.h"
#include "KindUI/Theming/ThemeAccess.h"
#include "KindUI/Theming/StyleRole.h"
#include "KindUI/Theming/Palette.h"
#include "KindUI/Theming/PaletteRuntime.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Tokens/DesignSystem.h"
#include "KindUI/Tokens/SurfaceRole.h"
#include "KindUI/Tokens/ChromeSeparation.h"

// --- Layout ---
#include "KindUI/Layout/Flex.h"
#include "KindUI/Layout/Spacer.h"
#include "KindUI/Layout/Grid.h"
#include "KindUI/Layout/Splitter.h"
#include "KindUI/Layout/ScrollViewport.h"
#include "KindUI/Layout/ScrollLayout.h"
#include "KindUI/Layout/CollapsibleGroup.h"
#include "KindUI/Layout/PropertyRowLayout.h"
#include "KindUI/Layout/OverlayManager.h"
#include "KindUI/Layout/IPopupHost.h"
#include "KindUI/Layout/AutoAlign.h"

// --- Panel system ---
#include "KindUI/Panel/Panel.h"
#include "KindUI/Panel/PanelBuilder.h"
#include "KindUI/Panel/PanelChrome.h"
#include "KindUI/Panel/PanelBodyLayout.h"
#include "KindUI/Panel/PanelModeTabs.h"

// --- Shared widgets ---
#include "KindUI/Widgets/Label.h"
#include "KindUI/Widgets/TextBox.h"
#include "KindUI/Widgets/CheckBox.h"
#include "KindUI/Widgets/ColorPicker.h"
#include "KindUI/Widgets/Components.h"
#include "KindUI/Widgets/FilterTabStrip.h"
#include "KindUI/Widgets/CompactTreeWidget.h"
#include "KindUI/Widgets/ScrollContainer.h"
#include "KindUI/Widgets/ObjectTitleBar.h"
#include "KindUI/Widgets/DropdownMenu.h"
#include "KindUI/Widgets/MenuBar.h"
#include "KindUI/Widgets/Breadcrumb.h"
#include "KindUI/Widgets/TreeColumnHeader.h"
#include "KindUI/Widgets/FormSectionTitle.h"
#include "KindUI/Widgets/PropertyResetButton.h"
#include "KindUI/Widgets/VirtualList.h"
#include "KindUI/Widgets/ModalHost.h"
#include "KindUI/Widgets/RichTextView.h"
#include "KindUI/Widgets/ScreenDebugOverlay.h"

#include "KindUI/Core/Widgets/DesignSystemControls.h"
#include "KindUI/Core/Widgets/PanelToolbarRow.h"
#include "KindUI/Core/Widgets/ToolbarIconButton.h"
#include "KindUI/Core/Widgets/ToolbarNavigationButton.h"
#include "KindUI/Core/Widgets/ToolbarGlyphButton.h"
#include "KindUI/Core/Widgets/VerticalDivider.h"

// --- Public chrome / metrics helpers (compose editor chrome without private headers) ---
#include "KindUI/Core/ControlChrome.h"
#include "KindUI/Core/ToolbarButtonChrome.h"
#include "KindUI/Core/PropertyPanelChrome.h"
#include "KindUI/Core/PropertyColumnSplitter.h"
#include "KindUI/Core/LayoutMetrics.h"
#include "KindUI/Core/ColorSpace.h"
#include "KindUI/Core/TextMetrics.h"
#include "KindUI/Core/UiMetrics.h"
#include "KindUI/Rendering/IconMetrics.h"

// --- Input ---
#include "KindUI/Input/InputEvents.h"
#include "KindUI/Input/HotkeyManager.h"

// --- Editor declarative DSL ---
#include "KindUI/DSL/EditorDSL.h"

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace EditorUI {

// ---------------------------------------------------------------------------
// Factories — thin wrappers over canonical KindUI types / makers.
// Type names stay in we::runtime::kindui to avoid colliding with factories.
// ---------------------------------------------------------------------------

[[nodiscard]] inline ::we::runtime::kindui::panels::PanelBuilder Panel(
    std::string_view title = "") {
    return ::we::runtime::kindui::panels::PanelBuilder::Create(title);
}

[[nodiscard]] inline ::we::runtime::kindui::panels::PanelBuilder Panel(const char* title) {
    return ::we::runtime::kindui::panels::PanelBuilder::Create(title);
}

[[nodiscard]] inline ::we::runtime::kindui::panels::PanelBuilder Panel(const std::string& title) {
    return ::we::runtime::kindui::panels::PanelBuilder::Create(title);
}

[[nodiscard]] inline std::shared_ptr<::we::runtime::kindui::PrimaryButton> Button(
    std::string label,
    ::we::runtime::kindui::WindIconRef icon = ::we::runtime::kindui::kWindIconNone) {
    return ::we::runtime::kindui::MakePrimaryAction(std::move(label), icon);
}

[[nodiscard]] inline std::shared_ptr<::we::runtime::kindui::PrimaryButton> Primary(
    std::string label,
    ::we::runtime::kindui::WindIconRef icon = ::we::runtime::kindui::kWindIconNone) {
    return Button(std::move(label), icon);
}

[[nodiscard]] inline std::shared_ptr<::we::runtime::kindui::SecondaryButton> Secondary(
    std::string label,
    ::we::runtime::kindui::WindIconRef icon = ::we::runtime::kindui::kWindIconNone) {
    return ::we::runtime::kindui::MakeSecondaryAction(std::move(label), icon);
}

/// KindUI design-system toolbar button (not Editor/Toolbar::ToolButton).
[[nodiscard]] inline std::shared_ptr<::we::runtime::kindui::ToolbarButton> ToolButton(
    std::string label,
    ::we::runtime::kindui::WindIconRef icon = ::we::runtime::kindui::kWindIconNone) {
    return std::make_shared<::we::runtime::kindui::ToolbarButton>(std::move(label), icon);
}

[[nodiscard]] inline std::shared_ptr<::we::runtime::kindui::ToolbarButton> ToolbarLabeledButton(
    std::string label,
    ::we::runtime::kindui::WindIconRef icon = ::we::runtime::kindui::kWindIconNone) {
    return ToolButton(std::move(label), icon);
}

[[nodiscard]] inline std::shared_ptr<::we::runtime::kindui::GhostButton> Ghost(
    std::string label,
    ::we::runtime::kindui::WindIconRef icon = ::we::runtime::kindui::kWindIconNone) {
    return std::make_shared<::we::runtime::kindui::GhostButton>(std::move(label), icon);
}

[[nodiscard]] inline std::shared_ptr<::we::runtime::kindui::DangerButton> Danger(
    std::string label,
    ::we::runtime::kindui::WindIconRef icon = ::we::runtime::kindui::kWindIconNone) {
    return std::make_shared<::we::runtime::kindui::DangerButton>(std::move(label), icon);
}

[[nodiscard]] inline std::shared_ptr<::we::runtime::kindui::IconButton> Icon(
    ::we::runtime::kindui::WindIconRef icon) {
    return std::make_shared<::we::runtime::kindui::IconButton>(icon);
}

[[nodiscard]] inline std::shared_ptr<::we::runtime::kindui::ToolbarIconButton> ToolbarIcon(
    ::we::runtime::kindui::WindIconRef icon,
    const char* tooltip = nullptr) {
    return std::make_shared<::we::runtime::kindui::ToolbarIconButton>(icon, tooltip);
}

[[nodiscard]] inline std::shared_ptr<::we::runtime::kindui::ToolbarNavigationButton> ToolbarNav(
    ::we::runtime::kindui::WindIconRef icon,
    const char* tooltip = nullptr) {
    return std::make_shared<::we::runtime::kindui::ToolbarNavigationButton>(icon, tooltip);
}

[[nodiscard]] inline std::shared_ptr<::we::runtime::kindui::SearchBoxControl> SearchBox(
    std::string placeholder = "Search...") {
    return ::we::runtime::kindui::MakeSearchBar(std::move(placeholder));
}

[[nodiscard]] inline std::shared_ptr<::we::runtime::kindui::SearchBoxControl> Search(
    std::string placeholder = "Search...") {
    return SearchBox(std::move(placeholder));
}

[[nodiscard]] inline std::shared_ptr<::we::runtime::kindui::PanelToolbarRow> PanelToolbar() {
    return std::make_shared<::we::runtime::kindui::PanelToolbarRow>();
}

[[nodiscard]] inline std::shared_ptr<::we::runtime::kindui::PanelTab> Tab(std::string label) {
    return ::we::runtime::kindui::MakePanelTab(std::move(label));
}

[[nodiscard]] inline std::shared_ptr<::we::runtime::kindui::FilterTabStrip> Tabs() {
    return std::make_shared<::we::runtime::kindui::FilterTabStrip>();
}

[[nodiscard]] inline std::shared_ptr<::we::runtime::kindui::FilterTabStrip> FilterTabs() {
    return Tabs();
}

/// KindUI compact tree (not ContentBrowser::TreeView).
[[nodiscard]] inline std::shared_ptr<::we::runtime::kindui::CompactTreeWidget> CompactTree() {
    return std::make_shared<::we::runtime::kindui::CompactTreeWidget>();
}

[[nodiscard]] inline std::shared_ptr<::we::runtime::kindui::CompactTreeWidget> Tree() {
    return CompactTree();
}

[[nodiscard]] inline std::shared_ptr<::we::runtime::kindui::TreeColumnHeader> TreeColumns() {
    return std::make_shared<::we::runtime::kindui::TreeColumnHeader>();
}

[[nodiscard]] inline std::shared_ptr<::we::runtime::kindui::FormSectionTitle> SectionTitle(
    std::string title,
    bool leadingGap = false) {
    return std::make_shared<::we::runtime::kindui::FormSectionTitle>(std::move(title), leadingGap);
}

[[nodiscard]] inline std::shared_ptr<::we::runtime::kindui::CollapsibleGroup> Section(
    std::string title = "",
    bool expanded = true) {
    return std::make_shared<::we::runtime::kindui::CollapsibleGroup>(std::move(title), expanded);
}

[[nodiscard]] inline std::shared_ptr<::we::runtime::kindui::CollapsibleGroup> Collapsible(
    std::string title = "",
    bool expanded = true) {
    return Section(std::move(title), expanded);
}

[[nodiscard]] inline std::shared_ptr<::we::runtime::kindui::ObjectTitleBar> ObjectTitle(
    std::string title = "",
    ::we::runtime::kindui::WindIconRef icon = ::we::runtime::kindui::kWindIconNone) {
    return std::make_shared<::we::runtime::kindui::ObjectTitleBar>(std::move(title), icon);
}

/// Property row chrome. Typed PropertyEditor fields stay in PropertyEditor.
[[nodiscard]] inline std::shared_ptr<::we::runtime::kindui::PropertyRow> Property(
    std::string label = "",
    std::string value = "") {
    return std::make_shared<::we::runtime::kindui::PropertyRow>(std::move(label), std::move(value));
}

[[nodiscard]] inline std::shared_ptr<::we::runtime::kindui::Label> Label(std::string text = "") {
    return std::make_shared<::we::runtime::kindui::Label>(std::move(text));
}

[[nodiscard]] inline std::shared_ptr<::we::runtime::kindui::TextBox> TextBox(std::string text = "") {
    return std::make_shared<::we::runtime::kindui::TextBox>(std::move(text));
}

[[nodiscard]] inline std::shared_ptr<::we::runtime::kindui::CheckBox> CheckBox(
    std::string label,
    bool checked = false) {
    return std::make_shared<::we::runtime::kindui::CheckBox>(std::move(label), checked);
}

[[nodiscard]] inline std::shared_ptr<::we::runtime::kindui::ColorPicker> ColorPicker() {
    return std::make_shared<::we::runtime::kindui::ColorPicker>();
}

[[nodiscard]] inline std::shared_ptr<::we::runtime::kindui::MenuItem> MenuItem() {
    return std::make_shared<::we::runtime::kindui::MenuItem>();
}

[[nodiscard]] inline std::shared_ptr<::we::runtime::kindui::DropdownMenu> Dropdown(
    const std::vector<std::shared_ptr<::we::runtime::kindui::MenuItem>>& items = {}) {
    return std::make_shared<::we::runtime::kindui::DropdownMenu>(items);
}

[[nodiscard]] inline std::shared_ptr<::we::runtime::kindui::MenuBar> Menu() {
    return std::make_shared<::we::runtime::kindui::MenuBar>();
}

[[nodiscard]] inline std::shared_ptr<::we::runtime::kindui::Breadcrumb> Breadcrumb() {
    return std::make_shared<::we::runtime::kindui::Breadcrumb>();
}

[[nodiscard]] inline std::shared_ptr<::we::runtime::kindui::Splitter> Splitter(
    ::we::runtime::kindui::Orientation orientation = ::we::runtime::kindui::Orientation::Horizontal,
    float initialRatio = 0.5f) {
    return std::make_shared<::we::runtime::kindui::Splitter>(orientation, initialRatio);
}

[[nodiscard]] inline std::shared_ptr<::we::runtime::kindui::ScrollContainer> Scroll(
    std::shared_ptr<::we::runtime::kindui::Widget> content = nullptr) {
    return std::make_shared<::we::runtime::kindui::ScrollContainer>(std::move(content));
}

[[nodiscard]] inline std::shared_ptr<::we::runtime::kindui::Row> Row() {
    return ::we::runtime::kindui::MakeRow();
}

[[nodiscard]] inline std::shared_ptr<::we::runtime::kindui::Column> Column() {
    return ::we::runtime::kindui::MakeColumn();
}

[[nodiscard]] inline std::shared_ptr<::we::runtime::kindui::Spacer> Spacer() {
    return std::make_shared<::we::runtime::kindui::Spacer>();
}

[[nodiscard]] inline std::shared_ptr<::we::runtime::kindui::Grid> Grid() {
    return ::we::runtime::kindui::MakeGrid();
}

[[nodiscard]] inline std::shared_ptr<::we::runtime::kindui::VerticalDivider> Divider() {
    return std::make_shared<::we::runtime::kindui::VerticalDivider>();
}

[[nodiscard]] inline std::shared_ptr<::we::runtime::kindui::EmptyState> EmptyState(
    std::string title,
    std::string subtitle = {}) {
    return ::we::runtime::kindui::MakeEmptyState(std::move(title), std::move(subtitle));
}

[[nodiscard]] inline std::shared_ptr<::we::runtime::kindui::StatusBadge> Badge(std::string text) {
    return ::we::runtime::kindui::MakeStatusBadge(std::move(text));
}

[[nodiscard]] inline std::shared_ptr<::we::runtime::kindui::Card> Card() {
    return ::we::runtime::kindui::MakeCard();
}

[[nodiscard]] inline std::shared_ptr<::we::runtime::kindui::VirtualList> VirtualList() {
    return ::we::runtime::kindui::MakeVirtualList();
}

[[nodiscard]] inline std::shared_ptr<::we::runtime::kindui::ModalHost> Modal() {
    return ::we::runtime::kindui::MakeModalHost();
}

} // namespace EditorUI

namespace we::editor {
namespace ui = ::EditorUI;
} // namespace we::editor
