#include "Widgets/ContentBrowser.h"
#include "Layout/ScrollViewport.h"
#include "Controllers/FilterController.h"
#include "Controllers/SearchController.h"
#include "Services/ContentBrowserService.h"
#include "Services/ContentBrowserFolderArt.h"
#include "Services/ContentBrowserBlueprintArt.h"
#include "WindEffects/Editor/UI/Panel/PanelChrome.h"
#include "Core/PaintContext.h"
#include "Core/DPIContext.h"
#include "Rendering/IconMetrics.h"
#include "WindEffects/Editor/UI/Theming/ThemeToken.h"
#include "Core/Icon.h"
#include <SDL3/SDL.h>
#include <algorithm>
#include <cctype>

namespace WindEffects::Editor::UI {

namespace {
using we::editor::contentbrowser::ContentBrowserService;
using we::editor::contentbrowser::FilterController;
using we::editor::contentbrowser::SearchController;
using we::editor::contentbrowser::ContentBrowserFolderArt;
using we::editor::contentbrowser::ContentBrowserBlueprintArt;

bool IsBlueprintItem(const ContentItem& item) {
    return item.type == "Blueprint";
}
}

ContentBrowser::ContentBrowser()
    : m_Style(WidgetStyle::Panel())
{
    m_Model = std::make_shared<ContentBrowserModel>();
    m_Controller = std::make_shared<ContentBrowserController>(m_Model);
}

void ContentBrowser::SetModel(std::shared_ptr<ContentBrowserModel> model) {
    m_Model = model;
    if (m_Model) {
        m_Model->onModelChanged = [this]() { BuildRenderList(); };
        BuildRenderList();
    }
}

ContentViewMode ContentBrowser::GetEffectiveViewMode() const {
    if (!m_Model) return ContentViewMode::LargeIcons;
    return m_Model->viewMode;
}

ContentBrowser::GridMetrics ContentBrowser::GetGridMetrics() const {
    GridMetrics m;
    m.padding = 8.0f;
    m.hSpacing = 6.0f;
    m.vSpacing = 6.0f;
    m.labelLineHeight = 13.0f;
    m.labelGap = 5.0f;

    switch (GetEffectiveViewMode()) {
        case ContentViewMode::LargeIcons:
            m.thumbSize = 104.0f;
            m.cellWidth = 112.0f;
            m.labelLines = 2.0f;
            m.cellHeight = m.thumbSize + m.labelGap + m.labelLineHeight * m.labelLines;
            break;
        case ContentViewMode::MediumIcons:
            m.thumbSize = 72.0f;
            m.cellWidth = 80.0f;
            m.labelLines = 2.0f;
            m.cellHeight = m.thumbSize + m.labelGap + m.labelLineHeight * m.labelLines;
            break;
        case ContentViewMode::SmallIcons:
            m.thumbSize = 48.0f;
            m.cellWidth = 56.0f;
            m.labelLines = 1.0f;
            m.cellHeight = m.thumbSize + m.labelGap + m.labelLineHeight;
            break;
        case ContentViewMode::Tiles:
            m.thumbSize = 104.0f;
            m.cellWidth = 112.0f;
            m.labelLines = 2.0f;
            m.cellHeight = m.thumbSize + m.labelGap + m.labelLineHeight * 2.0f + 14.0f;
            break;
        default:
            break;
    }
    return m;
}

Size ContentBrowser::Measure(const Size& availableSize) {
    m_DesiredSize = availableSize;
    RecalculateLayout();
    return m_DesiredSize;
}

void ContentBrowser::SyncScrollMetrics() {
    m_Scroll.Sync(m_Geometry.height, m_ContentHeight);
    const float uiScale = std::max(1.0f, DPIContext::GetScale());
    m_ScrollMetrics = m_Scroll.ComputeMetrics(m_Geometry, m_ContentHeight, uiScale);
}

float ContentBrowser::ComputeContentHeight() const {
    if (m_RenderList.empty()) {
        return 0.0f;
    }

    float maxBottom = 0.0f;
    for (const auto& item : m_RenderList) {
        maxBottom = std::max(maxBottom, item.geometry.y + item.geometry.height);
    }
    const float originY = m_ScrollMetrics.viewport.y > 0.0f
        ? m_ScrollMetrics.viewport.y - m_Scroll.offset
        : m_Geometry.y - m_Scroll.offset;
    return std::max(0.0f, maxBottom - originY) + 16.0f;
}

void ContentBrowser::RecalculateLayout() {
    BuildRenderList();

    const auto layoutPass = [&]() {
        switch (GetEffectiveViewMode()) {
            case ContentViewMode::List: CalculateListLayout(); break;
            case ContentViewMode::Details: CalculateDetailsLayout(); break;
            case ContentViewMode::Tiles: CalculateTilesLayout(); break;
            default: CalculateGridLayout(); break;
        }
    };

    m_ScrollMetrics.viewport = m_Geometry;
    layoutPass();
    m_ContentHeight = ComputeContentHeight();
    SyncScrollMetrics();
    layoutPass();
    m_ContentHeight = ComputeContentHeight();
    SyncScrollMetrics();
}

void ContentBrowser::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
    RecalculateLayout();
    UpdateVisibleRange();
    RequestVisibleThumbnails();
}

