#pragma once

#include "KindUI/Export.h"
#include "KindUI/Core/Types.h"
#include "KindUI/Core/WidgetVariant.h"

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace we::runtime::kindui {

class Widget;
class ICommand;

// Element types for declarative UI composition.
enum class ElementType {
    Row,
    Column,
    Flex,
    Grid,
    Stack,
    Overlay,
    Wrap,
    ScrollView,
    SplitView,
    SizeBox,
    AspectRatioBox,
    Button,
    Label,
    SectionHeader,
    TextBox,
    CheckBox,
    RadioButton,
    ComboBox,
    SearchBox,
    Image,
    Spacer,
    Card,
    Dialog,
    Menu,
    MenuItem,
    MenuSeparator,
    Toolbar,
    TreeView,
    TableView,
    PropertyGrid,
    VirtualList,
    EmptyState,
    Skeleton,
    Host, // Application-provided widget factory (custom components)
};

struct ElementEvents {
    std::function<void()> onClicked;
    std::function<void()> onDoubleClicked;
    std::function<void(bool)> onHovered;
    std::function<void(std::string_view)> onValueChanged;
    std::function<void()> onActivated;
    std::function<void(float, float)> onContextRequested;
    std::function<void(bool)> onFocusChanged;
    std::function<void(bool)> onSelectionChanged;
    std::shared_ptr<ICommand> command;
};

struct LayoutIntent {
    float flexGrow = 0.0f;
    float flexShrink = 1.0f;
    float flexBasis = -1.0f;
    float minWidth = 0.0f;
    float minHeight = 0.0f;
    float maxWidth = 1.0e9f;
    float maxHeight = 1.0e9f;
    float gap = -1.0f;
    float marginLeft = 0.0f;
    float marginTop = 0.0f;
    float marginRight = 0.0f;
    float marginBottom = 0.0f;
    float paddingLeft = -1.0f;
    float paddingTop = -1.0f;
    float paddingRight = -1.0f;
    float paddingBottom = -1.0f;
    bool fill = false;
    std::optional<HorizontalAlignment> hAlign;
    std::optional<VerticalAlignment> vAlign;
};

using WidgetFactory = std::function<std::shared_ptr<Widget>()>;
using WidgetConfigurator = std::function<void(Widget&)>;

struct Element {
    ElementType type = ElementType::Row;
    std::string id{};
    std::string text{};
    std::string subtitle{};
    std::string icon{};
    WidgetVariant variant = WidgetVariant::Default;
    std::string styleClass{};
    LayoutIntent layout{};
    ElementEvents events{};
    std::vector<Element> children{};

    bool enabled = true;
    bool visible = true;
    std::optional<bool> checked{};
    std::optional<std::string> placeholder{};
    std::optional<float> width{};
    std::optional<float> height{};

    // Conditional: include this subtree only when predicate is true.
    std::function<bool()> when{};

    // Collection expansion: produces child elements at build time.
    std::function<std::vector<Element>()> forEach{};

    // Custom widget host for application-specific components.
    WidgetFactory host{};
    WidgetConfigurator configure{};
};

} // namespace we::runtime::kindui
