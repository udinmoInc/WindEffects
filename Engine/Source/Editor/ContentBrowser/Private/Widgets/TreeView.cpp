#include "Platform/Platform.h"
#include "ContentBrowser/Widgets/TreeView.h"
#include "KindUI/Core/EventSystem.h"
#include "KindUI/Layout/OverlayManager.h"
#include "KindUI/Layout/ScrollViewport.h"
#include "Services/ContentBrowserFolderArt.h"
#include "Services/ContentBrowserBlueprintArt.h"
#include "KindUI/Core/PaintContext.h"
#include "WindEffects/Editor/UI/Panel/PanelChrome.h"
#include "KindUI/Theming/ThemeAccess.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Tokens/DesignSystem.h"
#include "KindUI/Theming/StyleRole.h"
#include "KindUI/Core/WindIcon.h"
#include "KindUI/Core/Icon.h"
#include "KindUI/Core/DPIContext.h"
#include "KindUI/Input/InputEvents.h"
#include "KindUI/Core/ControlChrome.h"
#include "KindUI/Rendering/IconMetrics.h"
#include "KindUI/Profiling/UiGeometryDebug.h"
#include "WindEffects/Editor/UI/Layout/EditorMetrics.h"
#include <algorithm>
#include <cmath>
#include <functional>

using ::we::runtime::kindui::ColorToken;
using ::we::runtime::kindui::MetricToken;
using ::we::runtime::kindui::PaddingToken;
using ::we::runtime::kindui::Point;

namespace we::editor::contentbrowser {
using ::we::runtime::kindui::IconPainter;
using ::we::runtime::kindui::MouseButton;
using ::we::runtime::kindui::KeyCodeToChar;
namespace WindIcons = ::we::runtime::kindui::WindIcons;
using ::we::runtime::kindui::kWindIconNone;
using ::we::runtime::kindui::WindIconRef;
namespace IconMetrics = ::we::runtime::kindui::IconMetrics;
namespace PanelChrome = ::we::editor::panels::PanelChrome;
namespace ControlChrome = ::we::runtime::kindui::ControlChrome;
using ::we::runtime::kindui::DPIContext;


namespace {

using ::we::editor::contentbrowser::ContentBrowserBlueprintArt;
using ::we::editor::contentbrowser::ContentBrowserFolderArt;
namespace EditorMetrics = ::we::editor::layout::EditorMetrics;

float TreeExplorerPrefix(float uiScale) {
    return EditorMetrics::TreeExplorerPrefixWidth() * uiScale;
}

float TreeExpanderHit(float uiScale) {
    return EditorMetrics::TreeExpanderHitSize() * uiScale;
}

float TreeAccessoryColumnX(float viewportX, int column, float uiScale) {
    const float pad = we::runtime::kindui::ResolveMetric(MetricToken::Space2) * uiScale;
    const float hit = TreeExpanderHit(uiScale);
    return viewportX + pad + static_cast<float>(column) * hit;
}


constexpr float kMinTreeZoom = 0.75f;
constexpr float kMaxTreeZoom = 1.75f;
constexpr float kTreeZoomStep = 0.1f;

float TreeUiScale() {
    return (std::max)(1.0f, DPIContext::GetScale());
}

void PaintTreeNodeIcon(PaintContext& context, const TreeNode& node, const Rect& iconRect, bool hovered) {
    (void)hovered;
    if (node.iconTexture != we::rhi::RHIDescriptorSetHandle::Invalid) {
        context.DrawTexture(iconRect, node.iconTexture);
        return;
    }
    if (node.icon.IsValid()) {
        IconPainter::Draw(context, node.icon, iconRect);
    }
}

struct TreeMenuItem {
    std::string label;
    std::function<void()> onClick;
    bool enabled = true;
};

class TreeContextMenu : public Widget {
public:
    TreeContextMenu(std::vector<TreeMenuItem> items, std::function<void()> onDismiss)
        : m_Items(std::move(items)), m_OnDismiss(std::move(onDismiss)) {}

    Size Measure(const Size& availableSize) override {
        (void)availableSize;
        const float itemHeight = ThemeMetric(MetricToken::MenuItemHeight);
        const float menuPad = ThemeMetric(MetricToken::MenuPadding);
        float maxWidth = ThemeMetric(MetricToken::PopupMinWidth);
        for (const auto& item : m_Items) {
            maxWidth = std::max(maxWidth, ThemeMetric(MetricToken::MenuTextIndent)
                + static_cast<float>(item.label.size()) * ThemeMetric(MetricToken::TextSizeSmall) * 0.55f
                + ThemeMetric(MetricToken::Space3));
        }
        m_DesiredSize = Size{ maxWidth, menuPad * 2.0f + m_Items.size() * itemHeight };
        return m_DesiredSize;
    }

    void Arrange(const Rect& allottedRect) override { m_Geometry = allottedRect; }