void ContentBrowser::Tick(float deltaTime) {
    Widget::Tick(deltaTime);
    ContentBrowserService::Get().Tick(deltaTime);
    RequestVisibleThumbnails();

    constexpr float kHoverDuration = 0.135f;
    const float hoverSpeed = 1.0f / kHoverDuration;
    const float target = m_HoveredId.empty() ? 0.0f : 1.0f;
    if (m_ItemHoverAlpha < target) {
        m_ItemHoverAlpha = std::min(target, m_ItemHoverAlpha + hoverSpeed * deltaTime);
    } else if (m_ItemHoverAlpha > target) {
        m_ItemHoverAlpha = std::max(target, m_ItemHoverAlpha - hoverSpeed * deltaTime);
    }
}

void ContentBrowser::UpdateVisibleRange() {
    m_FirstVisibleIndex = 0;
    m_LastVisibleIndex = static_cast<int>(m_RenderList.size()) - 1;
    if (m_RenderList.empty()) return;

    const float viewTop = m_Geometry.y;
    const float viewBottom = m_Geometry.y + m_Geometry.height;

    m_FirstVisibleIndex = static_cast<int>(m_RenderList.size());
    m_LastVisibleIndex = -1;
    for (int i = 0; i < static_cast<int>(m_RenderList.size()); ++i) {
        const auto& geom = m_RenderList[static_cast<size_t>(i)].geometry;
        if (geom.y + geom.height < viewTop || geom.y > viewBottom) continue;
        m_FirstVisibleIndex = std::min(m_FirstVisibleIndex, i);
        m_LastVisibleIndex = std::max(m_LastVisibleIndex, i);
    }

    if (m_LastVisibleIndex < m_FirstVisibleIndex) {
        m_FirstVisibleIndex = 0;
        m_LastVisibleIndex = -1;
    }
}

void ContentBrowser::RequestVisibleThumbnails() {
    if (!m_OnItemNeedsThumbnail) return;

    std::unordered_set<std::string> visibleIds;
    for (int i = m_FirstVisibleIndex; i <= m_LastVisibleIndex && i < static_cast<int>(m_RenderList.size()); ++i) {
        const auto& item = m_RenderList[static_cast<size_t>(i)].item;
        visibleIds.insert(item.id);
        if (item.iconTexture == VK_NULL_HANDLE && !item.thumbnailRequested && !item.isFolder) {
            m_OnItemNeedsThumbnail(item.id);
            if (m_Model) {
                for (auto& orig : m_Model->items) {
                    if (orig.id == item.id) {
                        orig.thumbnailRequested = true;
                        break;
                    }
                }
            }
        }
    }

    if (m_OnVisibleItemsChanged) {
        m_OnVisibleItemsChanged(visibleIds);
    }
}

void ContentBrowser::PaintTileChrome(PaintContext& context, const Rect& cell, bool selected, float hoverAlpha) {
    constexpr float radius = 4.0f;

    // Removed selector box - only show hover effect
    if (hoverAlpha > 0.001f) {
        Color hoverBg = ThemeColor(ThemeToken::ContentBrowserHoverBackground);
        hoverBg.a *= hoverAlpha * 0.65f;
        context.DrawRoundedRect(cell, hoverBg, radius);
    }
}

void ContentBrowser::PaintAssetThumbnail(PaintContext& context, const Rect& thumbRect,
    const ContentItem& item, bool selected, bool hovered)
{
    (void)selected;
    (void)hovered;
    const Color accent = ThemeColor(ThemeToken::AccentPrimary);

    if (item.isFolder) {
        ContentBrowserFolderArt::Get().PaintThumbnail(context, thumbRect, hovered);
        return;
    }

    if (IsBlueprintItem(item)) {
        ContentBrowserBlueprintArt::Get().PaintThumbnail(context, thumbRect, hovered);
    } else if (item.iconTexture != VK_NULL_HANDLE) {
        context.DrawTexture(thumbRect, item.iconTexture);
    } else {
        const float iconSize = std::min(thumbRect.width, thumbRect.height) * 0.42f;
        Rect iconRect{
            thumbRect.x + (thumbRect.width - iconSize) * 0.5f,
            thumbRect.y + (thumbRect.height - iconSize) * 0.5f,
            iconSize, iconSize
        };
        IconPainter::DrawIcon(context, item.iconName, iconRect, ThemeColor(ThemeToken::IconDefault));
    }

    if (item.isFavorite) {
        Rect star{ thumbRect.x + thumbRect.width - 16.0f, thumbRect.y + 4.0f, 12.0f, 12.0f };
        IconPainter::DrawIcon(context, Icons::StarFilledName, star, accent);
    }
    if (item.isDirty) {
        Rect dot{ thumbRect.x + 4.0f, thumbRect.y + 4.0f, 6.0f, 6.0f };
        context.DrawRoundedRect(dot, ThemeColor(ThemeToken::Warning), 3.0f);
    }
}

