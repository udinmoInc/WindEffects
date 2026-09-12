// ==============================================================================
// WindEffects — PropertyEditor — DetailsView
// UI widget used by the PropertyEditor module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "PropertyEditorInternal.h"
#include "PropertyEditor/IDetailsView.h"
#include "PropertyEditor/PropertyChangeEvent.h"
#include "Core/Logger.h"
#include "Core/DiagnosticMacros.h"

#include "KindUI/Panel/PanelChrome.h"
#include "KindUI/Core/LayoutMetrics.h"
#include "KindUI/Core/PropertyPanelChrome.h"
#include "KindUI/Core/PropertyColumnSplitter.h"
#include "KindUI/Core/Style.h"
#include "KindUI/Core/WindIcon.h"

#include <cmath>
#include <cstdint>
#include <vector>

namespace we::editor::property {
namespace detail {

namespace {
float DetailsRowHeightPx() {
    // One shared row height keeps labels, numeric inputs, vectors, and
    // checkboxes locked to the same compact vertical rhythm.
    return we::runtime::kindui::PropertyPanelChrome::RowHeight();
}
float DetailsSectionHeightPx() {
    return we::runtime::kindui::PropertyPanelChrome::SectionHeight();
}
float DetailsSectionGapPx() {
    // One shared inset for the top of the list, between root sections
    // (Actor / Transform / Light), and the bottom of the list.
    return we::runtime::kindui::PropertyPanelChrome::FormStackGap();
}
float DetailsWheelStepPx() {
    return we::runtime::kindui::ResolveMetric(we::runtime::kindui::MetricToken::ControlHeightLarge);
}
}

using we::runtime::kindui::MetricToken;
using we::runtime::kindui::MouseButton;
using we::runtime::kindui::MouseEvent;
using we::runtime::kindui::MouseEventType;
using we::runtime::kindui::PaintContext;
using we::runtime::kindui::Point;
using we::runtime::kindui::Rect;
using we::runtime::kindui::ScrollViewport;
using we::runtime::kindui::ScrollViewportMetrics;
using we::runtime::kindui::Size;
using we::runtime::kindui::Widget;
using we::runtime::kindui::WidgetStyle;
using we::runtime::kindui::DPIContext;
using we::runtime::kindui::PropertyColumnSplitterState;
namespace PanelChrome = we::runtime::kindui::PropertyPanelChrome;

class DetailsViewWidget final : public Widget {
public:
    DetailsViewWidget() : m_Style(WidgetStyle::Panel()) {}

    void SetTree(std::shared_ptr<IPropertyTree> tree) {
        m_Tree = std::move(tree);
        CaptureBaselines();
    }
    void SetFactory(IPropertyEditorFactory* factory) { m_Factory = factory; }

    void InvalidateEditors() { m_EditorWidgets.clear(); }

    void OnTreeRebuilt() {
        CaptureBaselines();
        InvalidateEditors();
        InvalidateLayout();
        InvalidatePaint();
    }

    void OnPropertyChanged(const PropertyChangeEvent& event) {
        if (event.path.empty()) {
            return;
        }
        auto& state = m_EditState[event.path];
        if (state.baseline.empty() && !event.beforeBytes.empty()) {
            state.baseline = event.beforeBytes;
        }
        if (!event.afterBytes.empty() && !state.baseline.empty()) {
            state.dirty = (event.afterBytes != state.baseline);
        } else {
            state.dirty = true;
        }

        const auto dotPos = event.path.rfind('.');
        if (dotPos != std::string::npos) {
            const std::string parentPath = event.path.substr(0, dotPos);
            auto& parentState = m_EditState[parentPath];
            parentState.dirty = true;
        }

        InvalidateEditors();
        InvalidatePaint();
    }

    Size Measure(const Size& availableSize) override {
        return Size{availableSize.width, availableSize.height};
    }

    void Arrange(const Rect& allottedRect) override {
        m_Geometry = allottedRect;
        SyncScroll();
        LayoutEditors();
    }

    void Paint(PaintContext& context) override {
        if (!m_Visible) return;
        SyncScroll();
        if (!m_Tree || m_Tree->GetFilteredRootNodes().empty()) {
            return;
        }

        // Viewport background cleared by parent panel layout

        const float uiScale = (std::max)(1.0f, DPIContext::GetScale());
        const float viewTop = m_ScrollMetrics.viewport.y;
        const float viewBottom = m_ScrollMetrics.viewport.y + m_ScrollMetrics.viewport.height;
        float y = ContentOriginY();
        context.PushClipRect(m_ScrollMetrics.viewport);

        bool firstRoot = true;
        for (const auto& root : m_Tree->GetFilteredRootNodes()) {
            if (!firstRoot) {
                y += SectionGap();
            }
            y = PaintNode(context, root, y, viewTop, viewBottom, uiScale);
            firstRoot = false;
        }

        context.PopClipRect();
        m_Scroll.Paint(context, m_ScrollMetrics, m_Scroll.IsThumbHovered());
    }

