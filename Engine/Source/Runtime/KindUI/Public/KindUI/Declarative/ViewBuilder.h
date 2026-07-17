#pragma once

#include "KindUI/Export.h"
#include "KindUI/Declarative/Element.h"
#include "KindUI/Core/Widget.h"

#include <memory>
#include <unordered_map>
#include <vector>

namespace we::runtime::kindui {

class IWidgetContext;

class KINDUI_API ViewBuilder {
public:
    explicit ViewBuilder(std::shared_ptr<IWidgetContext> context);

    [[nodiscard]] std::shared_ptr<Widget> Build(const Element& root);
    void Reconcile(Widget& existing, const Element& updated);

private:
    std::shared_ptr<IWidgetContext> m_Context;

    [[nodiscard]] Element Expand(const Element& element) const;
    [[nodiscard]] std::vector<Element> ExpandChildren(const Element& element) const;

    [[nodiscard]] std::shared_ptr<Widget> BuildElement(const Element& element);
    void ApplyElementProperties(Widget& widget, const Element& element);
    void ApplyLayoutIntent(Widget& widget, const LayoutIntent& intent);
    void ReconcileElement(Widget& existing, const Element& updated);
    void ReconcileChildren(Widget& container, const std::vector<Element>& children);
    void ReconcileScrollView(Widget& existing, const Element& expanded);

    [[nodiscard]] Widget* FindDirectChildById(Widget& parent, std::string_view id) const;
    [[nodiscard]] bool ElementsMatchForReuse(const Widget& widget, const Element& element) const;
};

} // namespace we::runtime::kindui