std::vector<std::string> ContentBrowser::WrapLabelText(
    PaintContext& context, const std::string& text, float maxWidth, float fontSize, int maxLines) const
{
    std::vector<std::string> lines;
    if (text.empty() || maxLines <= 0) return lines;

    auto truncateLine = [&](const std::string& value) {
        if (context.GetTextWidth(value, fontSize) <= maxWidth) return value;
        std::string trimmed = value;
        while (trimmed.length() > 1 && context.GetTextWidth(trimmed + "...", fontSize) > maxWidth) {
            trimmed.pop_back();
        }
        return trimmed + "...";
    };

    if (maxLines == 1 || context.GetTextWidth(text, fontSize) <= maxWidth) {
        lines.push_back(truncateLine(text));
        return lines;
    }

    size_t breakAt = text.size();
    for (size_t i = 1; i <= text.size(); ++i) {
        if (context.GetTextWidth(text.substr(0, i), fontSize) > maxWidth) {
            breakAt = i > 1 ? i - 1 : 1;
            break;
        }
    }

    size_t split = breakAt;
    const size_t lastSpace = text.rfind(' ', breakAt > 0 ? breakAt - 1 : 0);
    if (lastSpace != std::string::npos && lastSpace > 0) {
        split = lastSpace;
    }

    std::string line1 = text.substr(0, split);
    while (!line1.empty() && line1.back() == ' ') line1.pop_back();
    std::string remainder = split < text.size() ? text.substr(split) : std::string{};
    while (!remainder.empty() && remainder.front() == ' ') remainder.erase(remainder.begin());

    if (line1.empty()) line1 = truncateLine(text);
    lines.push_back(line1);

    if (!remainder.empty()) {
        lines.push_back(truncateLine(remainder));
    }
    return lines;
}

void ContentBrowser::PaintItemLabel(PaintContext& context, const Rect& cell, const std::string& name, float maxWidth, int maxLines) {
    const GridMetrics metrics = GetGridMetrics();
    const float fontSize = ThemeMetric(ThemeToken::TextSizeNormal);
    const float lineH = metrics.labelLineHeight;
    const int lineCount = GetEffectiveViewMode() == ContentViewMode::SmallIcons ? 1 : maxLines;

    const float labelTop = cell.y + metrics.thumbSize + metrics.labelGap;

    const auto lines = WrapLabelText(context, name, maxWidth, fontSize, lineCount);
    for (size_t i = 0; i < lines.size(); ++i) {
        const float textW = context.GetTextWidth(lines[i], fontSize);
        const float x = cell.x + (cell.width - textW) * 0.5f;
        const float y = labelTop + static_cast<float>(i) * lineH;
        context.DrawText(lines[i], Point{ x, y }, ThemeColor(ThemeToken::TextPrimary), fontSize, false);
    }
}

void ContentBrowser::PaintGridItem(PaintContext& context, const RenderItem& renderItem) {
    const auto& item = renderItem.item;
    const bool selected = IsSelected(item.id);
    const bool hovered = item.id == m_HoveredId;
    const float hoverAlpha = hovered ? m_ItemHoverAlpha : 0.0f;

    // Use thumb geometry directly without padding to avoid empty space
    PaintTileChrome(context, renderItem.thumbGeometry, selected, hoverAlpha);
    PaintAssetThumbnail(context, renderItem.thumbGeometry, item, selected, hovered);

    const int labelLines = GetEffectiveViewMode() == ContentViewMode::SmallIcons ? 1 : 2;
    PaintItemLabel(context, renderItem.geometry, item.name, renderItem.geometry.width - 4.0f, labelLines);

    if (GetEffectiveViewMode() == ContentViewMode::Tiles && !item.isFolder) {
        const float typeW = context.GetTextWidth(item.type, 10.0f);
        const float x = renderItem.geometry.x + (renderItem.geometry.width - typeW) * 0.5f;
        const float y = renderItem.geometry.y + renderItem.geometry.height - 26.0f;
        context.DrawText(item.type, Point{ x, y }, ThemeColor(ThemeToken::TextSecondary), 10.0f);
    }
}