    [[nodiscard]] std::optional<Rect> GetHitTestClipRect() const override {
        return m_ScrollMetrics.viewport;
    }

    [[nodiscard]] bool CanReceiveMouseWheelAt(const Point& pos) const override {
        return ScrollViewport::CanReceiveWheelAt(m_Geometry, m_ScrollMetrics.viewport, m_ScrollMetrics, pos);
    }

    std::shared_ptr<Widget> HitTestPoint(const Point& pos, const Rect* clip) override {
        if (!IsVisible() || IsPointerTransparent() || !IsEnabled()) {
            return nullptr;
        }
        if (!m_Geometry.Contains(pos)) {
            return nullptr;
        }

        SyncScroll();

        const Rect viewportClip = m_ScrollMetrics.viewport;
        Rect effectiveClip = viewportClip;
        if (clip != nullptr) {
            effectiveClip = viewportClip.Intersect(*clip);
        }
        if (effectiveClip.IsEmpty()) {
            return nullptr;
        }

        if (!effectiveClip.Contains(pos)) {
            if (ScrollViewport::ShowsScrollbarCursor(m_ScrollMetrics, pos)) {
                return shared_from_this();
            }
            return nullptr;
        }

        // Keep trailing action / row hover in sync even when a child editor owns
        // the pointer — chrome hover must not depend on who receives OnMouseMove.
        UpdateHoveredChrome(pos);

        if (!m_Tree) {
            return nullptr;
        }

        float y = ContentOriginY();
        bool firstRoot = true;
        for (const auto& root : m_Tree->GetFilteredRootNodes()) {
            if (!firstRoot) {
                y += SectionGap();
            }
            if (auto hit = HitTestNode(root, pos, y, &effectiveClip)) {
                return hit;
            }
            firstRoot = false;
        }
        return nullptr;
    }

    void OnMouseDown(const MouseEvent& event) override {
        SyncScroll();
        if (m_Scroll.OnMouseDown(event, m_ScrollMetrics, m_Geometry.height, m_ContentHeight)) {
            InvalidatePaint();
            return;
        }

        if (event.button == MouseButton::Left) {
            const float dividerX = GetCurrentColumnDividerX();
            if (m_SplitterState.OnMouseDown(event.position, m_ScrollMetrics.viewport, dividerX)) {
                InvalidateEditors();
                InvalidateLayout();
                InvalidatePaint();
                return;
            }
        }

        if (!m_Tree) {
            return;
        }

        if (event.button == MouseButton::Left && HandleActionClick(event.position)) {
            InvalidatePaint();
            return;
        }

        float y = ContentOriginY();
        bool firstRoot = true;
        for (const auto& root : m_Tree->GetFilteredRootNodes()) {
            if (!firstRoot) {
                y += SectionGap();
            }
            if (ToggleLockIconAt(root, event.position, y)) {
                InvalidatePaint();
                return;
            }
            if (ToggleCategoryAt(root, event.position, y)) {
                InvalidateEditors();
                InvalidateLayout();
                InvalidatePaint();
                return;
            }
            firstRoot = false;
        }
    }

    void OnMouseMove(const MouseEvent& event) override {
        m_Scroll.OnMouseMove(event, m_ScrollMetrics, m_Geometry.height, m_ContentHeight);
        UpdateHoveredChrome(event.position);

        const float dividerX = GetCurrentColumnDividerX();
        if (m_SplitterState.OnMouseMove(event.position, m_ScrollMetrics.viewport, dividerX)) {
            InvalidateEditors();
            InvalidateLayout();
            InvalidatePaint();
        }
    }

    void OnMouseUp(const MouseEvent& event) override {
        m_Scroll.OnMouseUp(event);
        const float dividerX = GetCurrentColumnDividerX();
        if (m_SplitterState.OnMouseUp(event.position, m_ScrollMetrics.viewport, dividerX)) {
            InvalidateEditors();
            InvalidateLayout();
            InvalidatePaint();
        }
    }

    void OnMouseWheel(const MouseEvent& event) override {
        SyncScroll();
        m_Scroll.ApplyWheel(event.wheelDeltaY, DetailsWheelStepPx(), m_Geometry.height, m_ContentHeight);
        LayoutEditors();
        InvalidatePaint();
    }

private:
    enum class HoveredAction {
        None,
        Undo
    };

    struct PropertyEditState {
        std::vector<std::uint8_t> baseline;
        bool dirty = false;
    };

    void LayoutEditors() {
        if (!m_Tree) {
            return;
        }

        const float viewTop = m_ScrollMetrics.viewport.y;
        const float viewBottom = m_ScrollMetrics.viewport.y + m_ScrollMetrics.viewport.height;
        float y = ContentOriginY();
        bool firstRoot = true;
        for (const auto& root : m_Tree->GetFilteredRootNodes()) {
            if (!firstRoot) {
                y += SectionGap();
            }
            y = LayoutNode(root, y, viewTop, viewBottom);
            firstRoot = false;
        }
    }

