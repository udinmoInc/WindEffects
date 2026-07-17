#pragma once

#include "KindUI/Export.h"
#include "KindUI/Core/WidgetVariant.h"

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace we::runtime::kindui {

class Widget;

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
    TextBox,
    CheckBox,
    RadioButton,
    ComboBox,
    SearchBox,
    Image,
    Spacer,
    Card,
    Dialog,
    Toolbar,
    TreeView,
    TableView,
    PropertyGrid,
    VirtualList,
    EmptyState,
    Skeleton,
};

// Semantic events exposed by widgets. KindUI handles routing.
struct ElementEvents {
    std::function<void()> onClicked;
    std::function<void()> onDoubleClicked;
    std::function<void(bool)> onHovered;
    std::function<void(std::string_view)> onValueChanged;
    std::function<void()> onActivated;
    std::function<void(float, float)> onContextRequested;
    std::function<void(bool)> onFocusChanged;
    std::function<void(bool)> onSelectionChanged;
};

// Layout intent for declarative containers.
struct LayoutIntent {
    float flexGrow = 0.0f;
    float flexShrink = 1.0f;
    float flexBasis = -1.0f;
    float minWidth = 0.0f;
    float minHeight = 0.0f;
    float maxWidth = 1.0e9f;
    float maxHeight = 1.0e9f;
    float gap = -1.0f; // -1 = use theme default
    bool fill = false;
};

// Declarative element description. Stateless; reconciled into widget tree by ViewBuilder.
struct Element {
    ElementType type = ElementType::Row;
    std::string id{};
    std::string text{};
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
};

} // namespace we::runtime::kindui