void ContentBrowser::PaintListItem(PaintContext& context, const RenderItem& renderItem) {
    const auto& item = renderItem.item;
    const bool selected = IsSelected(item.id);
    const bool hovered = item.id == m_HoveredId;

    if (selected || hovered) {
        PanelChrome::PaintListRowBackground(context, renderItem.geometry, hovered, selected);
    }

    const float iconSize = static_cast<float>(IconMetrics::NativeIconTierPx(ThemeMetric(ThemeToken::IconSizeTree)));
    const float iconX = renderItem.geometry.x + PanelChrome::PanelPaddingH();
    const float iconY = renderItem.geometry.y + (renderItem.geometry.height - iconSize) * 0.5f;
    Rect iconRect{ iconX, iconY, iconSize, iconSize };

    if (item.isFolder) {
        ContentBrowserFolderArt::Get().PaintSmallIcon(context, iconRect, hovered);
    } else if (IsBlueprintItem(item)) {
        ContentBrowserBlueprintArt::Get().PaintSmallIcon(context, iconRect, hovered);
    } else if (item.iconTexture != VK_NULL_HANDLE) {
        context.DrawTexture(iconRect, item.iconTexture);
    } else {
        IconPainter::DrawIcon(context, item.iconName, iconRect, ThemeColor(ThemeToken::IconDefault));
    }

    const float nameX = iconX + iconSize + ThemeMetric(ThemeToken::Space2);
    const float nameY = renderItem.geometry.y + (renderItem.geometry.height - ThemeMetric(ThemeToken::TextSizeBody)) * 0.5f;
    const float typeW = context.GetTextWidth(item.type, ThemeMetric(ThemeToken::TextSizeBody));
    const float maxNameWidth = renderItem.geometry.width - (nameX - renderItem.geometry.x) - typeW - PanelChrome::PanelPaddingH() * 2.0f;
    std::string displayName = item.name;
    if (context.GetTextWidth(displayName, ThemeMetric(ThemeToken::TextSizeBody)) > maxNameWidth) {
        while (displayName.length() > 1 && context.GetTextWidth(displayName + "...", ThemeMetric(ThemeToken::TextSizeBody)) > maxNameWidth) {
            displayName.pop_back();
        }
        displayName += "...";
    }
    context.DrawText(displayName, Point{ nameX, nameY }, ThemeColor(ThemeToken::TextPrimary), ThemeMetric(ThemeToken::TextSizeBody), false);
    context.DrawText(item.type, Point{ renderItem.geometry.x + renderItem.geometry.width - typeW - PanelChrome::PanelPaddingH(), nameY },
        ThemeColor(ThemeToken::TextSecondary), ThemeMetric(ThemeToken::TextSizeBody));
}

void ContentBrowser::Paint(PaintContext& context) {
    PanelChrome::PaintContentRegion(context, m_Geometry);
    SyncScrollMetrics();
    UpdateVisibleRange();

    const float viewTop = m_ScrollMetrics.viewport.y;
    const float viewBottom = m_ScrollMetrics.viewport.y + m_ScrollMetrics.viewport.height;
    const bool isGridLike = GetEffectiveViewMode() != ContentViewMode::List &&
                            GetEffectiveViewMode() != ContentViewMode::Details;

    context.PushClipRect(m_ScrollMetrics.viewport);

    for (int i = m_FirstVisibleIndex; i <= m_LastVisibleIndex && i < static_cast<int>(m_RenderList.size()); ++i) {
        const auto& renderItem = m_RenderList[static_cast<size_t>(i)];
        if (renderItem.geometry.y + renderItem.geometry.height < viewTop ||
            renderItem.geometry.y > viewBottom) {
            continue;
        }
        if (isGridLike) PaintGridItem(context, renderItem);
        else PaintListItem(context, renderItem);
    }

    context.PopClipRect();

    if (m_IsSelecting) {
        const float minX = std::min(m_SelectStart.x, m_SelectEnd.x);
        const float maxX = std::max(m_SelectStart.x, m_SelectEnd.x);
        const float minY = std::min(m_SelectStart.y, m_SelectEnd.y);
        const float maxY = std::max(m_SelectStart.y, m_SelectEnd.y);
        Rect selectBox{ minX, minY, maxX - minX, maxY - minY };
        context.DrawRect(selectBox, ThemeColor(ThemeToken::SelectionHighlight));
        context.DrawRoundedRectOutline(selectBox, ThemeColor(ThemeToken::AccentPrimary), 1.0f, 0.0f);
    }

    if (m_IsDragging && m_Model && !m_Model->selectedIds.empty()) {
        for (const auto& renderItem : m_RenderList) {
            if (!IsSelected(renderItem.item.id)) continue;
            Rect ghostRect{ m_MousePos.x - 40.0f, m_MousePos.y - 40.0f, 80.0f, 80.0f };
            context.DrawRoundedRect(ghostRect, ThemeColor(ThemeToken::DragGhostBackground), 4.0f);
            context.DrawRoundedRectOutline(ghostRect, ThemeColor(ThemeToken::AccentPrimary), 1.0f, 4.0f);
            if (renderItem.item.iconTexture != VK_NULL_HANDLE) {
                Rect iconRect{ ghostRect.x + 12.0f, ghostRect.y + 12.0f, 56.0f, 56.0f };
                context.DrawTexture(iconRect, renderItem.item.iconTexture);
            }
            if (m_Model->selectedIds.size() > 1) {
                const std::string countStr = std::to_string(m_Model->selectedIds.size());
                Rect badgeRect{ ghostRect.x + ghostRect.width - 24.0f, ghostRect.y - 8.0f, 32.0f, 20.0f };
                context.DrawRoundedRect(badgeRect, ThemeColor(ThemeToken::ErrorForeground), 10.0f);
                context.DrawText(countStr, Point{ badgeRect.x + ThemeMetric(ThemeToken::Space2), badgeRect.y + ThemeMetric(ThemeToken::Space1) - 2.0f }, ThemeColor(ThemeToken::TextPrimary), ThemeMetric(ThemeToken::TextSizeSmall));
            }
            break;
        }
    }

    m_Scroll.Paint(context, m_ScrollMetrics, m_Scroll.IsThumbHovered());
}

