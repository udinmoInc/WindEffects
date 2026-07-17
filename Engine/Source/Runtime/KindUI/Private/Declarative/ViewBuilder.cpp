#include "KindUI/Declarative/ViewBuilder.h"

#include "KindUI/Core/WidgetContext.h"
#include "KindUI/Layout/Flex.h"
#include "KindUI/Layout/Spacer.h"
#include "KindUI/Widgets/Button.h"
#include "KindUI/Widgets/Label.h"
#include "KindUI/Widgets/TextBox.h"

namespace we::runtime::kindui {

ViewBuilder::ViewBuilder(std::shared_ptr<IWidgetContext> context)
    : m_Context(std::move(context)) {}

std::shared_ptr<Widget> ViewBuilder::Build(const Element& root) {
    return BuildElement(root);
}

void ViewBuilder::Reconcile(Widget& existing, const Element& updated) {
    existing.SetVisible(updated.visible);
    existing.SetEnabled(updated.enabled);
    if (!updated.styleClass.empty()) {
        existing.SetStyleClass(std::string(updated.styleClass));
    }
    // Full structural reconciliation is expanded as element types grow.
    (void)updated;
}

void ViewBuilder::ApplyLayoutIntent(Widget& widget, const LayoutIntent& intent) {
    widget.SetFlexGrow(intent.flexGrow);
    widget.SetFlexShrink(intent.flexShrink);
    widget.SetFlexBasis(intent.flexBasis);
    widget.SetMinSize({intent.minWidth, intent.minHeight});
    widget.SetMaxSize({intent.maxWidth, intent.maxHeight});
}

void ViewBuilder::ApplyEvents(Widget& widget, const ElementEvents& events) {
    (void)widget;
    (void)events;
    // Event wiring is delegated to concrete widget types during BuildElement.
}

std::shared_ptr<Widget> ViewBuilder::BuildElement(const Element& element) {
    std::shared_ptr<Widget> widget;

    switch (element.type) {
    case ElementType::Row: {
        auto row = std::make_shared<Row>();
        if (element.layout.gap >= 0.0f) {
            row->Gap(element.layout.gap);
        }
        for (const Element& child : element.children) {
            row->AddChild(BuildElement(child));
        }
        widget = std::move(row);
        break;
    }
    case ElementType::Column: {
        auto col = std::make_shared<Column>();
        if (element.layout.gap >= 0.0f) {
            col->Gap(element.layout.gap);
        }
        for (const Element& child : element.children) {
            col->AddChild(BuildElement(child));
        }
        widget = std::move(col);
        break;
    }
    case ElementType::Button: {
        auto btn = std::make_shared<Button>(element.text);
        if (element.events.onClicked) {
            btn->SetOnClicked(element.events.onClicked);
        }
        widget = std::move(btn);
        break;
    }
    case ElementType::Label: {
        widget = std::make_shared<Label>(element.text);
        break;
    }
    case ElementType::TextBox: {
        auto box = std::make_shared<TextBox>(element.placeholder.value_or(""));
        widget = std::move(box);
        break;
    }
    case ElementType::Spacer: {
        widget = std::make_shared<Spacer>();
        break;
    }
    default: {
        widget = std::make_shared<Row>();
        break;
    }
    }

    if (widget) {
        widget->SetContext(m_Context);
        widget->SetVisible(element.visible);
        widget->SetEnabled(element.enabled);
        if (!element.styleClass.empty()) {
            widget->SetStyleClass(std::string(element.styleClass));
        }
        ApplyLayoutIntent(*widget, element.layout);
    }

    return widget;
}

} // namespace we::runtime::kindui