    [[nodiscard]] float GetNodeRowHeight(const PropertyNodePtr& node) const {
        if (!node) {
            return 0.0f;
        }
        if (node->IsCategoryNode()) {
            return CategoryHeight();
        }
        float baseH = RowHeight();
        if (m_Factory && node->GetHandle() && node->GetPropertyInfo()) {
            const std::string path(node->GetPath());
            auto it = m_EditorWidgets.find(path);
            std::shared_ptr<Widget> widget;
            if (it != m_EditorWidgets.end() && it->second) {
                widget = it->second;
            } else {
                if (auto editor = m_Factory->CreateEditor(*node->GetPropertyInfo(), node->GetHandle())) {
                    widget = editor->CreateWidget();
                    m_EditorWidgets[path] = widget;
                }
            }
            if (widget) {
                const Size measured = widget->Measure(Size{ 400.0f, 1000.0f });
                if (measured.height > baseH) {
                    baseH = measured.height;
                }
            }
        }
        return baseH;
    }

    float LayoutNode(
        const PropertyNodePtr& node,
        float y,
        float viewTop,
        float viewBottom)
    {
        if (!node) {
            return y;
        }

        const float height = GetNodeRowHeight(node);
        if (y + height >= viewTop && y <= viewBottom && !node->IsCategoryNode()) {
            if (m_Factory && node->GetHandle() && node->GetPropertyInfo()) {
                auto& editorWidget = m_EditorWidgets[std::string(node->GetPath())];
                if (!editorWidget) {
                    if (auto editor =
                            m_Factory->CreateEditor(*node->GetPropertyInfo(), node->GetHandle())) {
                        editorWidget = editor->CreateWidget();
                    }
                }
                if (editorWidget) {
                    Rect row{
                        m_ScrollMetrics.viewport.x,
                        y,
                        m_ScrollMetrics.viewport.width,
                        height};
                    const auto icons = ResolveActionIcons(node);
                    const bool hasLockIcon = ShouldShowLockIcon(node);
                    const auto layout = PanelChrome::LayoutPropertyRow(row, node->GetDepth(), icons, hasLockIcon,
                        m_SplitterState.GetRatio());
                    // Every editor shares the same compact value band so single
                    // fields and XYZ strips align on one horizontal rhythm.
                    editorWidget->Arrange(PanelChrome::LayoutPropertyControlRect(layout.value));
                }
            }
        }

        y += height;
        if (node->IsExpanded()) {
            for (const auto& child : node->GetChildren()) {
                y = LayoutNode(child, y, viewTop, viewBottom);
            }
        }
        return y;
    }
    void SyncScroll() {
        m_ContentHeight = MeasureContentHeight();
        const float uiScale = std::max(1.0f, DPIContext::GetScale());
        m_ScrollMetrics = m_Scroll.UpdateMetrics(m_Geometry, m_Geometry.height, m_ContentHeight, uiScale);
    }

    [[nodiscard]] float RowHeight() const { return DetailsRowHeightPx(); }
    [[nodiscard]] float CategoryHeight() const { return DetailsSectionHeightPx(); }
    [[nodiscard]] float SectionGap() const { return DetailsSectionGapPx(); }
    [[nodiscard]] float ContentOriginY() const {
        return m_ScrollMetrics.viewport.y - m_Scroll.offset;
    }

    [[nodiscard]] float GetCurrentColumnDividerX() const {
        Rect sampleRow{ m_ScrollMetrics.viewport.x, 0.0f, m_ScrollMetrics.viewport.width, DetailsRowHeightPx() };
        auto layout = PanelChrome::LayoutPropertyRow(sampleRow, 0, {}, false, m_SplitterState.GetRatio());
        return layout.columnDividerX;
    }

    float MeasureContentHeight() const {
        if (!m_Tree) {
            return 0.f;
        }
        const auto& roots = m_Tree->GetFilteredRootNodes();
        if (roots.empty()) {
            return 0.f;
        }
        float h = 0.0f;
        bool firstRoot = true;
        for (const auto& root : roots) {
            if (!firstRoot) {
                h += SectionGap();
            }
            h += MeasureNodeHeight(root);
            firstRoot = false;
        }
        return h;
    }

    float MeasureNodeHeight(const PropertyNodePtr& node) const {
        if (!node) {
            return 0.f;
        }
        float h = GetNodeRowHeight(node);
        if (node->IsExpanded()) {
            for (const auto& child : node->GetChildren()) {
                h += MeasureNodeHeight(child);
            }
        }
        return h;
    }

