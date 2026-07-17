#include "KindUI/Declarative/ViewBuilder.h"

#include "KindUI/Commands/ICommandRegistry.h"
#include "KindUI/Core/Widgets/DesignSystemControls.h"
#include "KindUI/Layout/Flex.h"
#include "KindUI/Layout/ScrollLayout.h"
#include "KindUI/Layout/Spacer.h"
#include "KindUI/Widgets/Button.h"
#include "KindUI/Widgets/CheckBox.h"
#include "KindUI/Widgets/Components.h"
#include "KindUI/Widgets/Label.h"
#include "KindUI/Widgets/TextBox.h"

#include <algorithm>

namespace we::runtime::kindui {
namespace {

std::shared_ptr<DesignButton> MakeDesignButton(const Element& element) {
    const char* icon = element.icon.empty() ? nullptr : element.icon.c_str();
    switch (element.variant) {
    case WidgetVariant::Primary:
    case WidgetVariant::Accent:
    case WidgetVariant::Success:
        return std::make_shared<PrimaryButton>(element.text, icon);
    case WidgetVariant::Danger:
    case WidgetVariant::Warning:
        return std::make_shared<DangerButton>(element.text, icon);
    case WidgetVariant::Ghost:
    case WidgetVariant::Flat:
    case WidgetVariant::Link:
        return std::make_shared<GhostButton>(element.text, icon);
    case WidgetVariant::Toolbar:
        return std::make_shared<ToolbarButton>(element.text, icon);
    case WidgetVariant::Secondary:
    case WidgetVariant::Outline:
    default:
        return std::make_shared<SecondaryButton>(element.text, icon);
    }
}

void WireButtonEvents(Widget& widget, const Element& element) {
    if (auto* design = dynamic_cast<DesignButton*>(&widget)) {
        if (element.events.onClicked) {
            design->SetOnClicked(element.events.onClicked);
        } else if (element.events.command) {
            const auto command = element.events.command;
            design->SetOnClicked([command] {
                if (command && command->CanExecute({})) {
                    command->Execute({});
                }
            });
        }
    }
}

template <typename ContainerT>
std::shared_ptr<ContainerT> MakeContainer(const Element& element) {
    auto container = std::make_shared<ContainerT>();
    if (element.layout.gap >= 0.0f) {
        container->Gap(element.layout.gap);
    }
    return container;
}

} // namespace

ViewBuilder::ViewBuilder(std::shared_ptr<IWidgetContext> context)
    : m_Context(std::move(context)) {}

Element ViewBuilder::Expand(const Element& element) const {
    Element expanded = element;
    if (element.when && !element.when()) {
        expanded.visible = false;
        expanded.children.clear();
        return expanded;
    }
    if (element.forEach) {
        expanded.children = element.forEach();
    }
    std::vector<Element> resolved;
    resolved.reserve(expanded.children.size());
    for (const Element& child : expanded.children) {
        resolved.push_back(Expand(child));
    }
    expanded.children = std::move(resolved);
    return expanded;
}

std::vector<Element> ViewBuilder::ExpandChildren(const Element& element) const {
    return Expand(element).children;
}

std::shared_ptr<Widget> ViewBuilder::Build(const Element& root) {
    return BuildElement(Expand(root));
}

void ViewBuilder::ApplyLayoutIntent(Widget& widget, const LayoutIntent& intent) {
    widget.SetFlexGrow(intent.flexGrow);
    widget.SetFlexShrink(intent.flexShrink);
    widget.SetFlexBasis(intent.flexBasis);
    widget.SetMinSize({ intent.minWidth, intent.minHeight });
    widget.SetMaxSize({ intent.maxWidth, intent.maxHeight });
    if (intent.fill) {
        widget.SetHorizontalAlignment(HorizontalAlignment::Fill);
        widget.SetVerticalAlignment(VerticalAlignment::Fill);
    }
    if (intent.hAlign) {
        widget.SetHorizontalAlignment(*intent.hAlign);
    }
    if (intent.vAlign) {
        widget.SetVerticalAlignment(*intent.vAlign);
    }
    const bool hasMargin = intent.marginLeft != 0.0f || intent.marginTop != 0.0f
        || intent.marginRight != 0.0f || intent.marginBottom != 0.0f;
    if (hasMargin) {
        widget.SetMargin({
            intent.marginLeft,
            intent.marginTop,
            intent.marginRight,
            intent.marginBottom,
        });
    }
    if (auto* flex = dynamic_cast<Flex*>(&widget)) {
        if (intent.paddingLeft >= 0.0f) {
            flex->Padding({
                intent.paddingLeft,
                intent.paddingTop >= 0.0f ? intent.paddingTop : 0.0f,
                intent.paddingRight >= 0.0f ? intent.paddingRight : 0.0f,
                intent.paddingBottom >= 0.0f ? intent.paddingBottom : 0.0f,
            });
        }
    }
}

void ViewBuilder::ApplyElementProperties(Widget& widget, const Element& element) {
    widget.SetContext(m_Context);
    widget.SetVisible(element.visible);
    widget.SetEnabled(element.enabled);
    if (!element.styleClass.empty()) {
        widget.SetStyleClass(element.styleClass);
    }
    ApplyLayoutIntent(widget, element.layout);
    if (element.width.has_value() || element.height.has_value()) {
        widget.SetMinSize({
            element.width.value_or(widget.GetMinSize().width),
            element.height.value_or(widget.GetMinSize().height),
        });
    }
    if (element.configure) {
        element.configure(widget);
    }
}

Widget* ViewBuilder::FindDirectChildById(Widget& parent, std::string_view id) const {
    if (id.empty()) {
        return nullptr;
    }
    for (const auto& child : parent.GetChildren()) {
        if (child && child->GetId() == id) {
            return child.get();
        }
    }
    return nullptr;
}

bool ViewBuilder::ElementsMatchForReuse(const Widget& widget, const Element& element) const {
    if (!element.id.empty() && widget.GetId() == element.id) {
        return true;
    }
    return element.id.empty();
}

void ViewBuilder::ReconcileElement(Widget& existing, const Element& updated) {
    const Element expanded = Expand(updated);
    existing.SetVisible(expanded.visible);
    existing.SetEnabled(expanded.enabled);
    if (!expanded.styleClass.empty()) {
        existing.SetStyleClass(expanded.styleClass);
    }
    ApplyLayoutIntent(existing, expanded.layout);
    if (expanded.configure) {
        expanded.configure(existing);
    }
}

void ViewBuilder::ReconcileChildren(Widget& container, const std::vector<Element>& children) {
    auto& existingChildren = container.GetChildren();

    std::unordered_map<std::string, std::size_t> idToIndex;
    for (std::size_t i = 0; i < existingChildren.size(); ++i) {
        if (existingChildren[i] && !existingChildren[i]->GetId().empty()) {
            idToIndex[existingChildren[i]->GetId()] = i;
        }
    }

    std::vector<std::shared_ptr<Widget>> newChildren;
    newChildren.reserve(children.size());
    std::vector<bool> reused(existingChildren.size(), false);

    for (const Element& childElement : children) {
        const Element expanded = Expand(childElement);
        if (!expanded.visible && expanded.when) {
            continue;
        }

        std::shared_ptr<Widget> childWidget;
        if (!expanded.id.empty()) {
            const auto it = idToIndex.find(expanded.id);
            if (it != idToIndex.end() && !reused[it->second]) {
                childWidget = existingChildren[it->second];
                reused[it->second] = true;
                Reconcile(*childWidget, expanded);
            }
        }

        if (!childWidget) {
            childWidget = BuildElement(expanded);
        }

        newChildren.push_back(std::move(childWidget));
    }

    container.ClearChildren();
    for (auto& child : newChildren) {
        container.AddChild(child);
    }
}

void ViewBuilder::Reconcile(Widget& existing, const Element& updated) {
    ReconcileElement(existing, updated);
    const Element expanded = Expand(updated);
    ReconcileChildren(existing, expanded.children);
}

std::shared_ptr<Widget> ViewBuilder::BuildElement(const Element& element) {
    const Element expanded = Expand(element);
    if (!expanded.visible && expanded.when) {
        auto placeholder = std::make_shared<Spacer>();
        placeholder->SetVisible(false);
        return placeholder;
    }

    std::shared_ptr<Widget> widget;

    switch (expanded.type) {
    case ElementType::Host:
        if (expanded.host) {
            widget = expanded.host();
        }
        break;
    case ElementType::Row:
    case ElementType::Toolbar: {
        auto row = MakeContainer<Row>(expanded);
        if (expanded.type == ElementType::Toolbar) {
            row->SetStyleClass("ToolbarButton");
            row->Align(AlignItems::Center);
            row->SetMinSize({ 0.0f, 36.0f });
        }
        for (const Element& child : expanded.children) {
            if (auto built = BuildElement(child)) {
                row->AddChild(built);
            }
        }
        widget = std::move(row);
        break;
    }
    case ElementType::Column: {
        auto col = MakeContainer<Column>(expanded);
        for (const Element& child : expanded.children) {
            if (auto built = BuildElement(child)) {
                col->AddChild(built);
            }
        }
        widget = std::move(col);
        break;
    }
    case ElementType::Flex: {
        auto flex = std::make_shared<Flex>(FlexDirection::Row);
        if (expanded.layout.gap >= 0.0f) {
            flex->Gap(expanded.layout.gap);
        }
        for (const Element& child : expanded.children) {
            if (auto built = BuildElement(child)) {
                flex->AddChild(built);
            }
        }
        widget = std::move(flex);
        break;
    }
    case ElementType::ScrollView: {
        auto scroll = std::make_shared<ScrollLayout>();
        auto content = MakeContainer<Column>(expanded);
        for (const Element& child : expanded.children) {
            if (auto built = BuildElement(child)) {
                content->AddChild(built);
            }
        }
        scroll->SetContent(content);
        widget = std::move(scroll);
        break;
    }
    case ElementType::Card: {
        auto card = std::make_shared<Card>();
        for (const Element& child : expanded.children) {
            if (auto built = BuildElement(child)) {
                card->AddChild(built);
            }
        }
        widget = std::move(card);
        break;
    }
    case ElementType::Button: {
        if (!expanded.icon.empty() && expanded.text.empty()) {
            auto iconBtn = std::make_shared<IconButton>(expanded.icon.c_str());
            if (expanded.events.onClicked) {
                iconBtn->SetOnClicked(expanded.events.onClicked);
            }
            widget = std::move(iconBtn);
        } else {
            auto btn = MakeDesignButton(expanded);
            WireButtonEvents(*btn, expanded);
            widget = std::move(btn);
        }
        break;
    }
    case ElementType::SectionHeader:
        widget = std::make_shared<SectionHeader>(expanded.text, expanded.subtitle);
        break;
    case ElementType::Label:
        widget = std::make_shared<Label>(expanded.text);
        break;
    case ElementType::TextBox:
        widget = std::make_shared<TextBox>(expanded.placeholder.value_or(""));
        break;
    case ElementType::SearchBox: {
        auto search = std::make_shared<SearchBoxControl>(expanded.placeholder.value_or("Search..."));
        if (expanded.events.onValueChanged) {
            search->SetOnChanged([cb = expanded.events.onValueChanged](const std::string& value) {
                cb(value);
            });
        }
        widget = std::move(search);
        break;
    }
    case ElementType::CheckBox: {
        auto box = std::make_shared<CheckBox>(expanded.text, expanded.checked.value_or(false));
        if (expanded.events.onSelectionChanged) {
            box->SetOnChanged(expanded.events.onSelectionChanged);
        }
        widget = std::move(box);
        break;
    }
    case ElementType::EmptyState:
        widget = std::make_shared<EmptyState>(expanded.text, expanded.subtitle);
        break;
    case ElementType::Skeleton:
        widget = MakeSkeleton();
        break;
    case ElementType::Spacer:
        widget = std::make_shared<Spacer>();
        break;
    case ElementType::Dialog: {
        std::shared_ptr<Widget> body;
        if (!expanded.children.empty()) {
            auto bodyCol = MakeContainer<Column>(expanded);
            for (const Element& child : expanded.children) {
                if (auto built = BuildElement(child)) {
                    bodyCol->AddChild(built);
                }
            }
            body = std::move(bodyCol);
        }
        widget = std::make_shared<DialogChrome>(expanded.text, body);
        break;
    }
    case ElementType::Menu: {
        auto menu = std::make_shared<Card>();
        menu->SetStyleClass(expanded.styleClass.empty() ? "Menu" : expanded.styleClass);
        auto col = std::make_shared<Column>();
        col->Gap(2.0f);
        for (const Element& child : expanded.children) {
            if (auto built = BuildElement(child)) {
                col->AddChild(built);
            }
        }
        menu->AddChild(col);
        widget = std::move(menu);
        break;
    }
    case ElementType::MenuSeparator: {
        auto sep = std::make_shared<Spacer>();
        sep->SetMinSize({ 0.0f, 1.0f });
        widget = std::move(sep);
        break;
    }
    case ElementType::MenuItem: {
        auto btn = MakeDesignButton(expanded);
        if (expanded.enabled == false) {
            btn->SetEnabled(false);
        }
        widget = std::move(btn);
        break;
    }
    default: {
        auto fallback = MakeContainer<Column>(expanded);
        for (const Element& child : expanded.children) {
            if (auto built = BuildElement(child)) {
                fallback->AddChild(built);
            }
        }
        widget = std::move(fallback);
        break;
    }
    }

    if (widget) {
        ApplyElementProperties(*widget, expanded);
        if (!expanded.id.empty()) {
            widget->SetId(expanded.id);
        }
        WireButtonEvents(*widget, expanded);
    }

    return widget;
}

} // namespace we::runtime::kindui