void ContentBrowser::ScrollSelectionIntoView() {
    if (!m_Model || m_Model->selectedIds.empty()) {
        return;
    }

    const std::string& selectedId = m_Model->selectedIds.back();
    for (const auto& item : m_RenderList) {
        if (item.item.id != selectedId) {
            continue;
        }
        const float top = item.geometry.y + m_Scroll.offset - m_ScrollMetrics.viewport.y;
        const float bottom = top + item.geometry.height;
        if (m_Scroll.ScrollToRange(top, bottom, m_Geometry.height, m_ContentHeight)) {
            RecalculateLayout();
            UpdateVisibleRange();
        }
        break;
    }
}

void ContentBrowser::OnMouseDown(const MouseEvent& event) {
    SyncScrollMetrics();
    if (m_Scroll.OnMouseDown(event, m_ScrollMetrics, m_Geometry.height, m_ContentHeight)) {
        RecalculateLayout();
        UpdateVisibleRange();
        return;
    }

    RenderItem* renderItem = GetItemAtPosition(event.position);

    if (event.button == MouseButton::Left) {
        const double now = SDL_GetTicks() / 1000.0;
        const bool isDoubleClick = renderItem &&
            (now - m_LastClickTime) < 0.35 &&
            std::abs(event.position.x - m_LastClickPos.x) < 4.0f &&
            std::abs(event.position.y - m_LastClickPos.y) < 4.0f;

        if (isDoubleClick && m_OnItemDoubleClicked) {
            m_OnItemDoubleClicked(renderItem->item);
            m_LastClickTime = 0.0;
            return;
        }
        m_LastClickTime = now;
        m_LastClickPos = event.position;

        if (renderItem) {
            const bool ctrlDown = event.ctrlDown;
            const bool shiftDown = event.shiftDown;

            if (ctrlDown) {
                if (IsSelected(renderItem->item.id)) m_Controller->RemoveFromSelection(renderItem->item.id);
                else m_Controller->AddToSelection(renderItem->item.id);
            } else if (shiftDown && !m_LastSelectedId.empty()) {
                int startIdx = -1, endIdx = -1;
                for (size_t i = 0; i < m_RenderList.size(); ++i) {
                    if (m_RenderList[i].item.id == m_LastSelectedId) startIdx = static_cast<int>(i);
                    if (m_RenderList[i].item.id == renderItem->item.id) endIdx = static_cast<int>(i);
                }
                if (startIdx != -1 && endIdx != -1) {
                    ClearSelection();
                    const int minIdx = std::min(startIdx, endIdx);
                    const int maxIdx = std::max(startIdx, endIdx);
                    for (int i = minIdx; i <= maxIdx; ++i) m_Controller->AddToSelection(m_RenderList[static_cast<size_t>(i)].item.id);
                    m_LastSelectedId = renderItem->item.id;
                } else {
                    m_Controller->SetSelectedId(renderItem->item.id);
                }
            } else {
                if (!IsSelected(renderItem->item.id)) m_Controller->SetSelectedId(renderItem->item.id);
                m_DragStart = event.position;
                m_IsDragging = false;
            }
            m_LastSelectedId = renderItem->item.id;
            if (m_OnItemSelected) m_OnItemSelected(renderItem->item);
        } else {
            ClearSelection();
            m_IsSelecting = true;
            m_SelectStart = event.position;
            m_SelectEnd = event.position;
            m_DragStart = Point{0,0};
        }
    } else if (event.button == MouseButton::Right) {
        if (renderItem) {
            if (!IsSelected(renderItem->item.id)) {
                m_Controller->SetSelectedId(renderItem->item.id);
                if (m_OnItemSelected) m_OnItemSelected(renderItem->item);
            }
            if (m_OnItemRightClicked) m_OnItemRightClicked(renderItem->item, event.position);
        } else {
            ClearSelection();
            if (m_OnBackgroundRightClicked) m_OnBackgroundRightClicked(event.position);
        }
    }
}

