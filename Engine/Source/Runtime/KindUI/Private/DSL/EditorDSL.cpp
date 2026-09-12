// ==============================================================================
// WindEffects — KindUI — EditorDSL
// Implementation for the Engine-wide Declarative Editor DSL.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "KindUI/DSL/EditorDSL.h"
#include "KindUI/Layout/Flex.h"
#include "KindUI/Widgets/Label.h"
#include "KindUI/Widgets/TextBox.h"
#include "KindUI/Core/LayoutMetrics.h"

namespace we::editor::dsl {

using namespace ::we::runtime::kindui;

// ------------------------------------------------------------------------------
// SectionContext Implementation
// ------------------------------------------------------------------------------

SectionContext::SectionContext(std::string title)
    : m_Title(std::move(title)) {}

SectionContext::~SectionContext() = default;

void SectionContext::Field(std::string label, KindUIWidgetPtr valueWidget, std::function<void(FieldConfig&)> config) {
    FieldConfig cfg;
    if (config) {
        config(cfg);
    }

    if (!valueWidget) {
        valueWidget = std::make_shared<Label>(label);
    }

    auto row = std::make_shared<PropertyRowLayout>(std::move(label), valueWidget);
    row->SetModified(cfg.IsModified());
    row->SetReadOnly(cfg.IsReadOnly());
    if (cfg.GetOnReset()) {
        row->SetOnResetClicked(cfg.GetOnReset());
    }

    m_Rows.push_back(std::move(row));
}

void SectionContext::Bool(std::string label, bool& value, std::function<void(FieldConfig&)> config) {
    (void)value;
    Field(std::move(label), std::make_shared<IconButton>(WindIcons::Check16), std::move(config));
}

void SectionContext::Bool(std::string label, std::function<bool()> getter, std::function<void(bool)> setter,
    std::function<void(FieldConfig&)> config) {
    (void)getter;
    (void)setter;
    Field(std::move(label), std::make_shared<IconButton>(WindIcons::Check16), std::move(config));
}

void SectionContext::Int(std::string label, int& value, std::function<void(FieldConfig&)> config) {
    (void)value;
    Field(std::move(label), std::make_shared<TextBox>(label), std::move(config));
}

void SectionContext::Float(std::string label, float& value, std::function<void(FieldConfig&)> config) {
    (void)value;
    Field(std::move(label), std::make_shared<TextBox>(label), std::move(config));
}

void SectionContext::Double(std::string label, double& value, std::function<void(FieldConfig&)> config) {
    (void)value;
    Field(std::move(label), std::make_shared<TextBox>(label), std::move(config));
}

void SectionContext::Slider(std::string label, double& value, double minVal, double maxVal, double step,
    std::function<void(FieldConfig&)> config) {
    (void)value; (void)minVal; (void)maxVal; (void)step;
    Field(std::move(label), std::make_shared<TextBox>(label), std::move(config));
}

void SectionContext::String(std::string label, std::string& value, bool multiline, std::function<void(FieldConfig&)>
    config) {
    (void)value; (void)multiline;
    Field(std::move(label), std::make_shared<TextBox>(label), std::move(config));
}

void SectionContext::Enum(std::string label, int& value, std::vector<std::string> options,
    std::function<void(FieldConfig&)> config) {
    (void)value; (void)options;
    Field(std::move(label), std::make_shared<TextBox>(label), std::move(config));
}

void SectionContext::Flags(std::string label, uint32_t& value, std::vector<std::pair<uint32_t, std::string>> flagBits,
    std::function<void(FieldConfig&)> config) {
    (void)value; (void)flagBits;
    Field(std::move(label), std::make_shared<TextBox>(label), std::move(config));
}

void SectionContext::Vector2(std::string label, float* vec2Values, std::function<void(FieldConfig&)> config) {
    (void)vec2Values;
    Field(std::move(label), std::make_shared<TextBox>(label), std::move(config));
}

void SectionContext::Vector3(std::string label, float* vec3Values, std::function<void(FieldConfig&)> config) {
    (void)vec3Values;
    Field(std::move(label), std::make_shared<TextBox>(label), std::move(config));
}

void SectionContext::Vector4(std::string label, float* vec4Values, std::function<void(FieldConfig&)> config) {
    (void)vec4Values;
    Field(std::move(label), std::make_shared<TextBox>(label), std::move(config));
}

void SectionContext::Rotation(std::string label, float* rotEulerValues, std::function<void(FieldConfig&)> config) {
    (void)rotEulerValues;
    Field(std::move(label), std::make_shared<TextBox>(label), std::move(config));
}

void SectionContext::Transform(std::string label, float* transformValues, std::function<void(FieldConfig&)> config) {
    (void)transformValues;
    Field(std::move(label), std::make_shared<TextBox>(label), std::move(config));
}

void SectionContext::Color(std::string label, float* rgbaValues, bool includeAlpha, std::function<void(FieldConfig&)>
    config) {
    (void)rgbaValues; (void)includeAlpha;
    Field(std::move(label), std::make_shared<TextBox>(label), std::move(config));
}

void SectionContext::Gradient(std::string label, std::function<void(FieldConfig&)> config) {
    Field(std::move(label), std::make_shared<TextBox>(label), std::move(config));
}

void SectionContext::Asset(std::string label, std::string& assetPath, std::string_view extension,
    std::function<void(FieldConfig&)> config) {
    (void)assetPath; (void)extension;
    Field(std::move(label), std::make_shared<TextBox>(label), std::move(config));
}

void SectionContext::ObjectRef(std::string label, std::string& objectName, std::function<void(FieldConfig&)> config) {
    (void)objectName;
    Field(std::move(label), std::make_shared<TextBox>(label), std::move(config));
}

void SectionContext::ClassRef(std::string label, std::string& className, std::function<void(FieldConfig&)> config) {
    (void)className;
    Field(std::move(label), std::make_shared<TextBox>(label), std::move(config));
}

void SectionContext::Container(std::string label, std::vector<std::string>& elements, std::function<void(FieldConfig&)>
    config) {
    (void)elements;
    Field(std::move(label), std::make_shared<TextBox>(label), std::move(config));
}

void SectionContext::Curve(std::string label, std::function<void(FieldConfig&)> config) {
    Field(std::move(label), std::make_shared<TextBox>(label), std::move(config));
}

void SectionContext::TagLayer(std::string label, std::string& tagOrLayer, bool isLayerMode,
    std::function<void(FieldConfig&)> config) {
    (void)tagOrLayer; (void)isLayerMode;
    Field(std::move(label), std::make_shared<TextBox>(label), std::move(config));
}

void SectionContext::Widget(KindUIWidgetPtr widget) {
    if (widget) {
        m_Rows.push_back(std::move(widget));
    }
}

void SectionContext::Button(std::string label, std::function<void()> onClicked) {
    auto btn = std::make_shared<ToolbarButton>(label);
    if (onClicked) {
        btn->SetOnClicked(std::move(onClicked));
    }
    m_Rows.push_back(std::move(btn));
}

void SectionContext::Button(std::string label, WindIconRef icon, std::function<void()> onClicked) {
    auto btn = std::make_shared<ToolbarButton>(label, icon);
    if (onClicked) {
        btn->SetOnClicked(std::move(onClicked));
    }
    m_Rows.push_back(std::move(btn));
}

KindUIWidgetPtr SectionContext::BuildWidget() const {
    if (!m_Title.empty()) {
        auto group = std::make_shared<CollapsibleGroup>(m_Title, true);
        for (const auto& row : m_Rows) {
            group->AddContentChild(row);
        }
        return group;
    }
    auto col = std::make_shared<Column>();
    col->Gap(4.0f);
    for (const auto& row : m_Rows) {
        col->AddChild(row);
    }
    return col;
}

// ------------------------------------------------------------------------------
// ToolbarContext Implementation
// ------------------------------------------------------------------------------

void ToolbarContext::Button(std::string label, std::function<void()> onClicked) {
    auto btn = std::make_shared<ToolbarButton>(label);
    if (onClicked) btn->SetOnClicked(std::move(onClicked));
    m_Items.push_back(std::move(btn));
}

void ToolbarContext::Button(std::string label, WindIconRef icon, std::function<void()> onClicked) {
    auto btn = std::make_shared<ToolbarButton>(label, icon);
    if (onClicked) btn->SetOnClicked(std::move(onClicked));
    m_Items.push_back(std::move(btn));
}

void ToolbarContext::IconButton(WindIconRef icon, std::function<void()> onClicked) {
    auto btn = std::make_shared<::we::runtime::kindui::IconButton>(icon);
    if (onClicked) btn->SetOnClicked(std::move(onClicked));
    m_Items.push_back(std::move(btn));
}

void ToolbarContext::Search(std::string placeholder, std::function<void(const std::string&)> onQueryChanged) {
    auto search = std::make_shared<SearchBoxControl>();
    search->SetPlaceholder(placeholder);
    if (onQueryChanged) search->SetOnTextChanged(std::move(onQueryChanged));
    m_Items.push_back(std::move(search));
}

void ToolbarContext::Separator() {
    auto sep = std::make_shared<Row>();
    m_Items.push_back(std::move(sep));
}

void ToolbarContext::Custom(KindUIWidgetPtr widget) {
    if (widget) m_Items.push_back(std::move(widget));
}

KindUIWidgetPtr ToolbarContext::BuildWidget() const {
    auto row = std::make_shared<Row>();
    row->Padding(Margin{ 4.0f, 2.0f, 4.0f, 2.0f });
    row->Gap(6.0f);
    for (const auto& item : m_Items) {
        row->AddChild(item);
    }
    return row;
}

// ------------------------------------------------------------------------------
// TabContext Implementation
// ------------------------------------------------------------------------------

void TabContext::Tab(std::string name, std::function<void(PanelContext&)> buildContent) {
    PanelContext pc(name);
    if (buildContent) {
        buildContent(pc);
    }
    m_Tabs.emplace_back(std::move(name), pc.BuildPanel());
}

void TabContext::Tab(std::string name, WindIconRef icon, std::function<void(PanelContext&)> buildContent) {
    (void)icon;
    Tab(std::move(name), std::move(buildContent));
}

std::shared_ptr<FilterTabStrip> TabContext::BuildTabStrip() const {
    auto strip = std::make_shared<FilterTabStrip>();
    std::vector<std::string> labels;
    labels.reserve(m_Tabs.size());
    for (const auto& pair : m_Tabs) {
        labels.push_back(pair.first);
    }
    strip->SetTabs(std::move(labels));
    return strip;
}

// ------------------------------------------------------------------------------
// PanelContext Implementation
// ------------------------------------------------------------------------------

PanelContext::PanelContext(std::string title)
    : m_Title(std::move(title))
    , m_Builder(m_Title) {}

PanelContext::~PanelContext() = default;

PanelContext& PanelContext::Header(std::string title, WindIconRef icon) {
    m_TitleBar = std::make_shared<ObjectTitleBar>(std::move(title), icon);
    return *this;
}

PanelContext& PanelContext::Section(std::string title, std::function<void(SectionContext&)> buildSection) {
    SectionContext sc(std::move(title));
    if (buildSection) {
        buildSection(sc);
    }
    m_BodyWidgets.push_back(sc.BuildWidget());
    return *this;
}

PanelContext& PanelContext::Section(std::string title, bool expanded, std::function<void(SectionContext&)>
    buildSection) {
    (void)expanded;
    return Section(std::move(title), std::move(buildSection));
}

PanelContext& PanelContext::Toolbar(std::function<void(ToolbarContext&)> buildToolbar) {
    ToolbarContext tc;
    if (buildToolbar) {
        buildToolbar(tc);
    }
    m_ToolbarWidget = tc.BuildWidget();
    m_Builder.Toolbar(m_ToolbarWidget);
    return *this;
}

PanelContext& PanelContext::Search(std::string placeholder, std::function<void(const std::string&)> onQueryChanged) {
    auto search = std::make_shared<SearchBoxControl>();
    search->SetPlaceholder(placeholder);
    if (onQueryChanged) {
        search->SetOnTextChanged(std::move(onQueryChanged));
    }
    m_Builder.Search(search);
    return *this;
}

PanelContext& PanelContext::Search(KindUIWidgetPtr searchWidget) {
    m_Builder.Search(searchWidget);
    return *this;
}

PanelContext& PanelContext::FilterStrip(std::vector<std::string> tabs, std::function<void(const std::string&)>
    onTabSelected) {
    m_FilterStrip = std::make_shared<FilterTabStrip>();
    m_FilterStrip->SetTabs(std::move(tabs));
    if (onTabSelected) {
        m_FilterStrip->SetOnTabSelected(std::move(onTabSelected));
    }
    m_Builder.ModeTabs(m_FilterStrip);
    return *this;
}

PanelContext& PanelContext::Tree(std::string id, std::vector<CompactTreeNode> items,
    std::function<void(const CompactTreeNode&)> onItemClicked) {
    (void)id;
    m_TreeWidget = std::make_shared<CompactTreeWidget>();
    m_TreeWidget->SetItems(std::move(items));
    if (onItemClicked) {
        m_TreeWidget->SetOnItemClicked(std::move(onItemClicked));
    }
    m_BodyWidgets.push_back(m_TreeWidget);
    return *this;
}

PanelContext& PanelContext::Tree(std::shared_ptr<CompactTreeWidget> treeWidget) {
    m_TreeWidget = std::move(treeWidget);
    m_BodyWidgets.push_back(m_TreeWidget);
    return *this;
}

PanelContext& PanelContext::AssetGrid(KindUIWidgetPtr gridWidget) {
    if (gridWidget) {
        m_BodyWidgets.push_back(gridWidget);
    }
    return *this;
}

PanelContext& PanelContext::Viewport(KindUIWidgetPtr viewportWidget) {
    if (viewportWidget) {
        m_CustomContent = viewportWidget;
    } else {
        m_CustomContent = std::make_shared<Label>("Viewport");
    }
    return *this;
}

PanelContext& PanelContext::Tabs(std::function<void(TabContext&)> buildTabs) {
    TabContext tc;
    if (buildTabs) {
        buildTabs(tc);
    }
    m_FilterStrip = tc.BuildTabStrip();
    m_Builder.ModeTabs(m_FilterStrip);
    return *this;
}

PanelContext& PanelContext::Content(KindUIWidgetPtr contentWidget) {
    m_CustomContent = std::move(contentWidget);
    return *this;
}

PanelContext& PanelContext::ColumnHeader(KindUIWidgetPtr columnHeaderWidget) {
    m_Builder.ColumnHeader(std::move(columnHeaderWidget));
    return *this;
}

PanelContext& PanelContext::Footer(KindUIWidgetPtr footerWidget) {
    m_Builder.Footer(std::move(footerWidget));
    return *this;
}

PanelContext& PanelContext::WithCloseButton(std::function<void()> onClose) {
    m_Builder.WithCloseButton(std::move(onClose));
    return *this;
}

PanelContext& PanelContext::TabIcon(WindIconRef icon) {
    m_Builder.TabIcon(icon);
    return *this;
}

PanelContext& PanelContext::Transparent(bool transparent) {
    if (transparent) m_Builder.Transparent();
    return *this;
}

PanelContext& PanelContext::HeaderHeight(float height) {
    m_Builder.HeaderHeight(height);
    return *this;
}

EditorPanelPtr PanelContext::BuildPanel() {
    if (m_CustomContent) {
        m_Builder.Content(m_CustomContent);
    } else {
        auto bodyColumn = std::make_shared<Column>();
        bodyColumn->Gap(6.0f);
        if (m_TitleBar) {
            bodyColumn->AddChild(m_TitleBar);
        }
        for (const auto& w : m_BodyWidgets) {
            bodyColumn->AddChild(w);
        }
        auto scrollContainer = std::make_shared<ScrollContainer>(bodyColumn);
        m_Builder.Content(scrollContainer);
    }
    return m_Builder.Build();
}

// ------------------------------------------------------------------------------
// Entry Functions
// ------------------------------------------------------------------------------

EditorPanelPtr Panel(std::string title, std::function<void(PanelContext&)> build) {
    PanelContext ctx(std::move(title));
    if (build) {
        build(ctx);
    }
    return ctx.BuildPanel();
}

EditorPanelPtr Window(std::string title, std::function<void(WindowContext&)> build) {
    return Panel(std::move(title), std::move(build));
}

} // namespace we::editor::dsl
