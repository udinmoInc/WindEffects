// ==============================================================================
// WindEffects — KindUI — EditorDSL
// Public API surface for the Engine-wide Declarative Editor DSL.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "KindUI/Export.h"
#include "KindUI/Panel/PanelBuilder.h"
#include "KindUI/Panel/Panel.h"
#include "KindUI/Core/Widget.h"
#include "KindUI/Core/WindIcon.h"
#include "KindUI/Layout/PropertyRowLayout.h"
#include "KindUI/Layout/CollapsibleGroup.h"
#include "KindUI/Widgets/ObjectTitleBar.h"
#include "KindUI/Widgets/FilterTabStrip.h"
#include "KindUI/Widgets/CompactTreeWidget.h"
#include "KindUI/Widgets/ScrollContainer.h"
#include "KindUI/Widgets/TextBox.h"
#include "KindUI/Core/Widgets/DesignSystemControls.h"

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include <utility>

namespace we::editor::dsl {

using namespace ::we::runtime::kindui;
using ::we::runtime::kindui::panels::PanelBuilder;

using EditorPanelPtr = std::shared_ptr<::we::runtime::kindui::panels::Panel>;
using KindUIWidgetPtr = std::shared_ptr<::we::runtime::kindui::Widget>;

/// Field configuration helper passed to field builder lambdas.
class KINDUI_API FieldConfig {
public:
    FieldConfig() = default;

    FieldConfig& ToolTip(std::string tooltip) { m_ToolTip = std::move(tooltip); return *this; }
    FieldConfig& ReadOnly(bool readOnly = true) { m_ReadOnly = readOnly; return *this; }
    FieldConfig& Disabled(bool disabled = true) { m_Disabled = disabled; return *this; }
    FieldConfig& Modified(bool modified = true) { m_Modified = modified; return *this; }
    FieldConfig& OnChanged(std::function<void()> cb) { m_OnChanged = std::move(cb); return *this; }
    FieldConfig& OnReset(std::function<void()> cb) { m_OnReset = std::move(cb); return *this; }

    [[nodiscard]] const std::string& GetToolTip() const { return m_ToolTip; }
    [[nodiscard]] bool IsReadOnly() const { return m_ReadOnly; }
    [[nodiscard]] bool IsDisabled() const { return m_Disabled; }
    [[nodiscard]] bool IsModified() const { return m_Modified; }
    [[nodiscard]] const std::function<void()>& GetOnChanged() const { return m_OnChanged; }
    [[nodiscard]] const std::function<void()>& GetOnReset() const { return m_OnReset; }

private:
    std::string m_ToolTip;
    bool m_ReadOnly = false;
    bool m_Disabled = false;
    bool m_Modified = false;
    std::function<void()> m_OnChanged;
    std::function<void()> m_OnReset;
};

/// Declarative builder context for section / group content.
class KINDUI_API SectionContext {
public:
    explicit SectionContext(std::string title = "");
    ~SectionContext();

    void Field(std::string label, KindUIWidgetPtr valueWidget, std::function<void(FieldConfig&)> config = nullptr);
    
    void Bool(std::string label, bool& value, std::function<void(FieldConfig&)> config = nullptr);
    void Bool(std::string label, std::function<bool()> getter, std::function<void(bool)> setter,
        std::function<void(FieldConfig&)> config = nullptr);

    void Int(std::string label, int& value, std::function<void(FieldConfig&)> config = nullptr);
    void Float(std::string label, float& value, std::function<void(FieldConfig&)> config = nullptr);
    void Double(std::string label, double& value, std::function<void(FieldConfig&)> config = nullptr);
    
    void Slider(std::string label, double& value, double minVal, double maxVal, double step = 0.1,
        std::function<void(FieldConfig&)> config = nullptr);
    void String(std::string label, std::string& value, bool multiline = false, std::function<void(FieldConfig&)>
        config = nullptr);
    
    void Enum(std::string label, int& value, std::vector<std::string> options, std::function<void(FieldConfig&)>
        config = nullptr);
    void Flags(std::string label, uint32_t& value, std::vector<std::pair<uint32_t, std::string>> flagBits,
        std::function<void(FieldConfig&)> config = nullptr);
    
    void Vector2(std::string label, float* vec2Values, std::function<void(FieldConfig&)> config = nullptr);
    void Vector3(std::string label, float* vec3Values, std::function<void(FieldConfig&)> config = nullptr);
    void Vector4(std::string label, float* vec4Values, std::function<void(FieldConfig&)> config = nullptr);
    void Rotation(std::string label, float* rotEulerValues, std::function<void(FieldConfig&)> config = nullptr);
    void Transform(std::string label, float* transformValues, std::function<void(FieldConfig&)> config = nullptr);
    
    void Color(std::string label, float* rgbaValues, bool includeAlpha = true, std::function<void(FieldConfig&)>
        config = nullptr);
    void Gradient(std::string label, std::function<void(FieldConfig&)> config = nullptr);
    
