#pragma once

#include "KindUI/Export.h"
#include "KindUI/Declarative/Element.h"
#include "KindUI/Core/Widget.h"

#include <memory>

namespace we::runtime::kindui {

class IWidgetContext;

// Builds and reconciles declarative element trees into retained widget instances.
class KINDUI_API ViewBuilder {
public:
    explicit ViewBuilder(std::shared_ptr<IWidgetContext> context);

    // Build a fresh widget tree from an element description.
    [[nodiscard]] std::shared_ptr<Widget> Build(const Element& root);

    // Reconcile an existing widget tree with an updated element description.
    void Reconcile(Widget& existing, const Element& updated);

private:
    std::shared_ptr<IWidgetContext> m_Context;

    [[nodiscard]] std::shared_ptr<Widget> BuildElement(const Element& element);
    void ApplyLayoutIntent(Widget& widget, const LayoutIntent& intent);
    void ApplyEvents(Widget& widget, const ElementEvents& events);
};

// Fluent declarative helpers for concise composition.
namespace UI {

inline Element Row(std::vector<Element> children = {}) {
    Element e{};
    e.type = ElementType::Row;
    e.children = std::move(children);
    return e;
}

inline Element Column(std::vector<Element> children = {}) {
    Element e{};
    e.type = ElementType::Column;
    e.children = std::move(children);
    return e;
}

inline Element Button(std::string text, WidgetVariant variant = WidgetVariant::Primary) {
    Element e{};
    e.type = ElementType::Button;
    e.text = std::move(text);
    e.variant = variant;
    return e;
}

inline Element Label(std::string text) {
    Element e{};
    e.type = ElementType::Label;
    e.text = std::move(text);
    return e;
}

inline Element TextBox(std::string placeholder = {}) {
    Element e{};
    e.type = ElementType::TextBox;
    e.placeholder = std::move(placeholder);
    return e;
}

inline Element Spacer(float flexGrow = 1.0f) {
    Element e{};
    e.type = ElementType::Spacer;
    e.layout.flexGrow = flexGrow;
    return e;
}

inline Element& WithId(Element& e, std::string id) {
    e.id = std::move(id);
    return e;
}

inline Element& WithStyle(Element& e, std::string styleClass) {
    e.styleClass = std::move(styleClass);
    return e;
}

inline Element& OnClick(Element& e, std::function<void()> handler) {
    e.events.onClicked = std::move(handler);
    return e;
}

} // namespace UI

} // namespace we::runtime::kindui
