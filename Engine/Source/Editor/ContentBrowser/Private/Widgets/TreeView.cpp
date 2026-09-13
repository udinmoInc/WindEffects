// ==============================================================================
// WindEffects — ContentBrowser — TreeView
// UI widget used by the ContentBrowser module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "Platform/Platform.h"
#include "Core/Logger.h"
#include "Core/DiagnosticMacros.h"
#include "ContentBrowser/Widgets/TreeView.h"
#include <KindUI/EditorUI.h>
#include "Services/ContentBrowserFolderArt.h"
#include "Services/ContentBrowserBlueprintArt.h"
#include "KindUI/Profiling/ScreenRecorder.h"
#include "KindUI/Profiling/UiGeometryDebug.h"
#include "Text/Layout/TextStyle.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <functional>
#include <string_view>

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
namespace LayoutMetrics = ::we::runtime::kindui::LayoutMetrics;
namespace PanelChrome = ::we::runtime::kindui::panels::PanelChrome;
namespace ControlChrome = ::we::runtime::kindui::ControlChrome;
using ::we::runtime::kindui::DPIContext;

namespace {

using ::we::editor::contentbrowser::ContentBrowserBlueprintArt;
using ::we::editor::contentbrowser::ContentBrowserFolderArt;

float TreeExplorerPrefix(float uiScale) {
    return 58.0f * uiScale;
}

float TreeExpanderHit(float uiScale) {
    return 18.0f * uiScale;
}

float TreeAccessoryColumnX(float viewportX, int column, float uiScale) {
    if (column == 0) {
        return viewportX;
    }
    return viewportX + std::floor(30.0f * uiScale);
}

constexpr float kMinTreeZoom = 0.75f;
constexpr float kMaxTreeZoom = 1.75f;
constexpr float kTreeZoomStep = 0.1f;

float TreeUiScale() {
    return (std::max)(1.0f, DPIContext::GetScale());
}

void PaintTreeNodeIcon(PaintContext& context, const TreeNode& node, const Rect& iconRect, bool hovered) {
    if (node.iconTexture != we::rhi::RHIDescriptorSetHandle::Invalid) {
        context.DrawTexture(iconRect, node.iconTexture);
        return;
    }
    if (!node.icon.IsValid()) {
        return;
    }

    const std::string_view stem = node.icon.stem ? node.icon.stem : "";
    const bool isFolder =
        stem == "folder"
        || stem == "folder-open"
        || stem == "folder-mask"
        || stem == "folder-open-mask"
        || stem == "content-folder"
        || (WindIcons::Folder16.stem != nullptr && stem == WindIcons::Folder16.stem)
        || (WindIcons::FolderOpen16.stem != nullptr && stem == WindIcons::FolderOpen16.stem)
        || (WindIcons::FolderMask16.stem != nullptr && stem == WindIcons::FolderMask16.stem)
        || (WindIcons::FolderOpenMask16.stem != nullptr && stem == WindIcons::FolderOpenMask16.stem);
    if (isFolder) {
        const bool opened =
            node.expanded
            || stem == "folder-open"
            || stem == "folder-open-mask"
            || (WindIcons::FolderOpen16.stem != nullptr && stem == WindIcons::FolderOpen16.stem)
            || (WindIcons::FolderOpenMask16.stem != nullptr && stem == WindIcons::FolderOpenMask16.stem);
        ContentBrowserFolderArt::Get().PaintSmallIcon(context, iconRect, hovered, opened);
        return;
    }

    const Color iconColor = we::runtime::kindui::ResolveColor(ColorToken::IconSecondary);
    IconPainter::Draw(context, node.icon, iconRect, iconColor);
}

}

TreeView::TreeView()
    : m_Style(WidgetStyle::TreeItem())
{
    m_BaseItemHeight = we::runtime::kindui::LayoutMetrics::UnifiedListItemHeight();
    m_BaseIndentWidth = we::runtime::kindui::ResolveMetric(MetricToken::TreeIndentWidth);
    m_ItemHeight = m_BaseItemHeight;
    m_IndentWidth = m_BaseIndentWidth;
    m_Root = std::make_shared<TreeNode>();
    m_Root->id = "root";
    m_Root->label = "Root";
}

