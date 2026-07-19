#include "PropertyEditorInternal.h"
#include "PropertyEditor/IDetailsView.h"

#include "KindUI/Core/Style.h"

#include <cmath>

namespace we::editor::property {
namespace detail {

using we::runtime::kindui::ColorToken;
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
namespace PanelChrome = we::editor::panels::PanelChrome;

class DetailsViewWidget final : public Widget {
public:
    DetailsViewWidget() : m_Style(WidgetStyle::Panel()) {}

    void SetTree(std::shared_ptr<IPropertyTree> tree) { m_Tree = std::move(tree); }
    void SetFactory(IPropertyEditorFactory* factory) { m_Factory = factory; }

    void InvalidateEditors() { m_EditorWidgets.clear(); }

    Size Measure(const Size& availableSize) override {
        return Size{availableSize.width, availableSize.height};
    }

    void Arrange(const Rect& allottedRect) override {
        m_Geometry = allottedRect;
        SyncScroll();
    }

    void Paint(PaintContext& context) override {
        PanelChrome::PaintContentRegion(context, m_Geometry);
        SyncScroll();
        if (!m_Tree) {
            return;
        }

        const float uiScale = std::max(1.0f, DPIContext::GetScale());
        const float viewTop = m_ScrollMetrics.viewport.y;
        const float viewBottom = m_ScrollMetrics.viewport.y + m_ScrollMetrics.viewport.height;
        float y = m_ScrollMetrics.viewport.y - m_Scroll.offset;
        context.PushClipRect(m_ScrollMetrics.viewport);

        for (const auto& root : m_Tree->GetFilteredRootNodes()) {
            y = PaintNode(context, root, y, viewTop, viewBottom, uiScale);
        }

        context.PopClipRect();
        m_Scroll.Paint(context, m_ScrollMetrics, m_Scroll.IsThumbHovered());
    }

    void OnMouseDown(const MouseEvent& event) override {
        if (!m_Tree) {
            return;
        }
        SyncScroll();
        if (m_Scroll.OnMouseDown(event, m_ScrollMetrics, m_Geometry.height, m_ContentHeight)) {
            return;
        }

        float y = m_ScrollMetrics.viewport.y - m_Scroll.offset;
        for (const auto& root : m_Tree->GetFilteredRootNodes()) {
            if (HitNode(root, event.position, y)) {
                return;
            }
        }
    }

    void OnMouseMove(const MouseEvent& event) override {
        m_Scroll.OnMouseMove(event, m_ScrollMetrics, m_Geometry.height, m_ContentHeight);
    }

    void OnMouseUp(const MouseEvent& event) override { m_Scroll.OnMouseUp(event); }

    void OnMouseWheel(const MouseEvent& event) override {
        SyncScroll();
        m_Scroll.ApplyWheel(event.wheelDeltaY, 40.f, m_Geometry.height, m_ContentHeight);
    }

private:
    void SyncScroll() {
        m_ContentHeight = MeasureContentHeight();
        m_Scroll.Sync(m_Geometry.height, m_ContentHeight);
        const float uiScale = std::max(1.0f, DPIContext::GetScale());
        m_ScrollMetrics = m_Scroll.ComputeMetrics(m_Geometry, m_ContentHeight, uiScale);
    }

    [[nodiscard]] float RowHeight() const { return 22.f; }
    [[nodiscard]] float CategoryHeight() const { return 22.f; }

    float MeasureContentHeight() const {
        if (!m_Tree) {
            return 0.f;
        }
        float h = 0.f;
        for (const auto& root : m_Tree->GetFilteredRootNodes()) {
            h += MeasureNodeHeight(root);
        }
        return h;
    }