void ContentBrowser::OnMouseMove(const MouseEvent& event) {
    m_MousePos = event.position;

    SyncScrollMetrics();
    m_Scroll.OnMouseMove(event, m_ScrollMetrics, m_Geometry.height, m_ContentHeight);
    if (m_Scroll.IsDraggingThumb()) {
        RecalculateLayout();
        UpdateVisibleRange();
        return;
    }

    if (m_IsSelecting) {
        m_SelectEnd = event.position;
        const float minX = std::min(m_SelectStart.x, m_SelectEnd.x);
        const float maxX = std::max(m_SelectStart.x, m_SelectEnd.x);
        const float minY = std::min(m_SelectStart.y, m_SelectEnd.y);
        const float maxY = std::max(m_SelectStart.y, m_SelectEnd.y);
        Rect selectBox{ minX, minY, maxX - minX, maxY - minY };
        ClearSelection();
        for (const auto& renderItem : m_RenderList) {
            Rect intersection = renderItem.geometry.Intersect(selectBox);
            if (intersection.width > 0.0f && intersection.height > 0.0f) {
                m_Controller->AddToSelection(renderItem.item.id);
            }
        }
    } else if (m_DragStart.x != 0.0f || m_DragStart.y != 0.0f) {
        const float dx = event.position.x - m_DragStart.x;
        const float dy = event.position.y - m_DragStart.y;
        if (dx * dx + dy * dy > 25.0f) m_IsDragging = true;
    }

    RenderItem* renderItem = GetItemAtPosition(event.position);
    m_HoveredId = renderItem ? renderItem->item.id : "";
}

void ContentBrowser::OnMouseUp(const MouseEvent& event) {
    m_Scroll.OnMouseUp(event);

    if (event.button != MouseButton::Left) return;

    if (m_IsSelecting) {
        m_IsSelecting = false;
    } else if (m_IsDragging) {
        m_IsDragging = false;
        m_DragStart = Point{0,0};
        RenderItem* target = GetItemAtPosition(event.position);
        if (target && target->item.isFolder) {
            for (const auto& id : m_Model->selectedIds) {
                if (id == target->item.id) continue;
                // Drop into folder - future filesystem move hook
            }
        }
    } else {
        m_DragStart = Point{0,0};
    }
}

void ContentBrowser::OnMouseWheel(const MouseEvent& event) {
    SyncScrollMetrics();
    const GridMetrics metrics = GetGridMetrics();
    const float stride = (GetEffectiveViewMode() == ContentViewMode::List ||
                          GetEffectiveViewMode() == ContentViewMode::Details)
        ? m_ListRowHeight : metrics.cellHeight + metrics.vSpacing;
    m_Scroll.ApplyWheel(
        event.wheelDeltaY,
        stride * 0.5f,
        m_Geometry.height,
        m_ContentHeight);
    RecalculateLayout();
    UpdateVisibleRange();
}

bool ContentBrowser::ShowsPointerCursor(const Point& position) const {
    return m_ScrollMetrics.showsScrollbar &&
        (m_ScrollMetrics.thumb.Contains(position) || m_ScrollMetrics.track.Contains(position));
}

void ContentBrowser::OnKeyDown(const KeyEvent& event) {
    if (!m_Model || m_RenderList.empty()) return;
    int current = -1;
    for (size_t i = 0; i < m_RenderList.size(); ++i) {
        if (IsSelected(m_RenderList[i].item.id)) { current = static_cast<int>(i); break; }
    }
    if (current < 0) current = 0;

    if (event.keycode == SDLK_DOWN) current = std::min(current + 1, static_cast<int>(m_RenderList.size()) - 1);
    else if (event.keycode == SDLK_UP) current = std::max(current - 1, 0);
    else return;

    m_Controller->SetSelectedId(m_RenderList[static_cast<size_t>(current)].item.id);
    if (m_OnItemSelected) m_OnItemSelected(m_RenderList[static_cast<size_t>(current)].item);
    ScrollSelectionIntoView();
}

void ContentBrowser::AddItem(const ContentItem& item) {
    if (m_Controller) m_Controller->AddItem(item);
}

void ContentBrowser::RemoveItem(const std::string& id) {
    if (m_Controller) m_Controller->RemoveItem(id);
}

void ContentBrowser::Clear() {
    if (m_Controller) m_Controller->Clear();
}