    void Paint(PaintContext& context) override {
        ControlChrome::PaintPopupSurface(context, m_Geometry);

        const float itemHeight = ThemeMetric(MetricToken::MenuItemHeight);
        const float menuPad = ThemeMetric(MetricToken::MenuPadding);
        const float padX = ThemeMetric(MetricToken::Space2);
        const float textSize = ThemeMetric(MetricToken::TextSizeSmall);
        const float rowRadius = ThemeMetric(MetricToken::CornerRadiusSmall);

        float y = m_Geometry.y + menuPad;
        for (size_t i = 0; i < m_Items.size(); ++i) {
            const auto& item = m_Items[i];
            Rect row{ m_Geometry.x + menuPad, y, m_Geometry.width - menuPad * 2.0f, itemHeight };
            if (!item.enabled) {
                y += itemHeight;
                continue;
            }
            if (static_cast<int>(i) == m_Hovered) {
                ControlChrome::InteractionState state{};
                state.hoverAnim = 1.0f;
                ControlChrome::PaintListRow(context, row, state);
            }
            const float textY = row.y + (row.height - textSize) * 0.5f;
            context.DrawText(item.label, Point{ row.x + padX, textY }, ThemeColor(ColorToken::TextPrimary), textSize);
            y += itemHeight;
        }
    }

    void OnMouseMove(const MouseEvent& event) override {
        m_Hovered = -1;
        const float itemHeight = ThemeMetric(MetricToken::MenuItemHeight);
        const float menuPad = ThemeMetric(MetricToken::MenuPadding);
        float y = m_Geometry.y + menuPad;
        for (size_t i = 0; i < m_Items.size(); ++i) {
            Rect row{ m_Geometry.x + menuPad, y, m_Geometry.width - menuPad * 2.0f, itemHeight };
            if (row.Contains(event.position)) {
                m_Hovered = static_cast<int>(i);
                break;
            }
            y += itemHeight;
        }
    }

    void OnMouseDown(const MouseEvent& event) override {
        if (event.button != MouseButton::Left) {
            return;
        }
        const float itemHeight = ThemeMetric(MetricToken::MenuItemHeight);
        const float menuPad = ThemeMetric(MetricToken::MenuPadding);
        float y = m_Geometry.y + menuPad;
        for (size_t i = 0; i < m_Items.size(); ++i) {
            Rect row{ m_Geometry.x + menuPad, y, m_Geometry.width - menuPad * 2.0f, itemHeight };
            if (row.Contains(event.position) && m_Items[i].enabled && m_Items[i].onClick) {
                m_Items[i].onClick();
                if (auto* overlay = GetPopupHost()) {
                    overlay->CloseAllPopups();
                }
                if (m_OnDismiss) {
                    m_OnDismiss();
                }
                return;
            }
            y += itemHeight;
        }
    }

private:
    std::vector<TreeMenuItem> m_Items;
    std::function<void()> m_OnDismiss;
    int m_Hovered = -1;
};

} // namespace

TreeView::TreeView()
    : m_Style(WidgetStyle::TreeItem())
{
    m_BaseItemHeight = we::runtime::kindui::ResolveMetric(MetricToken::ListRowHeight);
    m_BaseIndentWidth = we::runtime::kindui::ResolveMetric(MetricToken::TreeIndentWidth);
    m_ItemHeight = m_BaseItemHeight;
    m_IndentWidth = m_BaseIndentWidth;
    m_Root = std::make_shared<TreeNode>();
    m_Root->id = "root";
    m_Root->label = "Root";
}

void TreeView::SyncScrollMetrics() {
    const float headerHeight = (m_ExplorerStyle && m_ShowColumnHeader)
        ? PanelChrome::ColumnHeaderRowHeight()
        : 0.0f;
    Rect viewportGeom = m_Geometry;
    viewportGeom.y += headerHeight;
    viewportGeom.height = (std::max)(0.0f, viewportGeom.height - headerHeight);
    m_ScrollMetrics = m_Scroll.UpdateMetrics(viewportGeom, viewportGeom.height, m_ContentHeight, TreeUiScale());
}

void TreeView::ScrollSelectionIntoView() {
    if (m_SelectedIds.empty()) {
        return;
    }

    const std::string& selectedId = m_SelectedIds.back();
    const float rowHeight = m_ItemHeight * TreeUiScale();
    for (const auto& item : m_RenderList) {
        if (item.node->id != selectedId) {
            continue;
        }
        const float top = static_cast<float>(item.flatIndex) * rowHeight;
        const float bottom = top + rowHeight;
        if (m_Scroll.ScrollToRange(top, bottom, m_Geometry.height, m_ContentHeight)) {
            Arrange(m_Geometry);
        }
        break;
    }
}

Size TreeView::Measure(const Size& availableSize) {
    if (m_RenderListDirty) {
        BuildRenderList();
    }
    m_ContentHeight = static_cast<float>(m_RenderList.size()) * m_ItemHeight * TreeUiScale();
    return Size{ availableSize.width, availableSize.height };
}

void TreeView::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
    if (m_RenderListDirty) {
        BuildRenderList();
    }
    SyncScrollMetrics();
    UpdateVisibleRange();

    const float uiScale = TreeUiScale();
    const float rowHeight = m_ItemHeight * uiScale;
    const float viewportX = m_ScrollMetrics.viewport.x;
    const float viewportWidth = m_ScrollMetrics.viewport.width;
    const float indentOffset = m_ExplorerStyle ? TreeExplorerPrefix(uiScale) : 0.0f;
    const int first = std::max(0, m_FirstVisibleIndex - 2);
    const int last = std::min(static_cast<int>(m_RenderList.size()) - 1, m_LastVisibleIndex + 2);
    float y = m_ScrollMetrics.viewport.y - m_Scroll.offset
        + static_cast<float>(first) * rowHeight;
    for (int i = first; i <= last && i < static_cast<int>(m_RenderList.size()); ++i) {
        auto& item = m_RenderList[static_cast<size_t>(i)];
        item.flatIndex = i;
        item.geometry = Rect{
            viewportX + indentOffset + item.depth * m_IndentWidth,
            y,
            std::max(0.0f, viewportWidth - indentOffset - item.depth * m_IndentWidth),
            rowHeight
        };
        y += rowHeight;
    }
}