void TreeView::SyncScrollMetrics() {
    const float uiScale = TreeUiScale();
    const float headerHeight = (m_ExplorerStyle && m_ShowColumnHeader)
        ? PanelChrome::ColumnHeaderRowHeight()
        : 0.0f;
    const float topPad = std::floor(4.0f * uiScale);
    const float bottomPad = std::floor(6.0f * uiScale);
    Rect viewportGeom = m_Geometry;
    viewportGeom.y += (headerHeight + topPad);
    viewportGeom.height = (std::max)(0.0f, viewportGeom.height - headerHeight - topPad - bottomPad);
    m_ScrollMetrics = m_Scroll.UpdateMetrics(viewportGeom, viewportGeom.height, m_ContentHeight, uiScale);
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

    for (size_t i = 0; i < m_RenderList.size(); ++i) {
        auto& item = m_RenderList[i];
        item.flatIndex = static_cast<int>(i);
        const float itemY = m_ScrollMetrics.viewport.y - m_Scroll.offset + static_cast<float>(i) * rowHeight;
        item.geometry = Rect{
            viewportX + indentOffset + item.depth * m_IndentWidth,
            itemY,
            std::max(0.0f, viewportWidth - indentOffset - item.depth * m_IndentWidth),
            rowHeight
        };
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
        const float colWidth = std::floor(30.0f * uiScale);
        const float iconSize = 16.0f * uiScale;
        layout.eyeBounds = Rect{
            viewportX + (colWidth - iconSize) * 0.5f,
            centerY - iconSize * 0.5f,
            iconSize,
            iconSize };
    }

    const float contentPad = m_ExplorerStyle ? 0.0f : ThemeMetric(MetricToken::Space2) * uiScale;
    layout.indentX = viewportX + prefixOffset + contentPad + (static_cast<float>(item.depth) * m_IndentWidth);

    const float expanderWidth = TreeExpanderHit(uiScale);
    layout.expanderBounds = Rect{ layout.indentX, item.geometry.y, expanderWidth, rowHeight };
    layout.hasExpander = !item.node->children.empty();

    const float contentSlotX = layout.indentX + expanderWidth + 4.0f * uiScale;

    const float iconSize = ThemeMetric(MetricToken::IconSizeTree);
    const bool hasIcon = item.node->icon.IsValid() || item.node->iconTexture !=
        we::rhi::RHIDescriptorSetHandle::Invalid;
    layout.hasIcon = hasIcon;
    if (hasIcon) {
        layout.iconBounds = Rect{ contentSlotX, centerY - iconSize * 0.5f, iconSize, iconSize };
        layout.textX = contentSlotX + iconSize + 6.0f * uiScale;
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
    if (!m_Visible) return;
    const float uiScale = TreeUiScale();
    const float fontSize = m_Style.text.size * uiScale;

    if (m_ExplorerStyle && m_ShowColumnHeader) {
        const float headerHeight = PanelChrome::ColumnHeaderRowHeight();
        const Rect headerRect{ m_Geometry.x, m_Geometry.y, m_Geometry.width, headerHeight };

        PanelChrome::PaintHeaderRegion(context, headerRect);
        PanelChrome::PaintExplorerColumnHeader(context, headerRect, "Item Label ▲");
    }

    SyncScrollMetrics();
    UpdateVisibleRange();

    if (m_PaintNavigationBackground && m_ScrollMetrics.viewport.width > 0.0f && m_ScrollMetrics.viewport.height >
        0.0f) {
        PanelChrome::PaintNavigationRegion(context, m_ScrollMetrics.viewport);
    }

    const float viewTop = m_ScrollMetrics.viewport.y;
    const float viewBottom = m_ScrollMetrics.viewport.y + m_ScrollMetrics.viewport.height;

    context.PushClipRect(m_ScrollMetrics.viewport);

    const int firstVisible = std::max(0, m_FirstVisibleIndex);
    const int lastVisible = std::min(static_cast<int>(m_RenderList.size()) - 1, m_LastVisibleIndex);

    for (int i = firstVisible; i <= lastVisible && i < static_cast<int>(m_RenderList.size()); ++i) {
        const auto& item = m_RenderList[static_cast<size_t>(i)];
        const auto& node = item.node;
        if (!node) {
            continue;
        }
        const float rowHeight = item.geometry.height;

        if (item.geometry.y + item.geometry.height < viewTop ||
            item.geometry.y > viewBottom) {
            continue;
        }

        const TreeRowLayoutSlots layout = ComputeTreeRowLayout(item);
        const bool selected = IsSelected(node->id);
        const bool hovered = node->id == m_HoveredId;

        if (m_ShowRowHighlight && (selected || hovered)) {
            PanelChrome::PaintListRowBackground(
                context, layout.rowBounds, hovered, selected, IsFocused());
        } else if (m_ShowAlternatingRowBackground) {
            PanelChrome::PaintAlternatingListRowBackground(
                context, layout.rowBounds, item.flatIndex);
        }

        if (m_Dragging && !m_DropTargetId.empty() && node->id == m_DropTargetId && m_DropTargetId != m_DragSourceId) {
            Rect dropLine{
                m_ScrollMetrics.viewport.x + 2.0f,
                layout.rowBounds.y,
                m_ScrollMetrics.viewport.width - 4.0f,
                2.0f
            };
            context.DrawRect(dropLine, ThemeColor(ColorToken::AccentPrimary));
        }

        if (m_ExplorerStyle) {
            if (hovered || selected || !node->visible) {
                const Color eyeColor = node->visible ? ThemeColor(ColorToken::TextSecondary) :
                    ThemeColor(ColorToken::TextDisabled);
                const WindIconRef eyeIcon = node->visible ? WindIcons::Eye16 : WindIcons::EyeOff16;
                IconPainter::Draw(context, eyeIcon, IconMetrics::PlaceGlyphCentered(layout.eyeBounds, 16u), eyeColor);
            }
        }

        if (layout.hasExpander) {
            const WindIconRef triangleIcon = node->expanded ? WindIcons::TriangleDown16 : WindIcons::TriangleRight16;
            const Color expanderColor = hovered ? ThemeColor(ColorToken::TextPrimary) :
                ThemeColor(ColorToken::TextSecondary);
            IconPainter::Draw(context, triangleIcon, IconMetrics::PlaceGlyphCentered(layout.expanderBounds, 16u),
                expanderColor);
        }

        if (layout.hasIcon) {
            Rect iconRect = IconMetrics::PlaceGlyphCentered(layout.iconBounds, layout.iconBounds.width);
            PaintTreeNodeIcon(context, *node, iconRect, hovered);
        }

        const float textY = LayoutMetrics::AlignTextTopY(layout.rowBounds, fontSize);
        Color textColor = ThemeColor(ColorToken::TextSecondary);
        if (!node->visible) {
            textColor = ThemeColor(ColorToken::TextDisabled);
        }

        if (node->id == m_RenamingId) {
            Rect editBg{ layout.textX - 4.0f, layout.rowBounds.y + 2.0f, (std::max)(40.0f, layout.maxTextWidth),
                rowHeight - 4.0f };
            context.DrawRoundedRect(editBg, ThemeColor(ColorToken::InputBackground), 3.0f);
            context.DrawRoundedRectOutline(editBg, ThemeColor(ColorToken::AccentPrimary), 1.0f, 3.0f);
            context.DrawText(m_RenameBuffer, Point{ layout.textX, textY }, ThemeColor(ColorToken::TextSecondary),
                fontSize);
            if (static_cast<int>(m_RenameCursorBlink * 2.0f) % 2 == 0) {
                const float cursorX = layout.textX + context.GetTextWidth(m_RenameBuffer, fontSize) + 1.0f;
                context.DrawRect(Rect{ cursorX, textY, 1.0f, fontSize }, ThemeColor(ColorToken::TextSecondary));
            }
        } else {
            if (!m_SearchQuery.empty()) {
                const std::string& label = node->label;
                const std::string& query = m_SearchQuery;
                const std::string_view labelView = label;
                size_t matchStart = 0;
                size_t matchEnd = 0;
                bool foundMatch = false;

                if (query.size() <= label.size()) {
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
                }

                if (foundMatch) {
                    const std::string_view beforeMatch = labelView.substr(0, matchStart);
                    float currentX = layout.textX;
                    if (!beforeMatch.empty()) {
                        context.DrawText(beforeMatch, Point{ currentX, textY }, textColor, fontSize);
                        currentX += context.GetTextWidth(beforeMatch, fontSize);
                    }
                    const std::string_view matchText = labelView.substr(matchStart, matchEnd - matchStart);
                    const float matchWidth = context.GetTextWidth(matchText, fontSize);
                    Rect highlightRect{ currentX, textY, matchWidth, fontSize };
                    context.DrawRoundedRect(highlightRect, ThemeColor(ColorToken::SelectionHighlight), 2.0f);
                    context.DrawText(matchText, Point{ currentX, textY }, ThemeColor(ColorToken::AccentPrimary),
                        fontSize);
                    currentX += matchWidth;
                    const std::string_view afterMatch = labelView.substr(matchEnd);
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

        if (!node->typeName.empty()) {
            const float typeFontSize = fontSize * 0.9f;
            const float typeWidth = context.GetTextWidth(node->typeName, typeFontSize);
            const float typeColumnReserve = we::runtime::kindui::ResolveMetric(MetricToken::Space6) * uiScale;
            const float typeRightX = m_ScrollMetrics.viewport.x + m_ScrollMetrics.viewport.width - typeColumnReserve;
            const float typeY = LayoutMetrics::AlignTextTopY(layout.rowBounds, typeFontSize);
            context.DrawText(node->typeName, Point{ typeRightX - typeWidth, typeY },
                ThemeColor(ColorToken::TextSecondary), typeFontSize);
        }

        if (m_ShowRowControls && !m_ExplorerStyle) {
            (void)(node->visible ? ThemeColor(ColorToken::TextSecondary) : ThemeColor(ColorToken::TextDisabled));
            const WindIconRef eyeIcon = node->visible ? WindIcons::Eye16 : kWindIconNone;
            IconPainter::Draw(context, eyeIcon, IconMetrics::PlaceGlyphCentered(layout.eyeBounds, 16u));

            if (node->locked) {
                IconPainter::Draw(context, WindIcons::Lock16, IconMetrics::PlaceGlyphCentered(layout.lockBounds, 16u));
            }
        }
    }

    if (m_ShowAlternatingRowBackground) {
        const float fillerRowHeight = m_ItemHeight * uiScale;
        if (fillerRowHeight > 0.0f) {
            const float contentBottom = m_ScrollMetrics.viewport.y - m_Scroll.offset
                + static_cast<float>(m_RenderList.size()) * fillerRowHeight;
            const float fillBottom = m_ScrollMetrics.viewport.y + m_ScrollMetrics.viewport.height;
            int fillerIndex = static_cast<int>(m_RenderList.size());
            for (float fillerY = contentBottom; fillerY < fillBottom; fillerY += fillerRowHeight, ++fillerIndex) {
                if ((fillerIndex % 2) == 0) {
                    continue;
                }
                const Rect fillerRow{
                    m_ScrollMetrics.viewport.x,
                    fillerY,
                    m_ScrollMetrics.viewport.width,
                    (std::min)(fillerRowHeight, fillBottom - fillerY)
                };
                Color stripe = ThemeColor(ColorToken::PanelBackground);
                stripe.a *= 0.3f;
                context.DrawRect(fillerRow, stripe);
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
            we::runtime::kindui::ResolveMetric(MetricToken::TreeIndentWidth),
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
        ::we::runtime::kindui::ScreenRecorder::Get().RecordEvent(
            "tree-drop " + m_DragSourceId + " -> " + m_DropTargetId);
    }
    m_Dragging = false;
    m_DropTargetId.clear();
    m_DragSourceId.clear();

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
    }

    lastClickedId = item->node->id;
    lastClickTime = now;
}

void TreeView::OnHoverLost() {
    m_HoveredId.clear();
    m_Dragging = false;
    m_DropTargetId.clear();
    m_DragSourceId.clear();
}

void TreeView::OnMouseMove(const MouseEvent& event) {
    SyncScrollMetrics();
    m_Scroll.OnMouseMove(event, m_ScrollMetrics, m_Geometry.height, m_ContentHeight);
    if (m_Scroll.IsDraggingThumb()) {
        Arrange(m_Geometry);
        return;
    }

    RenderItem* item = GetItemAtPosition(event.position);
    const std::string newHoveredId = item ? item->node->id : "";
    if (m_HoveredId != newHoveredId) {
        m_HoveredId = newHoveredId;
        InvalidatePaint();
    }

    if (!m_DragSourceId.empty()) {
        const float dx = event.position.x - m_DragStart.x;
        const float dy = event.position.y - m_DragStart.y;
        if (!m_Dragging && (dx * dx + dy * dy) > 25.0f) {
            m_Dragging = true;
        }
        if (m_Dragging) {
            const std::string newDropTargetId = item ? item->node->id : "";
            if (m_DropTargetId != newDropTargetId) {
                m_DropTargetId = newDropTargetId;
                InvalidatePaint();
            }
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
    m_Style.text.size = std::clamp(
        ThemeMetric(MetricToken::TextSizeSmall) * m_ZoomLevel,
        10.0f,
        20.0f);
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

    if (we::runtime::kindui::HotkeyManager::Get().Dispatch(event, "ContentBrowser")) {
        return;
    }

    if (event.ctrlDown && event.key == we::platform::KeyCode::A) {
        m_SelectedIds.clear();
        for (const auto& item : m_RenderList) {
            if (item.node && item.node->id != "root") {
                m_SelectedIds.push_back(item.node->id);
            }
        }
        SyncSelectedSet();
        if (m_OnSelectionChanged) {
            m_OnSelectionChanged(m_SelectedIds);
        }
        InvalidatePaint();
        return;
    }

    if (event.key == we::platform::KeyCode::Delete && !m_SelectedIds.empty()) {
        auto selected = m_SelectedIds;
        for (const auto& id : selected) {
            RemoveItem(id);
        }
        m_SelectedIds.clear();
        SyncSelectedSet();
        if (m_OnSelectionChanged) {
            m_OnSelectionChanged(m_SelectedIds);
        }
        InvalidatePaint();
        return;
    }

    if (event.key == we::platform::KeyCode::Escape && !m_SelectedIds.empty()) {
        m_SelectedIds.clear();
        SyncSelectedSet();
        if (m_OnSelectionChanged) {
            m_OnSelectionChanged(m_SelectedIds);
        }
        InvalidatePaint();
        return;
    }

    if (event.key == we::platform::KeyCode::F2 && !m_SelectedIds.empty()) {
        BeginRename(m_SelectedIds.back());
        return;
    }
}

void TreeView::SetRoot(const std::shared_ptr<TreeNode>& root) {
    m_Root = root;
    m_SelectedIds.clear();
    m_SelectedSet.clear();
    MarkRenderListDirty();
    BuildRenderList();
    InvalidateLayout();
    InvalidatePaint();
}

void TreeView::AddItem(const std::shared_ptr<TreeNode>& item, const std::string& parentId) {
    if (!item) return;
    if (parentId.empty()) {
        if (m_Root) m_Root->children.push_back(item);
    } else if (auto parent = FindNode(parentId)) {
        parent->children.push_back(item);
    }
    MarkRenderListDirty();
    BuildRenderList();
    InvalidateLayout();
    InvalidatePaint();
}

void TreeView::RemoveItem(const std::string& id) {
    if (!m_Root) return;
    std::function<bool(std::vector<std::shared_ptr<TreeNode>>&)> removeRecursive =
        [&](std::vector<std::shared_ptr<TreeNode>>& nodes) -> bool {
            for (auto it = nodes.begin(); it != nodes.end(); ++it) {
                if (!*it) continue;
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
    InvalidateLayout();
    InvalidatePaint();
}

void TreeView::SyncSelectedSet() {
    m_SelectedSet.clear();
    m_SelectedSet.insert(m_SelectedIds.begin(), m_SelectedIds.end());
}

void TreeView::Clear() {
    if (m_Root) m_Root->children.clear();
    m_SelectedIds.clear();
    m_SelectedSet.clear();
    MarkRenderListDirty();
    BuildRenderList();
    InvalidateLayout();
    InvalidatePaint();
}

void TreeView::SetSelectedId(const std::string& id) {
    m_SelectedIds = id.empty() ? std::vector<std::string>{} : std::vector<std::string>{ id };
    SyncSelectedSet();
    InvalidatePaint();
}

void TreeView::SetSelectedIds(const std::vector<std::string>& ids) {
    m_SelectedIds = ids;
    SyncSelectedSet();
    InvalidatePaint();
}

std::string TreeView::GetSelectedId() const {
    return m_SelectedIds.empty() ? std::string{} : m_SelectedIds.back();
}

void TreeView::BuildRenderList() {
    if (!m_RenderListDirty && !m_RenderList.empty()) {
        return;
    }

    m_RenderList.clear();
    if (!m_Root) {
        m_ContentHeight = 0.0f;
        m_RenderListDirty = false;
        m_FirstVisibleIndex = 0;
        m_LastVisibleIndex = -1;
        return;
    }

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

    auto matchesFilter = [this, fuzzyMatch](const std::shared_ptr<TreeNode>& node) -> bool {
        if (!node) return false;
        if (!m_SearchQuery.empty() && !fuzzyMatch(node->label, m_SearchQuery)) {
            return false;
        }

        if (!m_FilterOptions.showHidden && !node->visible) {
            return false;
        }

        // Locked items filter
        if (!m_FilterOptions.showLocked && node->locked) {
            return false;
        }

        // Empty folders filter - always show empty folders
        if (node->children.empty() && !node->icon.IsValid()) {
            return true;
        }

        return true;
    };

    std::function<void(const std::shared_ptr<TreeNode>&, int, bool)> buildRecursive =
        [&](const std::shared_ptr<TreeNode>& node, int depth, bool parentMatches) {
            if (!node) return;
            if (node->id != "root") {
                const bool nodeMatches = matchesFilter(node);
                const bool shouldShow = nodeMatches || parentMatches;

                if (shouldShow) {
                    m_RenderList.push_back({ node, depth, 0, Rect{} });
                }

                // Always expand children if searching or if parent matches
                if (nodeMatches || parentMatches || !m_SearchQuery.empty()) {
                    for (const auto& child : node->children) {
                        if (child) buildRecursive(child, depth + 1, shouldShow);
                    }
                } else if (node->expanded) {
                    for (const auto& child : node->children) {
                        if (child) buildRecursive(child, depth + 1, false);
                    }
                }
            } else {
                // Root node - process children
                for (const auto& child : node->children) {
                    if (child) buildRecursive(child, 0, false);
                }
            }
        };

    const auto startTime = std::chrono::high_resolution_clock::now();
    buildRecursive(m_Root, 0, false);

    m_ContentHeight = static_cast<float>(m_RenderList.size()) * m_ItemHeight * TreeUiScale();
    m_RenderListDirty = false;
    m_Scroll.Sync(m_Geometry.height, m_ContentHeight);
    UpdateVisibleRange();

    const auto endTime = std::chrono::high_resolution_clock::now();
    const double durationMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();
    WE_LOG_INFO(we::LogCategory::General.data(),
        "[TreeViewDebug] BuildRenderList: items=" + std::to_string(m_RenderList.size()) +
        " contentHeight=" + std::to_string(m_ContentHeight) +
        " duration=" + std::to_string(durationMs) + "ms");
}

void TreeView::UpdateVisibleRange() {
    if (m_RenderList.empty()) {
        m_FirstVisibleIndex = 0;
        m_LastVisibleIndex = -1;
        return;
    }

    const float rowHeight = m_ItemHeight * TreeUiScale();
    if (rowHeight <= 0.0f) {
        m_FirstVisibleIndex = 0;
        m_LastVisibleIndex = 0;
        return;
    }

    const int overscan = 2;
    const int rawFirst = static_cast<int>(std::floor(m_Scroll.offset / rowHeight));
    m_FirstVisibleIndex = std::clamp(rawFirst - overscan, 0, std::max(0, static_cast<int>(m_RenderList.size()) - 1));

    const int visibleCount = static_cast<int>(std::ceil(m_Geometry.height / rowHeight)) + overscan * 2;
    m_LastVisibleIndex = std::clamp(m_FirstVisibleIndex + visibleCount, 0, static_cast<int>(m_RenderList.size()) - 1);
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
                if (item.node && item.geometry.Contains(pos)) {
                    return &item;
                }
            }
        }
    }

    const int slowFirst = std::max(0, m_FirstVisibleIndex - 2);
    const int slowLast = std::min(static_cast<int>(m_RenderList.size()) - 1, m_LastVisibleIndex + 2);
    for (int i = slowFirst; i <= slowLast && i < static_cast<int>(m_RenderList.size()); ++i) {
        auto& item = m_RenderList[static_cast<size_t>(i)];
        if (item.node && item.geometry.Contains(pos)) {
            return &item;
        }
    }
    return nullptr;
}

std::shared_ptr<TreeNode> TreeView::FindNode(const std::string& id) {
    if (!m_Root) return nullptr;

    std::function<std::shared_ptr<TreeNode>(const std::shared_ptr<TreeNode>&)> findRecursive =
        [&](const std::shared_ptr<TreeNode>& node) -> std::shared_ptr<TreeNode> {
            if (!node) return nullptr;
            if (node->id == id) {
                return node;
            }
            for (const auto& child : node->children) {
                if (child) {
                    if (auto found = findRecursive(child)) {
                        return found;
                    }
                }
            }
            return nullptr;
        };

    if (m_Root->id == id) {
        return m_Root;
    }
    for (const auto& child : m_Root->children) {
        if (child) {
            if (auto found = findRecursive(child)) {
                return found;
            }
        }
    }
    return nullptr;
}

void TreeView::ToggleExpand(const std::string& id) {
    const auto startTime = std::chrono::high_resolution_clock::now();
    bool newState = false;
    if (auto node = FindNode(id)) {
        node->expanded = !node->expanded;
        newState = node->expanded;
        if (node->icon.IsValid()) {
            const std::string_view stem = node->icon.stem ? node->icon.stem : "";
            const bool isFolderGlyph =
                stem == "folder"
                || stem == "folder-open"
                || stem == "folder-mask"
                || stem == "folder-open-mask"
                || (WindIcons::Folder16.stem != nullptr && stem == WindIcons::Folder16.stem)
                || (WindIcons::FolderOpen16.stem != nullptr && stem == WindIcons::FolderOpen16.stem)
                || (WindIcons::FolderMask16.stem != nullptr && stem == WindIcons::FolderMask16.stem)
                || (WindIcons::FolderOpenMask16.stem != nullptr && stem == WindIcons::FolderOpenMask16.stem);
            if (isFolderGlyph) {
                node->icon = node->expanded ? WindIcons::FolderOpenMask16 : WindIcons::FolderMask16;
            }
        }
    }
    MarkRenderListDirty();
    BuildRenderList();
    Arrange(m_Geometry);
    const auto endTime = std::chrono::high_resolution_clock::now();
    const double durationMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();

    WE_LOG_INFO(we::LogCategory::General.data(),
        "[TreeViewDebug] ToggleExpand: node='" + id + "' expanded=" + (newState ? "true" : "false") +
        " totalItems=" + std::to_string(m_RenderList.size()) +
        " visibleRange=[" + std::to_string(m_FirstVisibleIndex) + ".." + std::to_string(m_LastVisibleIndex) + "]" +
        " duration=" + std::to_string(durationMs) + "ms");
}

void TreeView::ExpandAll() {
    if (!m_Root) {
        return;
    }
    we::runtime::kindui::Expansion::ScopedTransaction transaction("ExpandAll");
    std::vector<std::shared_ptr<TreeNode>> roots{ m_Root };
    we::runtime::kindui::Expansion::ExpandAll(
        roots,
        [](const std::shared_ptr<TreeNode>& node) -> const std::vector<std::shared_ptr<TreeNode>>& {
            return node->children;
        },
        [](const std::shared_ptr<TreeNode>& node, bool expanded) {
            node->expanded = expanded;
        });
    MarkRenderListDirty();
    BuildRenderList();
    InvalidateLayout();
    InvalidatePaint();
}

void TreeView::CollapseAll() {
    if (!m_Root) {
        return;
    }
    we::runtime::kindui::Expansion::ScopedTransaction transaction("CollapseAll");
    std::vector<std::shared_ptr<TreeNode>> roots{ m_Root };
    we::runtime::kindui::Expansion::CollapseAll(
        roots,
        [](const std::shared_ptr<TreeNode>& node) -> const std::vector<std::shared_ptr<TreeNode>>& {
            return node->children;
        },
        [](const std::shared_ptr<TreeNode>& node, bool expanded) {
            // Keep the true root expanded so the tree remains usable.
            if (node->id == "root" || node->id == "/Game") {
                node->expanded = true;
                return;
            }
            node->expanded = expanded;
        });
    MarkRenderListDirty();
    BuildRenderList();
    InvalidateLayout();
    InvalidatePaint();
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
    std::vector<std::shared_ptr<::we::runtime::kindui::MenuItem>> menuItems;
    menuItems.push_back([](const std::string& lbl, std::function<void()> fn) {
        auto mi = std::make_shared<::we::runtime::kindui::MenuItem>();
        mi->label = lbl;
        mi->onClick = std::move(fn);
        return mi;
    }("Rename", [this, id]() { BeginRename(id); }));

    menuItems.push_back([](const std::string& lbl, std::function<void()> fn) {
        auto mi = std::make_shared<::we::runtime::kindui::MenuItem>();
        mi->label = lbl;
        mi->onClick = std::move(fn);
        return mi;
    }("Duplicate", []() {}));

    menuItems.push_back([](const std::string& lbl, std::function<void()> fn) {
        auto mi = std::make_shared<::we::runtime::kindui::MenuItem>();
        mi->label = lbl;
        mi->onClick = std::move(fn);
        return mi;
    }("Delete", []() {}));

    menuItems.push_back([](const std::string& lbl, std::function<void()> fn) {
        auto mi = std::make_shared<::we::runtime::kindui::MenuItem>();
        mi->label = lbl;
        mi->onClick = std::move(fn);
        return mi;
    }("Create Child Actor", []() {}));

    auto menu = std::make_shared<::we::runtime::kindui::DropdownMenu>(menuItems);
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

    SyncSelectedSet();

    if (m_OnSelectionChanged) {
        m_OnSelectionChanged(m_SelectedIds);
    }

    ScrollSelectionIntoView();
    InvalidatePaint();
}

bool TreeView::IsSelected(const std::string& id) const {
    return m_SelectedSet.count(id) > 0;
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

}