bool ContentBrowser::IsSelected(const std::string& id) const {
    if (!m_Model) return false;
    return std::find(m_Model->selectedIds.begin(), m_Model->selectedIds.end(), id) != m_Model->selectedIds.end();
}

void ContentBrowser::BuildRenderList() {
    m_RenderList.clear();
    if (!m_Model) return;

    auto& service = ContentBrowserService::Get();
    const auto& search = service.GetSearchController();
    const auto& filters = service.GetFilterController();

    int index = 0;
    for (const auto& item : m_Model->items) {
        if (const auto* asset = service.GetRegistry().FindById(item.id)) {
            if (!search.Matches(*asset)) continue;
            if (!filters.Matches(*asset)) continue;
        } else if (!m_Model->filterText.empty()) {
            std::string lowerName = item.name;
            std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(),
                [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            if (lowerName.find(m_Model->filterText) == std::string::npos) continue;
        }

        RenderItem renderItem;
        renderItem.item = item;
        renderItem.sourceIndex = index++;
        renderItem.geometry = Rect{};
        m_RenderList.push_back(renderItem);
    }
}

void ContentBrowser::CalculateGridLayout() {
    const GridMetrics m = GetGridMetrics();
    const float contentX = m_ScrollMetrics.viewport.x;
    const float contentWidth = m_ScrollMetrics.viewport.width;
    float x = contentX + m.padding;
    float y = m_ScrollMetrics.viewport.y + m.padding - m_Scroll.offset;
    const int itemsPerRow = std::max(1,
        static_cast<int>((contentWidth - m.padding * 2.0f + m.hSpacing) / (m.cellWidth + m.hSpacing)));

    for (size_t i = 0; i < m_RenderList.size(); ++i) {
        const int col = static_cast<int>(i) % itemsPerRow;
        const int row = static_cast<int>(i) / itemsPerRow;
        const float cellX = x + col * (m.cellWidth + m.hSpacing);
        const float cellY = y + row * (m.cellHeight + m.vSpacing);

        const float thumbSize = m.thumbSize;
        const float thumbX = cellX + (m.cellWidth - thumbSize) * 0.5f;

        m_RenderList[i].geometry = Rect{ cellX, cellY, m.cellWidth, m.cellHeight };
        m_RenderList[i].thumbGeometry = Rect{ thumbX, cellY, thumbSize, thumbSize };
    }
}

void ContentBrowser::CalculateTilesLayout() {
    const GridMetrics m = GetGridMetrics();
    const float contentX = m_ScrollMetrics.viewport.x;
    const float contentWidth = m_ScrollMetrics.viewport.width;
    float x = contentX + m.padding;
    float y = m_ScrollMetrics.viewport.y + m.padding - m_Scroll.offset;
    const int itemsPerRow = std::max(1,
        static_cast<int>((contentWidth - m.padding * 2.0f + m.hSpacing) / (m.cellWidth + m.hSpacing)));

    for (size_t i = 0; i < m_RenderList.size(); ++i) {
        const int col = static_cast<int>(i) % itemsPerRow;
        const int row = static_cast<int>(i) / itemsPerRow;
        const float cellX = x + col * (m.cellWidth + m.hSpacing);
        const float cellY = y + row * (m.cellHeight + m.vSpacing);
        const float thumbSize = m.thumbSize;

        m_RenderList[i].geometry = Rect{ cellX, cellY, m.cellWidth, m.cellHeight };
        m_RenderList[i].thumbGeometry = Rect{
            cellX + (m.cellWidth - thumbSize) * 0.5f,
            cellY,
            thumbSize,
            thumbSize
        };
    }
}

void ContentBrowser::CalculateListLayout() {
    const float contentX = m_ScrollMetrics.viewport.x;
    const float contentWidth = m_ScrollMetrics.viewport.width;
    float y = m_ScrollMetrics.viewport.y - m_Scroll.offset;
    for (auto& renderItem : m_RenderList) {
        renderItem.geometry = Rect{ contentX, y, contentWidth, m_ListRowHeight };
        renderItem.thumbGeometry = renderItem.geometry;
        y += m_ListRowHeight;
    }
}

void ContentBrowser::CalculateDetailsLayout() {
    CalculateListLayout();
}

ContentBrowser::RenderItem* ContentBrowser::GetItemAtPosition(const Point& pos) {
    for (auto& renderItem : m_RenderList) {
        if (renderItem.geometry.Contains(pos)) return &renderItem;
    }
    return nullptr;
}

ContentBrowserStatusBar::ContentBrowserStatusBar() = default;

Size ContentBrowserStatusBar::Measure(const Size& availableSize) {
    (void)availableSize;
    m_DesiredSize = Size{ availableSize.width, 20.0f };
    return m_DesiredSize;
}

void ContentBrowserStatusBar::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
}