void TreeView::Tick(float deltaTime) {
    Widget::Tick(deltaTime);
    if (!m_RenamingId.empty()) {
        m_RenameCursorBlink += deltaTime;
    }
}

TreeView::TreeRowLayoutSlots TreeView::ComputeTreeRowLayout(const RenderItem& item) const {
    TreeRowLayoutSlots layout{};
    const float uiScale = TreeUiScale();
    const float rowHeight = m_ItemHeight * uiScale;
    const float viewportX = m_ScrollMetrics.viewport.x;
    const float viewportWidth = m_ScrollMetrics.viewport.width;

    layout.rowBounds = Rect{ viewportX, item.geometry.y, viewportWidth, rowHeight };

    const float accessorySize = static_cast<float>(16u) * uiScale;
    const float prefixOffset = m_ExplorerStyle ? TreeExplorerPrefix(uiScale) : 0.0f;
    const float centerY = item.geometry.y + rowHeight * 0.5f;

    if (m_ExplorerStyle) {
        const float hitSize = TreeExpanderHit(uiScale);
        layout.eyeBounds = Rect{
            TreeAccessoryColumnX(viewportX, 0, uiScale),
            centerY - hitSize * 0.5f,
            hitSize,
            hitSize };
        layout.lockBounds = Rect{
            TreeAccessoryColumnX(viewportX, 1, uiScale),
            centerY - hitSize * 0.5f,
            hitSize,
            hitSize };
    }

    layout.indentX = viewportX + prefixOffset + (static_cast<float>(item.depth) * m_IndentWidth);

    const float expanderWidth = TreeExpanderHit(uiScale);
    layout.expanderBounds = Rect{ layout.indentX, item.geometry.y, expanderWidth, rowHeight };
    layout.hasExpander = !item.node->children.empty();

    const float contentSlotX = layout.indentX + expanderWidth + ThemeMetric(MetricToken::SpaceXS) * uiScale;

    const float iconSize = static_cast<float>(16u);
    const bool hasIcon = item.node->icon.IsValid() || item.node->iconTexture != we::rhi::RHIDescriptorSetHandle::Invalid;
    layout.hasIcon = hasIcon;
    if (hasIcon) {
        layout.iconBounds = Rect{ contentSlotX, centerY - iconSize * 0.5f, iconSize, iconSize };
        layout.textX = contentSlotX + iconSize + 4.0f * uiScale;
    } else {
        layout.iconBounds = Rect{ contentSlotX, item.geometry.y, 0.0f, 0.0f };
        layout.textX = contentSlotX;
    }

    float rightReservedX = viewportX + viewportWidth - 12.0f * uiScale;

    if (m_ShowRowControls && !m_ExplorerStyle) {
        const float controlGap = 4.0f * uiScale;
        rightReservedX -= accessorySize;
        layout.eyeBounds = Rect{ rightReservedX, centerY - accessorySize * 0.5f, accessorySize, accessorySize };
        rightReservedX -= (accessorySize + controlGap);
        layout.lockBounds = Rect{ rightReservedX, centerY - accessorySize * 0.5f, accessorySize, accessorySize };
        rightReservedX -= controlGap;
    }

    if (!item.node->typeName.empty()) {
        const float typeWidth = 100.0f * uiScale;
        rightReservedX -= (typeWidth + 8.0f * uiScale);
        layout.typeBounds = Rect{ rightReservedX, item.geometry.y, typeWidth, rowHeight };
    }

    layout.maxTextWidth = (std::max)(10.0f, rightReservedX - layout.textX - 4.0f * uiScale);
    return layout;
}

