#pragma once

#include "KindUI/Declarative/Element.h"
#include "KindUI/Commands/ICommandRegistry.h"

#include <functional>
#include <utility>
#include <vector>

namespace we::runtime::kindui::UI {

// Chainable layout and behavior modifiers. Each returns a new Element value.

[[nodiscard]] inline Element Id(Element e, std::string id) {
    e.id = std::move(id);
    return e;
}

[[nodiscard]] inline Element Style(Element e, std::string styleClass) {
    e.styleClass = std::move(styleClass);
    return e;
}

[[nodiscard]] inline Element Variant(Element e, WidgetVariant variant) {
    e.variant = variant;
    return e;
}

[[nodiscard]] inline Element Fill(Element e) {
    e.layout.fill = true;
    e.layout.flexGrow = 1.0f;
    return e;
}

[[nodiscard]] inline Element Grow(Element e, float flexGrow = 1.0f) {
    e.layout.flexGrow = flexGrow;
    return e;
}

[[nodiscard]] inline Element Shrink(Element e, float flexShrink = 1.0f) {
    e.layout.flexShrink = flexShrink;
    return e;
}

[[nodiscard]] inline Element Gap(Element e, float gap) {
    e.layout.gap = gap;
    return e;
}

[[nodiscard]] inline Element Width(Element e, float width) {
    e.width = width;
    e.layout.minWidth = width;
    e.layout.maxWidth = width;
    return e;
}

[[nodiscard]] inline Element Height(Element e, float height) {
    e.height = height;
    e.layout.minHeight = height;
    e.layout.maxHeight = height;
    return e;
}

[[nodiscard]] inline Element MinWidth(Element e, float width) {
    e.layout.minWidth = width;
    return e;
}

[[nodiscard]] inline Element MinHeight(Element e, float height) {
    e.layout.minHeight = height;
    return e;
}

[[nodiscard]] inline Element MaxWidth(Element e, float width) {
    e.layout.maxWidth = width;
    return e;
}

[[nodiscard]] inline Element MaxHeight(Element e, float height) {
    e.layout.maxHeight = height;
    return e;
}

[[nodiscard]] inline Element Margin(Element e, float left, float top, float right, float bottom) {
    e.layout.marginLeft = left;
    e.layout.marginTop = top;
    e.layout.marginRight = right;
    e.layout.marginBottom = bottom;
    return e;
}

[[nodiscard]] inline Element Padding(Element e, float left, float top, float right, float bottom) {
    e.layout.paddingLeft = left;
    e.layout.paddingTop = top;
    e.layout.paddingRight = right;
    e.layout.paddingBottom = bottom;
    return e;
}

[[nodiscard]] inline Element AlignH(Element e, HorizontalAlignment align) {
    e.layout.hAlign = align;
    return e;
}

[[nodiscard]] inline Element AlignV(Element e, VerticalAlignment align) {
    e.layout.vAlign = align;
    return e;
}

[[nodiscard]] inline Element AlignStart(Element e) {
    return AlignH(std::move(e), HorizontalAlignment::Left);
}

[[nodiscard]] inline Element AlignEnd(Element e) {
    return AlignH(std::move(e), HorizontalAlignment::Right);
}

[[nodiscard]] inline Element Visible(Element e, bool visible) {
    e.visible = visible;
    return e;
}

[[nodiscard]] inline Element Enabled(Element e, bool enabled) {
    e.enabled = enabled;
    return e;
}

[[nodiscard]] inline Element OnClick(Element e, std::function<void()> handler) {
    e.events.onClicked = std::move(handler);
    return e;
}

[[nodiscard]] inline Element OnChange(Element e, std::function<void(std::string_view)> handler) {
    e.events.onValueChanged = std::move(handler);
    return e;
}

[[nodiscard]] inline Element OnChecked(Element e, std::function<void(bool)> handler) {
    e.events.onSelectionChanged = std::move(handler);
    return e;
}

[[nodiscard]] inline Element BindCommand(Element e, std::shared_ptr<ICommand> command) {
    e.events.command = std::move(command);
    if (command) {
        e.events.onClicked = [command] {
            if (command->CanExecute({})) {
                command->Execute({});
            }
        };
    }
    return e;
}

[[nodiscard]] inline Element Configure(Element e, WidgetConfigurator configure) {
    e.configure = std::move(configure);
    return e;
}

[[nodiscard]] inline Element When(bool condition, Element thenBranch) {
    thenBranch.when = [condition] { return condition; };
    return thenBranch;
}

[[nodiscard]] inline Element When(std::function<bool()> condition, Element thenBranch) {
    thenBranch.when = std::move(condition);
    return thenBranch;
}

// Collection composition — describe repeated UI from data.
template <typename Range, typename Builder>
[[nodiscard]] Element ForEach(Range&& range, Builder&& builder) {
    Element container{};
    container.type = ElementType::Column;
    container.forEach = [range = std::forward<Range>(range), builder = std::forward<Builder>(builder)]() mutable {
        std::vector<Element> items;
        std::size_t index = 0;
        for (auto& item : range) {
            Element child = builder(item, index++);
            if (!child.id.empty()) {
                child.id = child.id + "-" + std::to_string(index);
            }
            items.push_back(std::move(child));
        }
        return items;
    };
    return container;
}

[[nodiscard]] inline Element ForEach(std::function<std::vector<Element>()> producer) {
    Element container{};
    container.type = ElementType::Column;
    container.forEach = std::move(producer);
    return container;
}

// Host application-specific widgets inside declarative trees.
[[nodiscard]] inline Element Host(WidgetFactory factory) {
    Element e{};
    e.type = ElementType::Host;
    e.host = std::move(factory);
    return e;
}

} // namespace we::runtime::kindui::UI