    float PaintNode(
        PaintContext& context,
        const PropertyNodePtr& node,
        float y,
        float viewTop,
        float viewBottom,
        float uiScale)
    {
        if (!node) {
            return y;
        }

        const float height = GetNodeRowHeight(node);
        if (y + height >= viewTop && y <= viewBottom) {
            Rect row{
                m_ScrollMetrics.viewport.x,
                y,
                m_ScrollMetrics.viewport.width,
                height};

            if (node->IsCategoryNode()) {
                const bool hovered = m_HoveredSectionPath == node->GetPath();
                PanelChrome::PaintSectionHeader(
                    context,
                    row,
                    std::string(node->GetDisplayName()),
                    node->IsExpanded(),
                    hovered);
                PanelChrome::PaintPropertyHorizontalDivider(
                    context,
                    row.y + row.height,
                    row.x,
                    row.x + row.width);
            } else {
                const auto icons = ResolveActionIcons(node);
                const bool hasLockIcon = ShouldShowLockIcon(node);
                const auto layout = PanelChrome::LayoutPropertyRow(row, node->GetDepth(), icons, hasLockIcon,
                    m_SplitterState.GetRatio());
                const bool mixed = node->GetValueState() == PropertyValueState::Mixed;
                const bool rowHovered = m_HoveredPropertyPath == node->GetPath();
                if (rowHovered) {
                    PanelChrome::PaintPropertyRowBackground(context, row, true, false);
                }

                // Paint lock icon next to label if present
                if (hasLockIcon) {
                    PanelChrome::PaintPropertyLockIcon(context, layout.lockIcon, IsPropertyLocked(node));
                }

                PanelChrome::PaintPropertyRowLabel(
                    context,
                    layout.label,
                    std::string(node->GetDisplayName()),
                    mixed);
                PanelChrome::PaintPropertyRowGrid(context, layout);
                PanelChrome::PaintPropertySplitterHighlight(
                    context,
                    layout.columnDividerX,
                    row.y,
                    row.y + row.height,
                    m_SplitterState.IsDragging(),
                    m_SplitterState.IsHovered());

                if (m_Factory && node->GetHandle() && node->GetPropertyInfo()) {
                    auto& editorWidget = m_EditorWidgets[std::string(node->GetPath())];
                    if (!editorWidget) {
                        if (auto editor =
                                m_Factory->CreateEditor(*node->GetPropertyInfo(), node->GetHandle())) {
                            editorWidget = editor->CreateWidget();
                        }
                    }
                    if (editorWidget) {
                        editorWidget->Paint(context);
                    }
                }

                if (icons.Any()) {
                    PanelChrome::PaintPropertyActions(
                        context,
                        PanelChrome::LayoutPropertyActions(layout.actions),
                        icons);
                }
            }
        }

        y += height;
        if (node->IsExpanded()) {
            for (const auto& child : node->GetChildren()) {
                y = PaintNode(context, child, y, viewTop, viewBottom, uiScale);
            }
        }
        return y;
    }

    std::shared_ptr<Widget> HitTestNode(
        const PropertyNodePtr& node,
        const Point& pos,
        float& y,
        const Rect* clip)
    {
        if (!node) {
            return nullptr;
        }

        const float height = GetNodeRowHeight(node);
        Rect row{m_ScrollMetrics.viewport.x, y, m_ScrollMetrics.viewport.width, height};
        if (!row.Contains(pos)) {
            y += height;
            if (node->IsExpanded()) {
                for (const auto& child : node->GetChildren()) {
                    if (auto hit = HitTestNode(child, pos, y, clip)) {
                        return hit;
                    }
                }
            }
            return nullptr;
        }

        if (node->IsCategoryNode()) {
            return shared_from_this();
        }

        const auto icons = ResolveActionIcons(node);
        const bool hasLockIcon = ShouldShowLockIcon(node);
        const auto layout = PanelChrome::LayoutPropertyRow(row, node->GetDepth(), icons, hasLockIcon);
        if (layout.actions.Contains(pos) || layout.label.Contains(pos) || (!layout.lockIcon.IsEmpty() &&
            layout.lockIcon.Contains(pos))) {
            // Label, lock icon, and trailing actions belong to the Inspector chrome, not the
            // value editor, so hover/click stay on the Details view.
            return shared_from_this();
        }

        if (m_Factory && node->GetHandle() && node->GetPropertyInfo()) {
            auto it = m_EditorWidgets.find(std::string(node->GetPath()));
            if (it != m_EditorWidgets.end() && it->second) {
                if (auto hit = it->second->HitTestPoint(pos, clip)) {
                    return hit;
                }
                if (layout.value.Contains(pos)) {
                    return it->second;
                }
            }
        }

        return shared_from_this();
    }

    bool ToggleLockIconAt(const PropertyNodePtr& node, const Point& pos, float& y) {
        if (!node) {
            return false;
        }

        const float height = GetNodeRowHeight(node);
        Rect row{ m_ScrollMetrics.viewport.x, y, m_ScrollMetrics.viewport.width, height };

        if (!node->IsCategoryNode()) {
            const auto icons = ResolveActionIcons(node);
            const bool hasLockIcon = ShouldShowLockIcon(node);
            const auto layout = PanelChrome::LayoutPropertyRow(row, node->GetDepth(), icons, hasLockIcon,
                m_SplitterState.GetRatio());
            if (hasLockIcon && !layout.lockIcon.IsEmpty() && layout.lockIcon.Contains(pos)) {
                const bool currentLocked = we::runtime::kindui::PropertyAspectLockRegistry::IsLocked(node->GetPath(),
                    true);
                we::runtime::kindui::PropertyAspectLockRegistry::SetLocked(node->GetPath(), !currentLocked);
                return true;
            }
        }

        y += height;
        if (node->IsExpanded()) {
            for (const auto& child : node->GetChildren()) {
                if (ToggleLockIconAt(child, pos, y)) {
                    return true;
                }
            }
        }
        return false;
    }