void TreeView::Paint(PaintContext& context) {
    const float uiScale = TreeUiScale();
    const float fontSize = m_Style.text.size * uiScale;

    if (m_ExplorerStyle && m_ShowColumnHeader) {
        const float headerHeight = PanelChrome::ColumnHeaderRowHeight();
        const Rect headerRect{ m_Geometry.x, m_Geometry.y, m_Geometry.width, headerHeight };

        PanelChrome::PaintHeaderRegion(context, headerRect);

        const float glyphTier = static_cast<float>(16u);
        const float headerTextSize = ThemeMetric(MetricToken::TextSizeCaption) * uiScale;
        const float headerTextY = m_Geometry.y + (headerHeight - headerTextSize) * 0.5f;

        // Eye icon column header
        const float eyeX = TreeAccessoryColumnX(m_Geometry.x, 0, uiScale);
        Rect eyeBand{ eyeX, m_Geometry.y, glyphTier, headerHeight };
        IconPainter::Draw(context, WindIcons::Eye16, IconMetrics::PlaceGlyphCentered(eyeBand, 16u));

        // Pin/Lock column header
        const float lockX = TreeAccessoryColumnX(m_Geometry.x, 1, uiScale);
        Rect lockBand{ lockX, m_Geometry.y, glyphTier, headerHeight };
        IconPainter::Draw(context, we::runtime::kindui::kWindIconNone, IconMetrics::PlaceGlyphCentered(lockBand, 16u));

        // Item Label header
        const float labelX = m_Geometry.x + TreeExplorerPrefix(uiScale);
        context.DrawText("Item Label ▲", Point{ labelX, headerTextY }, ThemeColor(ColorToken::TextSecondary), headerTextSize, true);

        // Type column header
        const float typeColumnReserve = we::runtime::kindui::ResolveMetric(MetricToken::Space6) * uiScale;
        const float typeRightX = m_Geometry.x + m_Geometry.width - typeColumnReserve;
        const float typeWidth = context.GetTextWidth("Type", headerTextSize);
        context.DrawText("Type", Point{ typeRightX - typeWidth, headerTextY }, ThemeColor(ColorToken::TextSecondary), headerTextSize, true);
    }

    if (m_RenderList.empty()) {
        return;
    }

    SyncScrollMetrics();
    UpdateVisibleRange();

    if (m_PaintNavigationBackground && m_ScrollMetrics.viewport.width > 0.0f && m_ScrollMetrics.viewport.height > 0.0f) {
        context.DrawRect(
            m_ScrollMetrics.viewport,
            we::runtime::kindui::ds::Panel::NavigationBackground());
    }

    const float viewTop = m_ScrollMetrics.viewport.y;
    const float viewBottom = m_ScrollMetrics.viewport.y + m_ScrollMetrics.viewport.height;

    context.PushClipRect(m_ScrollMetrics.viewport);

    for (int i = m_FirstVisibleIndex; i <= m_LastVisibleIndex && i < static_cast<int>(m_RenderList.size()); ++i) {
        const auto& item = m_RenderList[static_cast<size_t>(i)];
        const auto& node = item.node;
        const float rowHeight = item.geometry.height;

        if (item.geometry.y + item.geometry.height < viewTop ||
            item.geometry.y > viewBottom) {
            continue;
        }

        const TreeRowLayoutSlots layout = ComputeTreeRowLayout(item);
        const bool selected = IsSelected(node->id);
        const bool hovered = node->id == m_HoveredId;

        // Full-Width Row Background
        ::we::editor::panels::PanelChrome::PaintAlternatingListRowBackground(
            context, layout.rowBounds, item.flatIndex);
        if (selected || hovered) {
            ::we::editor::panels::PanelChrome::PaintListRowBackground(
                context, layout.rowBounds, hovered, selected, IsFocused());
        }

        // Drop Target Indicator Line
        if (node->id == m_DropTargetId && m_Dragging) {
            Rect dropLine{
                m_ScrollMetrics.viewport.x + 4.0f,
                layout.rowBounds.y,
                m_ScrollMetrics.viewport.width - 8.0f,
                2.0f
            };
            context.DrawRect(dropLine, ThemeColor(ColorToken::AccentPrimary));
        }

        // Left Controls (ExplorerStyle)
        if (m_ExplorerStyle) {
            if (hovered || selected || !node->visible) {
                const Color eyeColor = node->visible ? ThemeColor(ColorToken::TextSecondary) : ThemeColor(ColorToken::TextSecondary) * 0.45f;
                const WindIconRef eyeIcon = node->visible ? WindIcons::Eye16 : kWindIconNone;
                IconPainter::Draw(context, eyeIcon, IconMetrics::PlaceGlyphCentered(layout.eyeBounds, 16u));
            }
            if (node->locked) {
                const Color lockColor = ThemeColor(ColorToken::Warning);
                IconPainter::Draw(context, we::runtime::kindui::kWindIconNone, IconMetrics::PlaceGlyphCentered(layout.lockBounds, 16u));
            }
        }

        // Expander Chevron
        if (layout.hasExpander) {
            const WindIconRef chevronIcon = node->expanded ? WindIcons::ChevronDown16 : WindIcons::ChevronRight16;
            IconPainter::Draw(context, chevronIcon, IconMetrics::PlaceGlyphCentered(layout.expanderBounds, 16u));
        }

        // Node Icon
        if (layout.hasIcon) {
            Rect iconRect = IconMetrics::PlaceGlyphCentered(layout.iconBounds, layout.iconBounds.width);
            PaintTreeNodeIcon(context, *node, iconRect, hovered);
        }

        // Node Label Text (With Search Highlighting & Text Clipping)
        const float textY = layout.rowBounds.y + (rowHeight - fontSize) * 0.5f;
        Color textColor = node->locked ? ThemeColor(ColorToken::TextSecondary) * 0.6f : ThemeColor(ColorToken::TextPrimary);
        if (!node->visible) {
            textColor = ThemeColor(ColorToken::TextSecondary) * 0.45f;
        }

        context.PushClipRect(Rect{ layout.textX, layout.rowBounds.y, layout.maxTextWidth, rowHeight });

        if (node->id == m_RenamingId) {
            Rect editBg{ layout.textX - 4.0f, layout.rowBounds.y + 2.0f, (std::max)(40.0f, layout.maxTextWidth), rowHeight - 4.0f };
            context.DrawRoundedRect(editBg, ThemeColor(ColorToken::InputBackground), 3.0f);
            context.DrawRoundedRectOutline(editBg, ThemeColor(ColorToken::AccentPrimary), 1.0f, 3.0f);
            context.DrawText(m_RenameBuffer, Point{ layout.textX, textY }, ThemeColor(ColorToken::TextPrimary), fontSize);
            if (static_cast<int>(m_RenameCursorBlink * 2.0f) % 2 == 0) {
                const float cursorX = layout.textX + context.GetTextWidth(m_RenameBuffer, fontSize) + 1.0f;
                context.DrawRect(Rect{ cursorX, textY, 1.0f, fontSize }, ThemeColor(ColorToken::TextPrimary));
            }
        } else {
            if (!m_SearchQuery.empty()) {
                const std::string& label = node->label;
                const std::string& query = m_SearchQuery;
                size_t matchStart = 0;
                size_t matchEnd = 0;
                bool foundMatch = false;

                for (size_t searchIdx = 0; searchIdx <= label.size() - query.size() && !foundMatch; ++searchIdx) {
                    bool matches = true;
                    for (size_t j = 0; j < query.size(); ++j) {
                        if (std::tolower(label[searchIdx + j]) != std::tolower(query[j])) {
                            matches = false;
                            break;
                        }
                    }
                    if (matches) {
                        matchStart = searchIdx;
                        matchEnd = searchIdx + query.size();
                        foundMatch = true;
                    }
                }

                if (foundMatch) {
                    const std::string beforeMatch = label.substr(0, matchStart);
                    float currentX = layout.textX;
                    if (!beforeMatch.empty()) {
                        context.DrawText(beforeMatch, Point{ currentX, textY }, textColor, fontSize);
                        currentX += context.GetTextWidth(beforeMatch, fontSize);
                    }
                    const std::string matchText = label.substr(matchStart, matchEnd - matchStart);
                    const float matchWidth = context.GetTextWidth(matchText, fontSize);
                    Rect highlightRect{ currentX, textY, matchWidth, fontSize };
                    context.DrawRoundedRect(highlightRect, ThemeColor(ColorToken::AccentPrimary) * 0.3f, 2.0f);
                    context.DrawText(matchText, Point{ currentX, textY }, ThemeColor(ColorToken::AccentPrimary), fontSize);
                    currentX += matchWidth;
                    const std::string afterMatch = label.substr(matchEnd);
                    if (!afterMatch.empty()) {
                        context.DrawText(afterMatch, Point{ currentX, textY }, textColor, fontSize);
                    }
                } else {
                    context.DrawText(node->label, Point{ layout.textX, textY }, textColor, fontSize);
                }
            } else {
                context.DrawText(node->label, Point{ layout.textX, textY }, textColor, fontSize);
            }
        }

        context.PopClipRect();

        // Trailing Type Column (Right-Aligned)
        if (!node->typeName.empty()) {
            const float typeFontSize = fontSize * 0.9f;
            const float typeWidth = context.GetTextWidth(node->typeName, typeFontSize);
            const float typeColumnReserve = we::runtime::kindui::ResolveMetric(MetricToken::Space6) * uiScale;
            const float typeRightX = m_ScrollMetrics.viewport.x + m_ScrollMetrics.viewport.width - typeColumnReserve;
            const float typeY = layout.rowBounds.y + (rowHeight - typeFontSize) * 0.5f;
            context.DrawText(node->typeName, Point{ typeRightX - typeWidth, typeY }, ThemeColor(ColorToken::TextSecondary), typeFontSize);
        }

        // Trailing Row Controls (Non-ExplorerStyle)
        if (m_ShowRowControls && !m_ExplorerStyle) {
            const Color eyeColor = node->visible ? ThemeColor(ColorToken::TextSecondary) : ThemeColor(ColorToken::TextSecondary) * 0.45f;
            const WindIconRef eyeIcon = node->visible ? WindIcons::Eye16 : kWindIconNone;
            IconPainter::Draw(context, eyeIcon, IconMetrics::PlaceGlyphCentered(layout.eyeBounds, 16u));

            if (node->locked) {
                IconPainter::Draw(context, kWindIconNone, IconMetrics::PlaceGlyphCentered(layout.lockBounds, 16u));
            }
        }
    }

    context.PopClipRect();
    m_Scroll.Paint(context, m_ScrollMetrics, m_Scroll.IsThumbHovered());

    if (we::runtime::kindui::UiGeometryDebug::IsEnabled() && !m_RenderList.empty()) {
        const float rowHeight = m_ItemHeight * uiScale;
        we::runtime::kindui::UiGeometryDebug::Get().TraceRegion(
            "OutlinerRow",
            Rect{ m_Geometry.x, m_Geometry.y, m_Geometry.width, rowHeight },
            "TreeView",
            EditorMetrics::TreeIndent(),
            0.0f,
            fontSize,
            16.0f);
    }
}

