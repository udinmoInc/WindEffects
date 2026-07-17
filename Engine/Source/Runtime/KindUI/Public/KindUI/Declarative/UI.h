#pragma once

// KindUI declarative DSL — describe hierarchy, structure, and behavior.
//
//   #include "KindUI/KindUI.h"
//   using namespace we::runtime::kindui::UI;
//
//   auto page = Column({
//       Section("Projects", "Recent work"),
//       SearchBar("Filter..."),
//       ForEach(projects, [](const auto& p, size_t i) {
//           return Id(ProjectRow(p), "project-" + std::to_string(i));
//       }),
//   });

#include "KindUI/Declarative/Element.h"
#include "KindUI/Declarative/Modifiers.h"
#include "KindUI/Declarative/ViewBuilder.h"
#include "KindUI/Core/IApplicationContext.h"
#include "KindUI/Core/WidgetContext.h"
#include "KindUI/Layout/IPopupHost.h"

#include <initializer_list>
#include <memory>
#include <utility>
#include <vector>

namespace we::runtime::kindui::UI {

// --- Containers ---------------------------------------------------------------

[[nodiscard]] inline Element Make(ElementType type, std::initializer_list<Element> children = {}) {
    Element e{};
    e.type = type;
    e.children.assign(children);
    return e;
}

[[nodiscard]] inline Element Row(std::initializer_list<Element> children = {}) {
    return Make(ElementType::Row, children);
}

[[nodiscard]] inline Element Row(std::vector<Element> children) {
    Element e{};
    e.type = ElementType::Row;
    e.children = std::move(children);
    return e;
}

[[nodiscard]] inline Element Column(std::initializer_list<Element> children = {}) {
    return Make(ElementType::Column, children);
}

[[nodiscard]] inline Element Column(std::vector<Element> children) {
    Element e{};
    e.type = ElementType::Column;
    e.children = std::move(children);
    return e;
}

[[nodiscard]] inline Element Flex(std::initializer_list<Element> children = {}) {
    return Make(ElementType::Flex, children);
}

[[nodiscard]] inline Element Flex(std::vector<Element> children) {
    Element e{};
    e.type = ElementType::Flex;
    e.children = std::move(children);
    return e;
}

[[nodiscard]] inline Element Scroll(std::initializer_list<Element> children = {}) {
    return Make(ElementType::ScrollView, children);
}

[[nodiscard]] inline Element Scroll(std::vector<Element> children) {
    Element e{};
    e.type = ElementType::ScrollView;
    e.children = std::move(children);
    return e;
}

[[nodiscard]] inline Element Card(std::initializer_list<Element> children = {}) {
    return Make(ElementType::Card, children);
}

[[nodiscard]] inline Element Toolbar(std::initializer_list<Element> children = {}) {
    return Make(ElementType::Toolbar, children);
}

// --- Widgets ------------------------------------------------------------------

[[nodiscard]] inline Element Label(std::string text) {
    Element e{};
    e.type = ElementType::Label;
    e.text = std::move(text);
    return e;
}

[[nodiscard]] inline Element Section(std::string title, std::string subtitle = {}) {
    Element e{};
    e.type = ElementType::SectionHeader;
    e.text = std::move(title);
    e.subtitle = std::move(subtitle);
    return e;
}

[[nodiscard]] inline Element Button(std::string text, WidgetVariant variant = WidgetVariant::Primary) {
    Element e{};
    e.type = ElementType::Button;
    e.text = std::move(text);
    e.variant = variant;
    return e;
}

[[nodiscard]] inline Element TextBox(std::string placeholder = {}) {
    Element e{};
    e.type = ElementType::TextBox;
    e.placeholder = std::move(placeholder);
    return e;
}

[[nodiscard]] inline Element SearchBar(std::string placeholder = "Search...") {
    Element e{};
    e.type = ElementType::SearchBox;
    e.placeholder = std::move(placeholder);
    return e;
}

[[nodiscard]] inline Element CheckBox(std::string label, bool checked = false) {
    Element e{};
    e.type = ElementType::CheckBox;
    e.text = std::move(label);
    e.checked = checked;
    return e;
}

[[nodiscard]] inline Element EmptyState(std::string title, std::string subtitle = {}) {
    Element e{};
    e.type = ElementType::EmptyState;
    e.text = std::move(title);
    e.subtitle = std::move(subtitle);
    return e;
}

[[nodiscard]] inline Element Spacer(float flexGrow = 1.0f) {
    Element e{};
    e.type = ElementType::Spacer;
    e.layout.flexGrow = flexGrow;
    return e;
}

[[nodiscard]] inline Element Skeleton() {
    Element e{};
    e.type = ElementType::Skeleton;
    return e;
}

[[nodiscard]] inline Element Dialog(std::string title, std::initializer_list<Element> children = {}) {
    Element e{};
    e.type = ElementType::Dialog;
    e.text = std::move(title);
    e.children.assign(children);
    return e;
}

[[nodiscard]] inline Element Dialog(std::string title, std::vector<Element> children) {
    Element e{};
    e.type = ElementType::Dialog;
    e.text = std::move(title);
    e.children = std::move(children);
    return e;
}

[[nodiscard]] inline Element Menu(std::initializer_list<Element> items = {}) {
    return Make(ElementType::Menu, items);
}

[[nodiscard]] inline Element Menu(std::vector<Element> items) {
    Element e{};
    e.type = ElementType::Menu;
    e.children = std::move(items);
    return e;
}

[[nodiscard]] inline Element MenuItem(
    std::string label,
    std::shared_ptr<ICommand> command = {},
    const char* icon = nullptr) {
    Element e{};
    e.type = ElementType::MenuItem;
    e.text = std::move(label);
    if (icon) {
        e.icon = icon;
    }
    e.events.command = std::move(command);
    return e;
}

[[nodiscard]] inline Element MenuSeparator() {
    Element e{};
    e.type = ElementType::MenuSeparator;
    return e;
}

// --- Build helpers ------------------------------------------------------------

[[nodiscard]] inline std::shared_ptr<Widget> View(
    IApplicationContext& app,
    const Element& root,
    IPopupHost* popupHost = nullptr)
{
    auto context = std::make_shared<WidgetContext>(app, popupHost);
    ViewBuilder builder(context);
    return builder.Build(root);
}

[[nodiscard]] inline std::shared_ptr<IWidgetContext> MakeContext(
    IApplicationContext& app,
    IPopupHost* popupHost = nullptr)
{
    return std::make_shared<WidgetContext>(app, popupHost);
}

} // namespace we::runtime::kindui::UI