    bool ToggleCategoryAt(const PropertyNodePtr& node, const Point& pos, float& y) {
        if (!node) {
            return false;
        }

        const float height = GetNodeRowHeight(node);
        Rect row{m_ScrollMetrics.viewport.x, y, m_ScrollMetrics.viewport.width, height};
        if (row.Contains(pos) && (node->IsCategoryNode() || !node->GetChildren().empty())) {
            node->SetExpanded(!node->IsExpanded());
            return true;
        }

        y += height;
        if (node->IsExpanded()) {
            for (const auto& child : node->GetChildren()) {
                if (ToggleCategoryAt(child, pos, y)) {
                    return true;
                }
            }
        }
        return false;
    }

    void UpdateHoveredChrome(const Point& pos) {
        if (!m_Tree) {
            return;
        }
        std::string hoveredSection;
        std::string hoveredProperty;
        HoveredAction hoveredAction = HoveredAction::None;
        float y = ContentOriginY();
        bool firstRoot = true;
        for (const auto& root : m_Tree->GetFilteredRootNodes()) {
            if (!firstRoot) {
                y += SectionGap();
            }
            if (FindHoveredChrome(root, pos, y, hoveredSection, hoveredProperty, hoveredAction)) {
                break;
            }
            firstRoot = false;
        }
        if (hoveredSection != m_HoveredSectionPath
            || hoveredProperty != m_HoveredPropertyPath
            || hoveredAction != m_HoveredAction) {
            m_HoveredSectionPath = std::move(hoveredSection);
            m_HoveredPropertyPath = std::move(hoveredProperty);
            m_HoveredAction = hoveredAction;
            InvalidatePaint();
        }
    }

    bool FindHoveredChrome(
        const PropertyNodePtr& node,
        const Point& pos,
        float& y,
        std::string& outSection,
        std::string& outProperty,
        HoveredAction& outAction)
    {
        if (!node) {
            return false;
        }
        const float height = GetNodeRowHeight(node);
        Rect row{ m_ScrollMetrics.viewport.x, y, m_ScrollMetrics.viewport.width, height };
        if (row.Contains(pos)) {
            if (node->IsCategoryNode()) {
                outSection = std::string(node->GetPath());
                return true;
            }

            outProperty = std::string(node->GetPath());
            const auto icons = ResolveActionIcons(node);
            if (icons.Any()) {
                const bool hasLockIcon = ShouldShowLockIcon(node);
                const auto layout = PanelChrome::LayoutPropertyRow(row, node->GetDepth(), icons, hasLockIcon);
                const auto actions = PanelChrome::LayoutPropertyActions(layout.actions);
                if (icons.undo && icons.undoEnabled && actions.undo.Contains(pos)) {
                    outAction = HoveredAction::Undo;
                }
            }
            return true;
        }

        y += height;
        if (node->IsExpanded()) {
            for (const auto& child : node->GetChildren()) {
                if (FindHoveredChrome(child, pos, y, outSection, outProperty, outAction)) {
                    return true;
                }
            }
        }
        return false;
    }

    [[nodiscard]] bool HandleActionClick(const Point& pos) {
        if (!m_Tree) {
            return false;
        }
        float y = ContentOriginY();
        bool firstRoot = true;
        for (const auto& root : m_Tree->GetFilteredRootNodes()) {
            if (!firstRoot) {
                y += SectionGap();
            }
            if (HandleActionClickNode(root, pos, y)) {
                return true;
            }
            firstRoot = false;
        }
        return false;
    }

    bool HandleActionClickNode(const PropertyNodePtr& node, const Point& pos, float& y) {
        if (!node) {
            return false;
        }
        const float height = node->IsCategoryNode() ? CategoryHeight() : RowHeight();
        Rect row{ m_ScrollMetrics.viewport.x, y, m_ScrollMetrics.viewport.width, height };
        if (row.Contains(pos) && !node->IsCategoryNode()) {
            const auto icons = ResolveActionIcons(node);
            const bool hasLockIcon = ShouldShowLockIcon(node);
            if (!icons.Any() && !hasLockIcon) {
                return false;
            }
            const auto layout = PanelChrome::LayoutPropertyRow(row, node->GetDepth(), icons, hasLockIcon);
            const auto actions = PanelChrome::LayoutPropertyActions(layout.actions);
            if (icons.undo && actions.undo.Contains(pos)) {
                if (icons.undoEnabled) {
                    TryUndoProperty(node);
                }
                return true;
            }
            if (hasLockIcon && !layout.lockIcon.IsEmpty() && layout.lockIcon.Contains(pos)) {
                const std::string path(node->GetPath());
                m_LockedProperties[path] = !IsPropertyLocked(node);
                InvalidateEditors();
                InvalidatePaint();
                return true;
            }
            return false;
        }

        y += height;
        if (node->IsExpanded()) {
            for (const auto& child : node->GetChildren()) {
                if (HandleActionClickNode(child, pos, y)) {
                    return true;
                }
            }
        }
        return false;
    }