void ContentBrowserStatusBar::Paint(PaintContext& context) {
    context.DrawRect(m_Geometry, ThemeColor(ThemeToken::HeaderBackground));
    const size_t total = m_AssetCount + m_FolderCount;
    std::string text = std::to_string(total) + " items";
    if (m_SelectedCount > 0) {
        text += "  ·  " + std::to_string(m_SelectedCount) + " selected";
    }
    context.DrawText(text, Point{ m_Geometry.x + 12.0f, m_Geometry.y + 3.0f }, ThemeColor(ThemeToken::TextSecondary), 11.0f);
}

Breadcrumb::Breadcrumb() = default;

Size Breadcrumb::Measure(const Size& availableSize) {
    CalculateLayout();
    return Size{ availableSize.width, 32.0f };
}

void Breadcrumb::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
    CalculateLayout();
}

void Breadcrumb::Paint(PaintContext& context) {
    context.DrawRect(m_Geometry, ThemeColor(ThemeToken::PanelBackground));
    context.DrawRect(Rect{ m_Geometry.x, m_Geometry.y + m_Geometry.height - 1.0f, m_Geometry.width, 1.0f }, ThemeColor(ThemeToken::Separator));

    const float iconSize = 16.0f;
    const float iconY = m_Geometry.y + (m_Geometry.height - iconSize) * 0.5f;
    ContentBrowserFolderArt::Get().PaintSmallIcon(context,
        WindEffects::Editor::UI::Rect{ m_Geometry.x + 12.0f, iconY, iconSize, iconSize }, false);

    float x = m_Geometry.x + 12.0f + iconSize + 8.0f;
    for (size_t i = 0; i < m_Crumbs.size(); ++i) {
        const auto& crumb = m_Crumbs[i];
        if (crumb.hovered) {
            context.DrawRoundedRect(crumb.geometry, ThemeColor(ThemeToken::HighlightSubtle), 4.0f);
        }
        const float textX = crumb.geometry.x + 8.0f;
        const float textY = crumb.geometry.y + (crumb.geometry.height - 13.0f) * 0.5f;
        const Color textColor = static_cast<int>(i) == m_HoveredCrumb ? ThemeColor(ThemeToken::IconHover) : ThemeColor(ThemeToken::IconDefault);
        context.DrawText(crumb.text, Point{ textX, textY }, textColor, 13.0f, false);
        if (i < m_Crumbs.size() - 1) {
            const float sepX = crumb.geometry.x + crumb.geometry.width + 4.0f;
            context.DrawText("/", Point{ sepX, textY }, ThemeColor(ThemeToken::TextSecondary), 12.0f);
        }
    }
}

void Breadcrumb::OnMouseDown(const MouseEvent& event) {
    CrumbInfo* crumb = GetCrumbAtPosition(event.position);
    if (crumb && m_OnCrumbClicked) {
        const size_t index = static_cast<size_t>(crumb - &m_Crumbs[0]);
        m_OnCrumbClicked(index);
    }
}

void Breadcrumb::OnMouseMove(const MouseEvent& event) {
    m_HoveredCrumb = -1;
    for (size_t i = 0; i < m_Crumbs.size(); ++i) m_Crumbs[i].hovered = false;
    if (CrumbInfo* crumb = GetCrumbAtPosition(event.position)) {
        m_HoveredCrumb = static_cast<int>(crumb - &m_Crumbs[0]);
        m_Crumbs[static_cast<size_t>(m_HoveredCrumb)].hovered = true;
    }
}

void Breadcrumb::SetPath(const std::vector<std::string>& path) {
    m_PathSegments = path;
    m_Crumbs.clear();
    for (const auto& crumb : path) {
        CrumbInfo info;
        info.text = crumb;
        m_Crumbs.push_back(info);
    }
    CalculateLayout();
}

void Breadcrumb::AddCrumb(const std::string& crumb) {
    CrumbInfo info;
    info.text = crumb;
    m_Crumbs.push_back(info);
    CalculateLayout();
}

void Breadcrumb::Clear() {
    m_Crumbs.clear();
}

void Breadcrumb::CalculateLayout() {
    const float iconSize = 16.0f;
    float x = m_Geometry.x + 12.0f + iconSize + 8.0f;
    const float h = std::max(32.0f, m_Geometry.height);
    for (auto& crumb : m_Crumbs) {
        const float textWidth = crumb.text.length() * 13.0f * 0.58f;
        const float width = std::max(32.0f, textWidth + 16.0f);
        crumb.geometry = Rect{ x, m_Geometry.y + (h - 32.0f) * 0.5f, width, 32.0f };
        x += width + 4.0f;
    }
}

Breadcrumb::CrumbInfo* Breadcrumb::GetCrumbAtPosition(const Point& pos) {
    for (auto& crumb : m_Crumbs) {
        if (crumb.geometry.Contains(pos)) return &crumb;
    }
    return nullptr;
}

} // namespace WindEffects::Editor::UI