void TreeView::OnMouseDown(const MouseEvent& event) {
    SyncScrollMetrics();
    if (m_Scroll.OnMouseDown(event, m_ScrollMetrics, m_Geometry.height, m_ContentHeight)) {
        Arrange(m_Geometry);
        return;
    }

    if (event.button == MouseButton::Right) {
        RenderItem* item = GetItemAtPosition(event.position);
        if (item) {
            HandleSelection(item->node->id, event.shiftDown, event.ctrlDown);
            ShowContextMenu(item->node->id, event.position);
        }
        return;
    }

    if (!m_RenamingId.empty()) {
        CommitRename();
    }

    RenderItem* item = GetItemAtPosition(event.position);
    if (!item) {
        return;
    }

    const TreeRowLayoutSlots layout = ComputeTreeRowLayout(*item);
    const auto& node = item->node;

    if (layout.hasExpander && layout.expanderBounds.Contains(event.position)) {
        ToggleExpand(node->id);
        return;
    }

    if (layout.lockBounds.width > 0.0f && layout.lockBounds.Contains(event.position)) {
        node->locked = !node->locked;
        if (m_OnLockToggled) {
            m_OnLockToggled(node->id, node->locked);
        }
        return;
    }

    if (layout.eyeBounds.width > 0.0f && layout.eyeBounds.Contains(event.position)) {
        node->visible = !node->visible;
        if (m_OnVisibilityToggled) {
            m_OnVisibilityToggled(node->id, node->visible);
        }
        return;
    }

    HandleSelection(node->id, event.shiftDown, event.ctrlDown);
    m_Dragging = false;
    m_DragStart = event.position;
    m_DragSourceId = node->id;
}