    static std::size_t GetHandleRawSize(IPropertyHandle* handle) {
        if (!handle) return 0;
        const auto* prop = handle->GetPropertyInfo();
        if (prop && prop->size > 0) {
            return prop->size;
        }
        if (prop) {
            switch (prop->primitive) {
                case we::runtime::reflection::PrimitiveKind::Bool: return sizeof(bool);
                case we::runtime::reflection::PrimitiveKind::Int8:
                case we::runtime::reflection::PrimitiveKind::UInt8: return 1;
                case we::runtime::reflection::PrimitiveKind::Int16:
                case we::runtime::reflection::PrimitiveKind::UInt16: return 2;
                case we::runtime::reflection::PrimitiveKind::Int32:
                case we::runtime::reflection::PrimitiveKind::UInt32:
                case we::runtime::reflection::PrimitiveKind::Float: return 4;
                case we::runtime::reflection::PrimitiveKind::Int64:
                case we::runtime::reflection::PrimitiveKind::UInt64:
                case we::runtime::reflection::PrimitiveKind::Double: return 8;
                case we::runtime::reflection::PrimitiveKind::Vec2: return sizeof(float) * 2;
                case we::runtime::reflection::PrimitiveKind::Vec3: return sizeof(float) * 3;
                case we::runtime::reflection::PrimitiveKind::Vec4: return sizeof(float) * 4;
                default: break;
            }
        }
        std::uint8_t testBuf[64];
        if (handle->GetRaw(testBuf, sizeof(float) * 4)) return sizeof(float) * 4;
        if (handle->GetRaw(testBuf, sizeof(float) * 3)) return sizeof(float) * 3;
        if (handle->GetRaw(testBuf, sizeof(float) * 2)) return sizeof(float) * 2;
        if (handle->GetRaw(testBuf, sizeof(double))) return sizeof(double);
        if (handle->GetRaw(testBuf, sizeof(float))) return sizeof(float);
        if (handle->GetRaw(testBuf, sizeof(bool))) return sizeof(bool);
        return 0;
    }

    [[nodiscard]] PanelChrome::PropertyActionIcons ResolveActionIcons(const PropertyNodePtr& node) const {
        PanelChrome::PropertyActionIcons icons;
        if (!node || node->IsCategoryNode() || !node->GetHandle()) {
            return icons;
        }
        auto handle = node->GetHandle();
        if (!handle->IsReadOnly()) {
            icons.undo = true;
            const std::string path(node->GetPath());
            auto it = m_EditState.find(path);
            if (it == m_EditState.end()) {
                const std::size_t rawSize = GetHandleRawSize(handle.get());
                if (rawSize > 0) {
                    PropertyEditState state;
                    state.baseline.resize(rawSize);
                    if (handle->GetRaw(state.baseline.data(), state.baseline.size())) {
                        it = const_cast<DetailsViewWidget*>(this)->m_EditState.emplace(path, std::move(state)).first;
                    }
                }
            }
            if (it != m_EditState.end()) {
                if (it->second.dirty) {
                    icons.undoEnabled = true;
                } else if (!it->second.baseline.empty()) {
                    std::vector<std::uint8_t> current(it->second.baseline.size());
                    if (handle->GetRaw(current.data(), current.size())) {
                        if (current != it->second.baseline) {
                            icons.undoEnabled = true;
                        }
                    }
                }
            }
        }
        return icons;
    }

    [[nodiscard]] bool ShouldShowLockIcon(const PropertyNodePtr& node) const {
        if (!node || node->IsCategoryNode() || !node->GetHandle()) {
            return false;
        }
        const std::string displayName = std::string(node->GetDisplayName());
        return displayName == "Scale";
    }

    [[nodiscard]] bool IsPropertyLocked(const PropertyNodePtr& node) const {
        if (!node || !node->GetHandle()) {
            return false;
        }
        return we::runtime::kindui::PropertyAspectLockRegistry::IsLocked(node->GetPath(), true);
    }

    void CaptureBaselines() {
        m_EditState.clear();
        if (!m_Tree) {
            return;
        }
        CaptureBaselinesRecursive(m_Tree->GetRootNodes());
    }

    void CaptureBaselinesRecursive(const std::vector<PropertyNodePtr>& nodes) {
        for (const auto& node : nodes) {
            if (!node) {
                continue;
            }
            if (auto handle = node->GetHandle()) {
                if (!handle->IsReadOnly()) {
                    const std::size_t rawSize = GetHandleRawSize(handle.get());
                    if (rawSize > 0) {
                        PropertyEditState state;
                        state.baseline.resize(rawSize);
                        if (handle->GetRaw(state.baseline.data(), state.baseline.size())) {
                            m_EditState[std::string(node->GetPath())] = std::move(state);
                        }
                    }
                }
            }
            CaptureBaselinesRecursive(node->GetChildren());
        }
    }