    void Asset(std::string label, std::string& assetPath, std::string_view extension = "",
        std::function<void(FieldConfig&)> config = nullptr);
    void ObjectRef(std::string label, std::string& objectName, std::function<void(FieldConfig&)> config = nullptr);
    void ClassRef(std::string label, std::string& className, std::function<void(FieldConfig&)> config = nullptr);
    
    void Container(std::string label, std::vector<std::string>& elements, std::function<void(FieldConfig&)> config =
        nullptr);
    void Curve(std::string label, std::function<void(FieldConfig&)> config = nullptr);
    void TagLayer(std::string label, std::string& tagOrLayer, bool isLayerMode = false,
        std::function<void(FieldConfig&)> config = nullptr);

    void Widget(KindUIWidgetPtr widget);
    void Button(std::string label, std::function<void()> onClicked);
    void Button(std::string label, WindIconRef icon, std::function<void()> onClicked);

    [[nodiscard]] KindUIWidgetPtr BuildWidget() const;

private:
    std::string m_Title;
    std::vector<KindUIWidgetPtr> m_Rows;
};

/// Declarative builder context for toolbars.
class KINDUI_API ToolbarContext {
public:
    ToolbarContext() = default;

    void Button(std::string label, std::function<void()> onClicked);
    void Button(std::string label, WindIconRef icon, std::function<void()> onClicked);
    void IconButton(WindIconRef icon, std::function<void()> onClicked);
    void Search(std::string placeholder = "Search...", std::function<void(const std::string&)> onQueryChanged =
        nullptr);
    void Separator();
    void Custom(KindUIWidgetPtr widget);

    [[nodiscard]] KindUIWidgetPtr BuildWidget() const;

private:
    std::vector<KindUIWidgetPtr> m_Items;
};

class PanelContext;

/// Declarative builder context for tab views.
class KINDUI_API TabContext {
public:
    TabContext() = default;

    void Tab(std::string name, std::function<void(PanelContext&)> buildContent);
    void Tab(std::string name, WindIconRef icon, std::function<void(PanelContext&)> buildContent);

    [[nodiscard]] std::shared_ptr<FilterTabStrip> BuildTabStrip() const;
    [[nodiscard]] const std::vector<std::pair<std::string, KindUIWidgetPtr>>& GetTabs() const { return m_Tabs; }

private:
    std::vector<std::pair<std::string, KindUIWidgetPtr>> m_Tabs;
};

/// Declarative builder context for editor panels and windows.
class KINDUI_API PanelContext {
public:
    explicit PanelContext(std::string title);
    ~PanelContext();

    PanelContext& Header(std::string title, WindIconRef icon = kWindIconNone);
    PanelContext& Section(std::string title, std::function<void(SectionContext&)> buildSection);
    PanelContext& Section(std::string title, bool expanded, std::function<void(SectionContext&)> buildSection);
    
    PanelContext& Toolbar(std::function<void(ToolbarContext&)> buildToolbar);
    PanelContext& Search(std::string placeholder, std::function<void(const std::string&)> onQueryChanged);
    PanelContext& Search(KindUIWidgetPtr searchWidget);
    
    PanelContext& FilterStrip(std::vector<std::string> tabs, std::function<void(const std::string&)> onTabSelected);
    PanelContext& Tree(std::string id, std::vector<CompactTreeNode> items, std::function<void(const CompactTreeNode&)>
        onItemClicked);
    PanelContext& Tree(std::shared_ptr<CompactTreeWidget> treeWidget);
    
    PanelContext& AssetGrid(KindUIWidgetPtr gridWidget);
    PanelContext& Viewport(KindUIWidgetPtr viewportWidget = nullptr);
    
    PanelContext& Tabs(std::function<void(TabContext&)> buildTabs);
    PanelContext& Content(KindUIWidgetPtr contentWidget);
    PanelContext& ColumnHeader(KindUIWidgetPtr columnHeaderWidget);
    PanelContext& Footer(KindUIWidgetPtr footerWidget);

    PanelContext& WithCloseButton(std::function<void()> onClose = nullptr);
    PanelContext& TabIcon(WindIconRef icon);
    PanelContext& Transparent(bool transparent = true);
    PanelContext& HeaderHeight(float height);

    [[nodiscard]] EditorPanelPtr BuildPanel();

private:
    std::string m_Title;
    PanelBuilder m_Builder;
    std::vector<KindUIWidgetPtr> m_BodyWidgets;
    std::shared_ptr<ObjectTitleBar> m_TitleBar;
    std::shared_ptr<FilterTabStrip> m_FilterStrip;
    std::shared_ptr<CompactTreeWidget> m_TreeWidget;
    KindUIWidgetPtr m_CustomContent;
    KindUIWidgetPtr m_ToolbarWidget;
};

using WindowContext = PanelContext;

/// Entry point function for creating a Panel using the declarative DSL.
KINDUI_API EditorPanelPtr Panel(std::string title, std::function<void(PanelContext&)> build);

/// Entry point function for creating a Window using the declarative DSL.
KINDUI_API EditorPanelPtr Window(std::string title, std::function<void(WindowContext&)> build);

} // namespace we::editor::dsl