void TreeView::OnMouseUp(const MouseEvent& event) {
    m_Scroll.OnMouseUp(event);

    if (m_Dragging && !m_DropTargetId.empty() && m_DropTargetId != m_DragSourceId) {
        if (m_OnReparentRequested) {
            m_OnReparentRequested(m_DragSourceId, m_DropTargetId);
        }
    }
    m_Dragging = false;
    m_DropTargetId.clear();

    static std::string lastClickedId;
    static uint64_t lastClickTime = 0;

    RenderItem* item = GetItemAtPosition(event.position);
    if (!item) {
        return;
    }

    const auto& platform = we::platform::Platform::Get();
    const uint64_t now = platform.GetHighResolutionCounter();
    const uint64_t freq = platform.GetHighResolutionFrequency();
    const double elapsed = static_cast<double>(now - lastClickTime) / static_cast<double>(freq);

    if (item->node->id == lastClickedId && elapsed < 0.3) {
        if (m_OnItemDoubleClicked) {
            m_OnItemDoubleClicked(item->node->id);
        }
        BeginRename(item->node->id);
    }

    lastClickedId = item->node->id;
    lastClickTime = now;
}

void TreeView::OnMouseMove(const MouseEvent& event) {
    SyncScrollMetrics();
    m_Scroll.OnMouseMove(event, m_ScrollMetrics, m_Geometry.height, m_ContentHeight);
    if (m_Scroll.IsDraggingThumb()) {
        Arrange(m_Geometry);
        return;
    }

    RenderItem* item = GetItemAtPosition(event.position);
    m_HoveredId = item ? item->node->id : "";

    if (!m_DragSourceId.empty()) {
        const float dx = event.position.x - m_DragStart.x;
        const float dy = event.position.y - m_DragStart.y;
        if (!m_Dragging && std::sqrt(dx * dx + dy * dy) > 5.0f) {
            m_Dragging = true;
        }
        if (m_Dragging) {
            m_DropTargetId = item ? item->node->id : "";
        }
    }
}

void TreeView::OnMouseWheel(const MouseEvent& event) {
    if (event.ctrlDown) {
        SetZoomLevel(m_ZoomLevel + event.wheelDeltaY * kTreeZoomStep);
        Arrange(m_Geometry);
        InvalidatePaint();
        return;
    }

    SyncScrollMetrics();
    m_Scroll.ApplyWheel(
        event.wheelDeltaY,
        m_ItemHeight * TreeUiScale() * 0.75f,
        m_Geometry.height,
        m_ContentHeight);
    Arrange(m_Geometry);
    InvalidatePaint();
}

bool TreeView::CanReceiveMouseWheelAt(const Point& pos) const {
    return ScrollViewport::CanReceiveWheelAt(m_Geometry, m_ScrollMetrics.viewport, m_ScrollMetrics, pos);
}

void TreeView::SetItemHeight(float height) {
    m_BaseItemHeight = std::max(12.0f, height);
    m_ItemHeight = m_BaseItemHeight * m_ZoomLevel;
}

void TreeView::SetIndentWidth(float width) {
    m_BaseIndentWidth = std::max(8.0f, width);
    m_IndentWidth = m_BaseIndentWidth * m_ZoomLevel;
}

void TreeView::SetZoomLevel(float zoomLevel) {
    m_ZoomLevel = std::clamp(zoomLevel, kMinTreeZoom, kMaxTreeZoom);
    m_ItemHeight = m_BaseItemHeight * m_ZoomLevel;
    m_IndentWidth = m_BaseIndentWidth * m_ZoomLevel;
    m_Style.text.size = std::clamp(13.0f * m_ZoomLevel, 10.0f, 20.0f);
}

void TreeView::OnKeyDown(const KeyEvent& event) {
    if (!m_RenamingId.empty()) {
        if (event.key == we::platform::KeyCode::Escape) {
            CancelRename();
            return;
        }
        if (event.key == we::platform::KeyCode::Enter || event.key == we::platform::KeyCode::NumpadEnter) {
            CommitRename();
            return;
        }
        if (event.key == we::platform::KeyCode::Backspace && !m_RenameBuffer.empty()) {
            m_RenameBuffer.pop_back();
            return;
        }
        if (const char ch = KeyCodeToChar(event.key, event.shiftDown); ch != '\0' && m_RenameBuffer.size() < 96) {
            m_RenameBuffer.push_back(ch);
        }
        return;
    }

    if (event.key == we::platform::KeyCode::F2 && !m_SelectedIds.empty()) {
        BeginRename(m_SelectedIds.back());
    }
}