    bool TryUndoProperty(const PropertyNodePtr& node) {
        if (!node || !node->GetHandle()) {
            return false;
        }
        const std::string path(node->GetPath());
        auto handle = node->GetHandle();
        auto it = m_EditState.find(path);
        if (it == m_EditState.end() || it->second.baseline.empty()) {
            return false;
        }
        if (!handle->SetRaw(it->second.baseline.data(), it->second.baseline.size())) {
            return false;
        }
        it->second.dirty = false;

        for (auto& [key, state] : m_EditState) {
            if (key == path || key.rfind(path + ".", 0) == 0 || path.rfind(key + ".", 0) == 0) {
                state.dirty = false;
            }
        }

        InvalidateEditors();
        InvalidatePaint();
        return true;
    }

    std::shared_ptr<IPropertyTree> m_Tree;
    IPropertyEditorFactory* m_Factory = nullptr;
    ScrollViewport m_Scroll;
    ScrollViewportMetrics m_ScrollMetrics{};
    float m_ContentHeight = 0.f;
    WidgetStyle m_Style;
    std::string m_HoveredSectionPath;
    std::string m_HoveredPropertyPath;
    HoveredAction m_HoveredAction = HoveredAction::None;
    mutable std::unordered_map<std::string, std::shared_ptr<Widget>> m_EditorWidgets;
    std::unordered_map<std::string, PropertyEditState> m_EditState;
    std::unordered_map<std::string, bool> m_LockedProperties;
    PropertyColumnSplitterState m_SplitterState{ "Inspector.DetailsView", 0.40f };
};

class DetailsViewImpl final : public IDetailsView {
public:
    explicit DetailsViewImpl(RuntimeServices services)
        : m_Services(std::move(services))
        , m_Tree(CreatePropertyTree(m_Services))
        , m_Widget(std::make_shared<DetailsViewWidget>())
    {
        m_Widget->SetTree(m_Tree);
        m_Widget->SetFactory(m_Services.factory);
    }

    void SetObject(TypeId typeId, void* instance) override {
        ResetCategoryFilter();
        m_Tree->Build(typeId, instance);
        ResolveObjectTitle(typeId);
        ApplyCustomizations(typeId);
        WireHandleListeners();
        m_Widget->OnTreeRebuilt();
    }

    void SetObjects(TypeId typeId, const std::vector<void*>& instances) override {
        ResetCategoryFilter();
        m_Tree->Build(typeId, instances);
        ResolveObjectTitle(typeId);
        ApplyCustomizations(typeId);
        WireHandleListeners();
        m_Widget->OnTreeRebuilt();
    }

    void SetBindings(const std::vector<ObjectBinding>& bindings) override {
        ResetCategoryFilter();
        m_Tree->BuildBindings(bindings);
        if (!bindings.empty()) {
            ApplyCustomizations(bindings.front().typeId);
        }
        if (m_ObjectTitle.empty() && !bindings.empty()) {
            ResolveObjectTitle(bindings.front().typeId);
        }
        WireHandleListeners();
        WE_LOG_INFO(we::LogCategory::General.data(),
            "[InspectorDebug] Details tree rebuilt: bindings=" + std::to_string(bindings.size()) +
            " roots=" + std::to_string(m_Tree->GetRootNodes().size()) +
            " visibleRoots=" + std::to_string(m_Tree->GetFilteredRootNodes().size()));
        m_Widget->OnTreeRebuilt();
    }

    void Clear() override {
        const std::size_t rootCount = m_Tree ? m_Tree->GetRootNodes().size() : 0;
        if (!m_ObjectTitle.empty() || rootCount != 0) {
            WE_LOG_INFO(we::LogCategory::General.data(),
                "[InspectorDebug] Details cleared: title='" + m_ObjectTitle +
                "' roots=" + std::to_string(rootCount));
        }
        m_Tree->Clear();
        m_ActiveCategory.clear();
        m_ObjectTitle.clear();
        m_ObjectIcon = we::runtime::kindui::kWindIconNone;
        m_Widget->OnTreeRebuilt();
    }

    void SetFilter(const PropertyFilterOptions& filter) override {
        m_Tree->ApplyFilter(filter);
        m_Widget->InvalidateEditors();
    }

    void SetSearchText(std::string_view text) override {
        auto filter = m_Tree->GetFilter();
        filter.searchText = std::string(text);
        SetFilter(filter);
    }

    void SetShowAdvanced(bool show) override {
        auto filter = m_Tree->GetFilter();
        filter.showAdvanced = show;
        m_Tree->ApplyFilter(filter);
        m_Tree->Rebuild();
        m_Widget->InvalidateEditors();
    }

    void SetObjectTitle(std::string title) override {
        m_ObjectTitle = std::move(title);
        WE_LOG_INFO(we::LogCategory::General.data(),
            "[InspectorDebug] Inspector title set to '" + m_ObjectTitle + "'.");
        InvalidateView();
    }
    [[nodiscard]] std::string GetObjectTitle() const override { return m_ObjectTitle; }