    float MeasureNodeHeight(const PropertyNodePtr& node) const {
        if (!node) {
            return 0.f;
        }
        float h = node->IsCategoryNode() ? CategoryHeight() : RowHeight();
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

        const float height = node->IsCategoryNode() ? CategoryHeight() : RowHeight();
        if (y + height >= viewTop && y <= viewBottom) {
            Rect row{
                m_ScrollMetrics.viewport.x,
                y,
                m_ScrollMetrics.viewport.width,
                height};

            if (node->IsCategoryNode()) {
                PanelChrome::PaintCategoryHeader(
                    context,
                    row,
                    std::string(node->GetDisplayName()),
                    node->IsExpanded(),
                    false);
            } else {
                const float indent = 8.f + static_cast<float>(node->GetDepth()) * 12.f;
                const float labelX = row.x + PanelChrome::PanelPaddingH() + indent;
                const float labelY = row.y + (height - ThemeMetric(MetricToken::TextSizeBody)) * 0.5f;
                const bool mixed = node->GetValueState() == PropertyValueState::Mixed;
                context.DrawText(
                    std::string(node->GetDisplayName()),
                    Point{labelX, labelY},
                    mixed ? ThemeColor(ColorToken::AccentPrimary) : ThemeColor(ColorToken::TextSecondary),
                    ThemeMetric(MetricToken::TextSizeBody));

                // Value column — paint editor widget if available
                if (m_Factory && node->GetHandle() && node->GetPropertyInfo()) {
                    auto& editorWidget = m_EditorWidgets[std::string(node->GetPath())];
                    if (!editorWidget) {
                        if (auto editor =
                                m_Factory->CreateEditor(*node->GetPropertyInfo(), node->GetHandle())) {
                            editorWidget = editor->CreateWidget();
                        }
                    }
                    if (editorWidget) {
                        const float valueX = row.x + 140.f;
                        Rect valueRect{valueX, row.y, row.width - (valueX - row.x) - 8.f, height};
                        editorWidget->Arrange(valueRect);
                        editorWidget->Paint(context);
                    }
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

    bool HitNode(const PropertyNodePtr& node, const Point& pos, float& y) {
        if (!node) {
            return false;
        }
        const float height = node->IsCategoryNode() ? CategoryHeight() : RowHeight();
        Rect row{m_ScrollMetrics.viewport.x, y, m_ScrollMetrics.viewport.width, height};
        if (row.Contains(pos)) {
            if (node->IsCategoryNode() || !node->GetChildren().empty()) {
                node->SetExpanded(!node->IsExpanded());
                InvalidateEditors();
            } else if (m_Factory) {
                auto it = m_EditorWidgets.find(std::string(node->GetPath()));
                if (it != m_EditorWidgets.end() && it->second) {
                    MouseEvent down{};
                    down.type = MouseEventType::MouseDown;
                    down.position = pos;
                    down.button = MouseButton::Left;
                    it->second->OnMouseDown(down);
                }
            }
            return true;
        }
        y += height;
        if (node->IsExpanded()) {
            for (const auto& child : node->GetChildren()) {
                if (HitNode(child, pos, y)) {
                    return true;
                }
            }
        }
        return false;
    }

    std::shared_ptr<IPropertyTree> m_Tree;
    IPropertyEditorFactory* m_Factory = nullptr;
    ScrollViewport m_Scroll;
    ScrollViewportMetrics m_ScrollMetrics{};
    float m_ContentHeight = 0.f;
    WidgetStyle m_Style;
    std::unordered_map<std::string, std::shared_ptr<Widget>> m_EditorWidgets;
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
        m_Tree->Build(typeId, instance);
        ApplyCustomizations(typeId);
        WireHandleListeners();
        m_Widget->InvalidateEditors();
    }

    void SetObjects(TypeId typeId, const std::vector<void*>& instances) override {
        m_Tree->Build(typeId, instances);
        ApplyCustomizations(typeId);
        WireHandleListeners();
        m_Widget->InvalidateEditors();
    }

    void Clear() override {
        m_Tree->Clear();
        m_Widget->InvalidateEditors();
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

    void ExpandAll() override { SetExpandedRecursive(m_Tree->GetRootNodes(), true); }
    void CollapseAll() override { SetExpandedRecursive(m_Tree->GetRootNodes(), false); }

    void SetCategoryExpanded(std::string_view category, bool expanded) override {
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
};

std::unique_ptr<IDetailsView> CreateDetailsView(RuntimeServices services) {
    return std::make_unique<DetailsViewImpl>(std::move(services));
}

} // namespace detail
} // namespace we::editor::property