void TreeView::SetRoot(const std::shared_ptr<TreeNode>& root) {
    m_Root = root;
    m_SelectedIds.clear();
    MarkRenderListDirty();
    BuildRenderList();
}

void TreeView::AddItem(const std::shared_ptr<TreeNode>& item, const std::string& parentId) {
    if (parentId.empty()) {
        m_Root->children.push_back(item);
    } else if (auto parent = FindNode(parentId)) {
        parent->children.push_back(item);
    }
    MarkRenderListDirty();
    BuildRenderList();
}

void TreeView::RemoveItem(const std::string& id) {
    std::function<bool(std::vector<std::shared_ptr<TreeNode>>&)> removeRecursive =
        [&](std::vector<std::shared_ptr<TreeNode>>& nodes) -> bool {
            for (auto it = nodes.begin(); it != nodes.end(); ++it) {
                if ((*it)->id == id) {
                    nodes.erase(it);
                    return true;
                }
                if (removeRecursive((*it)->children)) {
                    return true;
                }
            }
            return false;
        };
    removeRecursive(m_Root->children);
    MarkRenderListDirty();
    BuildRenderList();
}

void TreeView::Clear() {
    m_Root->children.clear();
    m_SelectedIds.clear();
    MarkRenderListDirty();
    BuildRenderList();
}

void TreeView::SetSelectedId(const std::string& id) {
    m_SelectedIds = id.empty() ? std::vector<std::string>{} : std::vector<std::string>{ id };
}

void TreeView::SetSelectedIds(const std::vector<std::string>& ids) {
    m_SelectedIds = ids;
}

std::string TreeView::GetSelectedId() const {
    return m_SelectedIds.empty() ? std::string{} : m_SelectedIds.back();
}

void TreeView::BuildRenderList() {
    if (!m_RenderListDirty && !m_RenderList.empty()) {
        return;
    }

    m_RenderList.clear();

    // Fuzzy match helper function
    auto fuzzyMatch = [](const std::string& text, const std::string& pattern) -> bool {
        if (pattern.empty()) return true;
        
        size_t textIdx = 0;
        size_t patternIdx = 0;
        
        while (textIdx < text.size() && patternIdx < pattern.size()) {
            if (std::tolower(text[textIdx]) == std::tolower(pattern[patternIdx])) {
                patternIdx++;
            }
            textIdx++;
        }
        
        return patternIdx == pattern.size();
    };

    // Check if node matches filter options
    auto matchesFilter = [this, fuzzyMatch](const std::shared_ptr<TreeNode>& node) -> bool {
        // Search query filter
        if (!m_SearchQuery.empty() && !fuzzyMatch(node->label, m_SearchQuery)) {
            return false;
        }
        
        // Hidden items filter
        if (!m_FilterOptions.showHidden && !node->visible) {
            return false;
        }
        
        // Locked items filter
        if (!m_FilterOptions.showLocked && node->locked) {
            return false;
        }
        
        // Empty folders filter
        if (!m_FilterOptions.showEmptyFolders && node->children.empty() && !node->icon.IsValid()) {
            return false;
        }
        
        return true;
    };

    std::function<void(const std::shared_ptr<TreeNode>&, int, bool)> buildRecursive =
        [&](const std::shared_ptr<TreeNode>& node, int depth, bool parentMatches) {
            if (node->id != "root") {
                const bool nodeMatches = matchesFilter(node);
                const bool shouldShow = nodeMatches || parentMatches;
                
                if (shouldShow) {
                    m_RenderList.push_back({ node, depth, 0, Rect{} });
                }
                
                // Always expand children if searching or if parent matches
                if (nodeMatches || parentMatches || !m_SearchQuery.empty()) {
                    for (const auto& child : node->children) {
                        buildRecursive(child, depth + 1, shouldShow);
                    }
                } else if (node->expanded) {
                    for (const auto& child : node->children) {
                        buildRecursive(child, depth + 1, false);
                    }
                }
            } else {
                // Root node - process children
                for (const auto& child : node->children) {
                    buildRecursive(child, 0, false);
                }
            }
        };

    buildRecursive(m_Root, 0, false);

    m_ContentHeight = static_cast<float>(m_RenderList.size()) * m_ItemHeight * TreeUiScale();
    m_RenderListDirty = false;
}

void TreeView::UpdateVisibleRange() {
    if (m_RenderList.empty()) {
        m_FirstVisibleIndex = 0;
        m_LastVisibleIndex = -1;
        return;
    }

    const float rowHeight = m_ItemHeight * TreeUiScale();
    const float viewTop = m_Geometry.y;
    const float viewBottom = m_Geometry.y + m_Geometry.height;
    const int overscan = 2;

    m_FirstVisibleIndex = static_cast<int>(std::floor(m_Scroll.offset / rowHeight));
    m_FirstVisibleIndex = std::max(0, m_FirstVisibleIndex - overscan);

    const int visibleCount = static_cast<int>(std::ceil(m_Geometry.height / rowHeight)) + overscan * 2;
    m_LastVisibleIndex = std::min(static_cast<int>(m_RenderList.size()) - 1, m_FirstVisibleIndex + visibleCount);

    (void)viewTop;
    (void)viewBottom;
}