    void SetObjectIcon(we::runtime::kindui::WindIconRef icon) override { m_ObjectIcon = icon; }
    [[nodiscard]] we::runtime::kindui::WindIconRef GetObjectIcon() const override { return m_ObjectIcon; }

    [[nodiscard]] bool HasSelection() const override {
        return !m_ObjectTitle.empty() && m_Tree && !m_Tree->GetRootNodes().empty();
    }

    void SetActiveCategory(std::string_view category) override {
        m_ActiveCategory = std::string(category);
        auto filter = m_Tree->GetFilter();
        if (category.empty()) {
            filter.categoryAllowlist.clear();
        } else {
            filter.categoryAllowlist = { std::string(category) };
        }
        SetFilter(filter);
    }

    [[nodiscard]] std::string GetActiveCategory() const override { return m_ActiveCategory; }

    [[nodiscard]] std::vector<std::string> GetCategoryNames() const override {
        std::vector<std::string> names;
        for (const auto& root : m_Tree->GetRootNodes()) {
            if (root && root->IsCategoryNode()) {
                names.emplace_back(root->GetDisplayName());
            }
        }
        return names;
    }

    void ExpandAll() override {
        WE_LOG_INFO(we::LogCategory::General.data(), "[DetailsViewDebug] ExpandAll requested");
        SetExpandedRecursive(m_Tree->GetRootNodes(), true);
    }

    void CollapseAll() override {
        WE_LOG_INFO(we::LogCategory::General.data(), "[DetailsViewDebug] CollapseAll requested");
        SetExpandedRecursive(m_Tree->GetRootNodes(), false);
    }

    void SetCategoryExpanded(std::string_view category, bool expanded) override {
        WE_LOG_INFO(we::LogCategory::General.data(),
            "[DetailsViewDebug] SetCategoryExpanded: category='" + std::string(category) + "' expanded=" +
                (expanded ? "true" : "false"));
        for (const auto& root : m_Tree->GetRootNodes()) {
            if (root && root->IsCategoryNode() && root->GetDisplayName() == category) {
                root->SetExpanded(expanded);
            }
        }
    }

    [[nodiscard]] IPropertyTree& GetTree() noexcept override { return *m_Tree; }
    [[nodiscard]] const IPropertyTree& GetTree() const noexcept override { return *m_Tree; }
    [[nodiscard]] std::shared_ptr<Widget> GetWidget() override { return m_Widget; }

    void AddChangeListener(PropertyChangeListener listener) override {
        m_Listeners.push_back(std::move(listener));
    }

private:
    void ResetCategoryFilter() {
        m_ActiveCategory.clear();
        auto filter = m_Tree->GetFilter();
        filter.categoryAllowlist.clear();
        m_Tree->ApplyFilter(filter);
    }

    void InvalidateView() {
        m_Widget->InvalidateEditors();
        m_Widget->InvalidateLayout();
        m_Widget->InvalidatePaint();
    }

    void SetExpandedRecursive(const std::vector<PropertyNodePtr>& nodes, bool expanded) {
        for (const auto& node : nodes) {
            if (!node) {
                continue;
            }
            node->SetExpanded(expanded);
            SetExpandedRecursive(node->GetChildren(), expanded);
        }
    }

    void ApplyCustomizations(TypeId typeId) {
        if (!m_Services.runtime) {
            return;
        }
        if (auto factory = m_Services.runtime->FindDetailCustomization(typeId)) {
            if (auto customization = factory()) {
                customization->CustomizeDetails(*this);
            }
        }
    }

    void ResolveObjectTitle(TypeId typeId) {
        if (!m_Services.registry) {
            m_ObjectTitle.clear();
            return;
        }
        if (const TypeInfo* info = m_Services.registry->Find(typeId)) {
            m_ObjectTitle = std::string(info->name) + " (Instance)";
        } else {
            m_ObjectTitle.clear();
        }
    }

    void WireHandleListeners() {
        WireNode(m_Tree->GetRootNodes());
    }

    void WireNode(const std::vector<PropertyNodePtr>& nodes) {
        for (const auto& node : nodes) {
            if (!node) {
                continue;
            }
            if (auto handle = node->GetHandle()) {
                handle->AddChangeListener([this](const PropertyChangeEvent& event) {
                    m_Widget->OnPropertyChanged(event);
                    for (const auto& listener : m_Listeners) {
                        if (listener) {
                            listener(event);
                        }
                    }
                });
            }
            WireNode(node->GetChildren());
        }
    }

    RuntimeServices m_Services;
    std::shared_ptr<IPropertyTree> m_Tree;
    std::shared_ptr<DetailsViewWidget> m_Widget;
    std::vector<PropertyChangeListener> m_Listeners;
    std::string m_ObjectTitle;
    we::runtime::kindui::WindIconRef m_ObjectIcon = we::runtime::kindui::kWindIconNone;
    std::string m_ActiveCategory;
};

std::unique_ptr<IDetailsView> CreateDetailsView(RuntimeServices services) {
    return std::make_unique<DetailsViewImpl>(std::move(services));
}

}
}