TreeView::RenderItem* TreeView::GetItemAtPosition(const Point& pos) {
    if (m_RenderList.empty()) return nullptr;

    const float rowHeight = m_ItemHeight * TreeUiScale();
    if (rowHeight > 0.0f) {
        const float viewTop = m_ScrollMetrics.viewport.y - m_Scroll.offset;
        const float relY = pos.y - viewTop;
        if (relY >= 0.0f) {
            const size_t index = static_cast<size_t>(relY / rowHeight);
            if (index < m_RenderList.size()) {
                auto& item = m_RenderList[index];
                if (item.geometry.Contains(pos)) {
                    return &item;
                }
            }
        }
    }

    for (auto& item : m_RenderList) {
        if (item.geometry.Contains(pos)) {
            return &item;
        }
    }
    return nullptr;
}

std::shared_ptr<TreeNode> TreeView::FindNode(const std::string& id) {
    std::function<std::shared_ptr<TreeNode>(const std::shared_ptr<TreeNode>&)> findRecursive =
        [&](const std::shared_ptr<TreeNode>& node) -> std::shared_ptr<TreeNode> {
            if (node->id == id) {
                return node;
            }
            for (const auto& child : node->children) {
                if (auto found = findRecursive(child)) {
                    return found;
                }
            }
            return nullptr;
        };

    if (m_Root->id == id) {
        return m_Root;
    }
    for (const auto& child : m_Root->children) {
        if (auto found = findRecursive(child)) {
            return found;
        }
    }
    return nullptr;
}

void TreeView::ToggleExpand(const std::string& id) {
    if (auto node = FindNode(id)) {
        node->expanded = !node->expanded;
    }
    MarkRenderListDirty();
    BuildRenderList();
    Arrange(m_Geometry);
}

void TreeView::BeginRename(const std::string& id) {
    if (auto node = FindNode(id)) {
        m_RenamingId = id;
        m_RenameBuffer = node->label;
        m_RenameCursorBlink = 0.0f;
    }
}

void TreeView::CommitRename() {
    if (m_RenamingId.empty()) {
        return;
    }
    if (auto node = FindNode(m_RenamingId)) {
        if (!m_RenameBuffer.empty() && m_RenameBuffer != node->label) {
            node->label = m_RenameBuffer;
            if (m_OnRenameCommitted) {
                m_OnRenameCommitted(m_RenamingId, m_RenameBuffer);
            }
        }
    }
    m_RenamingId.clear();
    m_RenameBuffer.clear();
}

void TreeView::CancelRename() {
    m_RenamingId.clear();
    m_RenameBuffer.clear();
}

void TreeView::ShowContextMenu(const std::string& id, const Point& position) {
    auto makeItem = [](const std::string& label, std::function<void()> onClick, bool enabled = true) {
        TreeMenuItem item;
        item.label = label;
        item.onClick = std::move(onClick);
        item.enabled = enabled;
        return item;
    };

    std::vector<TreeMenuItem> items;
    items.push_back(makeItem("Rename", [this, id]() { BeginRename(id); }));
    items.push_back(makeItem("Duplicate", []() {}));
    items.push_back(makeItem("Delete", []() {}));
    items.push_back(makeItem("Create Child Actor", []() {}));

    auto menu = std::make_shared<TreeContextMenu>(items, nullptr);
    if (auto* overlay = GetPopupHost()) {
        overlay->CloseAllPopups();
        overlay->ShowPopup(menu, position);
    }
}

void TreeView::HandleSelection(const std::string& id, bool shift, bool ctrl) {
    if (ctrl) {
        auto it = std::find(m_SelectedIds.begin(), m_SelectedIds.end(), id);
        if (it != m_SelectedIds.end()) {
            m_SelectedIds.erase(it);
        } else {
            m_SelectedIds.push_back(id);
        }
    } else if (shift && !m_SelectedIds.empty()) {
        const std::string anchor = m_SelectedIds.back();
        bool inRange = false;
        m_SelectedIds.clear();
        for (const auto& item : m_RenderList) {
            if (item.node->id == anchor || item.node->id == id) {
                inRange = !inRange;
                m_SelectedIds.push_back(item.node->id);
                if (item.node->id == anchor && item.node->id == id) {
                    break;
                }
                if (!inRange && (item.node->id == anchor || item.node->id == id)) {
                    break;
                }
                continue;
            }
            if (inRange) {
                m_SelectedIds.push_back(item.node->id);
            }
        }
        if (m_SelectedIds.empty()) {
            m_SelectedIds.push_back(id);
        }
    } else {
        m_SelectedIds = { id };
    }

    if (m_OnSelectionChanged) {
        m_OnSelectionChanged(m_SelectedIds);
    }

    ScrollSelectionIntoView();
}

bool TreeView::IsSelected(const std::string& id) const {
    return std::find(m_SelectedIds.begin(), m_SelectedIds.end(), id) != m_SelectedIds.end();
}

int TreeView::GetVisibleRowCount() const {
    return static_cast<int>(std::ceil(m_Geometry.height / m_ItemHeight));
}

void TreeView::SetSearchQuery(const std::string& query) {
    m_SearchQuery = query;
    MarkRenderListDirty();
    BuildRenderList();
    Arrange(m_Geometry);
}

bool TreeView::ShowsPointerCursor(const Point& position) const {
    return ScrollViewport::ShowsScrollbarCursor(m_ScrollMetrics, position);
}

} // namespace we::editor::contentbrowser
