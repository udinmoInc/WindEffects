#include "Platform/Platform.h"
#include "PlaceActors/PlaceActorsPanel.h"

#include "PlaceActors/ActorsPanelLayout.h"
#include "PlaceActors/ActorsPanelChrome.h"
#include "PlaceActors/PlaceActorsCatalog.h"
#include "PlaceActors/PlaceActorsConfig.h"
#include "PlaceActors/PlaceActorsSearch.h"
#include "PlaceActors/PlaceActorsPlacement.h"
#include "PlaceActors/PlaceActorsItem.h"
#include "PlaceActors/PlaceActorsCategory.h"
#include "PlaceActors/PlaceActorsActorCard.h"
#include "PlaceActors/PlaceActorsResponsiveGrid.h"
#include "PlaceActors/PlaceActorsFavoritesManager.h"
#include "PlaceActors/PlaceActorsRecentlyUsedManager.h"
#include "PlaceActors/PlaceActorsThumbnailProvider.h"

#include "WindEffects/Editor/UI/Shell/EditorToolsRegistry.h"
#include "ContentBrowser/Widgets/SearchBox.h"
#include "Widgets/ToolButton.h"
#include "WindEffects/Editor/UI/Panel/PanelChrome.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Core/DPIContext.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Theming/StyleRole.h"
#include "KindUI/Core/Icon.h"
#include "KindUI/Core/Animator.h"
#include "Core/EditorConfigPaths.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>


namespace we::programs::editor {
namespace PanelChrome = ::we::editor::panels::PanelChrome;
void PlaceActorsPanel::Paint(we::runtime::kindui::PaintContext& context) {
    PanelChrome::PaintContentRegion(context, m_Geometry);

    ActorsPanelChrome::PaintSearchField(
        context,
        m_SearchBox->GetGeometry(),
        "Search Assets...",
        m_SearchText,
        m_SearchBox->IsFocused(),
        m_SearchBox->ShouldShowCaret());
    m_FilterButton->Paint(context);

    auto& catalog = PlaceActorsCatalog::Get();
    auto& favorites = PlaceActorsFavoritesManager::Get();
    PlaceActorsItemMetrics metrics;
    metrics.iconSize = ActorsPanelLayout::IconSize();
    metrics.cardSize = PlaceActorsConfig::Get().cardSize;
    metrics.listRowHeight = ActorsPanelLayout::ActorRowHeight();
    metrics.cornerRadius = ThemeMetric(MetricToken::CornerRadiusSmall);

    // Warm thumbnail cache lazily for visible grid items.
    (void)PlaceActorsThumbnailProvider::Get();

    SyncScrollMetrics();
    UpdateVisibleRange();
    const std::string query = !m_ExternalSearchFilter.empty() ? m_ExternalSearchFilter : m_SearchText;
    const float textSize = ThemeMetric(MetricToken::TextSizeSmall);
    const Rect clip = m_ScrollMetrics.viewport;

    context.PushClipRect(clip);

    for (const auto& section : m_SectionBackgrounds) {
        if (section.y + section.height < clip.y || section.y > clip.y + clip.height) {
            continue;
        }
        PlaceActorsCategory::PaintSectionBackground(context, section);
    }

    for (int i = m_FirstVisibleIndex; i <= m_LastVisibleIndex && i < static_cast<int>(m_Layout.size()); ++i) {
        auto& entry = m_Layout[static_cast<size_t>(i)];

        if (entry.type == LayoutEntry::Type::EmptyState) {
            const std::string message = m_HasSearchResults
                ? "Drag actors here to favorite"
                : "No Results Found";
            PlaceActorsCategory::PaintEmptyState(context, entry.geometry, message);
            continue;
        }

        if (entry.type == LayoutEntry::Type::CategoryHeader) {
            const PlaceActorsCategoryData* category = nullptr;
            for (const auto& candidate : m_DisplayCategories) {
                if (candidate.id == entry.categoryId) {
                    category = &candidate;
                    break;
                }
            }
            if (!category) continue;
            const bool expanded = CategoryExpandAnim(entry.categoryId) > 0.5f || IsPinnedCategory(entry.categoryId);
            const bool showChevron = !IsPinnedCategory(entry.categoryId)
                && (!category->items.empty() || IsFavoritesCategory(entry.categoryId));
            PlaceActorsCategory::PaintHeader(
                context,
                entry.geometry,
                category->label,
                category->iconName,
                expanded,
                entry.hoverAnim,
                CategoryExpandAnim(entry.categoryId),
                IsFavoritesCategory(entry.categoryId),
                showChevron);
            continue;
        }

        const PlaceActorsItemData* item = catalog.FindItem(entry.toolId);
        if (!item) continue;
        const bool favorite = favorites.IsFavorite(item->toolId);
        if (m_ViewMode == PlaceActorsViewMode::Grid) {
            const Rect preview = entry.previewGeometry.width > 0.0f
                ? entry.previewGeometry
                : Rect{ entry.geometry.x, entry.geometry.y, entry.geometry.width, entry.geometry.width };
            PlaceActorsActorCard::Paint(
                context, entry.geometry, preview, *item,
                entry.hoverAnim, entry.pressAnim, entry.selected, favorite);
        } else {
            PlaceActorsItem::PaintList(
                context, entry.geometry, *item, metrics,
                entry.hoverAnim, entry.pressAnim, entry.selected, favorite, query, entry.revealAnim);
        }
    }

    context.PopClipRect();
    m_Scroll.Paint(context, m_ScrollMetrics, m_Scroll.IsThumbHovered());

    if (!m_TooltipText.empty() && m_TooltipRect.width > 0.0f) {
        context.DrawShadow(m_TooltipRect, ThemeColor(ColorToken::ShadowPopup), 3.0f, 6.0f);
        context.DrawRoundedRect(m_TooltipRect, ThemeColor(ColorToken::PopupBackground), ThemeMetric(MetricToken::CornerRadiusSmall));
        context.DrawText(m_TooltipText,
            Point{ m_TooltipRect.x + ActorsPanelLayout::ContentPadH(), m_TooltipRect.y + (m_TooltipRect.height - textSize) * 0.5f },
            ThemeColor(ColorToken::TextSecondary), textSize);
    }

    if (m_FilterMenuOpen) {
        context.DrawShadow(m_FilterMenuRect, ThemeColor(ColorToken::ShadowPopup), 3.0f, 8.0f);
        context.DrawRoundedRect(m_FilterMenuRect, ThemeColor(ColorToken::PopupBackground), ThemeMetric(MetricToken::CornerRadiusSmall));
        for (size_t i = 0; i < m_FilterMenuItems.size(); ++i) {
            if (static_cast<int>(i) == m_FilterMenuHovered) {
                context.DrawRect(m_FilterMenuItems[i].geometry, ThemeColor(ColorToken::HoverBackground));
            }
            const Color textColor = m_FilterMenuItems[i].checked
                ? ThemeColor(ColorToken::AccentPrimary)
                : ThemeColor(ColorToken::TextPrimary);
            context.DrawText(m_FilterMenuItems[i].label,
                Point{ m_FilterMenuItems[i].geometry.x + ActorsPanelLayout::ContentPadH(), m_FilterMenuItems[i].geometry.y + (m_FilterMenuItems[i].geometry.height - textSize) * 0.5f },
                textColor, textSize);
        }
    }

    if (m_ContextMenuOpen) {
        context.DrawShadow(m_ContextMenuRect, ThemeColor(ColorToken::ShadowPopup), 3.0f, 8.0f);
        context.DrawRoundedRect(m_ContextMenuRect, ThemeColor(ColorToken::PopupBackground), ThemeMetric(MetricToken::CornerRadiusSmall));
        const float rowH = ActorsPanelLayout::ActorRowHeight();
        for (size_t i = 0; i < m_ContextMenuItems.size(); ++i) {
            if (static_cast<int>(i) == m_ContextMenuHovered) {
                context.DrawRect(m_ContextMenuItems[i].geometry, ThemeColor(ColorToken::HoverBackground));
            }
            context.DrawText(m_ContextMenuItems[i].label,
                Point{ m_ContextMenuItems[i].geometry.x + ActorsPanelLayout::ContentPadH(), m_ContextMenuItems[i].geometry.y + (rowH - textSize) * 0.5f },
                ThemeColor(ColorToken::TextPrimary), textSize);
        }
    }
}

PlaceActorsPanel::LayoutEntry* PlaceActorsPanel::HitEntry(const Point& position) {
    for (auto& entry : m_Layout) {
        if (entry.geometry.Contains(position)) {
            return &entry;
        }
    }
    return nullptr;
}

void PlaceActorsPanel::SpawnItem(const std::string& toolId) {
    PlaceActorsPlacement::Get().SpawnTool(toolId);
    RebuildData();
}

void PlaceActorsPanel::ToggleFavorite(const std::string& toolId) {
    PlaceActorsFavoritesManager::Get().Toggle(toolId);
    SaveFavorites();
    RefreshFilteredContent();
}

void PlaceActorsPanel::ToggleFilterMenu(const Point& anchor) {
    if (m_FilterMenuOpen) {
        CloseFilterMenu();
        return;
    }

    m_FilterMenuOpen = true;
    m_FilterMenuItems.clear();
    const float rowH = ActorsPanelLayout::ActorRowHeight();
    const float menuWidth = 196.0f;
    float y = anchor.y;

    auto addItem = [&](const std::string& label, bool checked, auto action) {
        FilterMenuItem item;
        item.label = label;
        item.checked = checked;
        item.geometry = Rect{ anchor.x - menuWidth + ActorsPanelLayout::ToolbarIconSize(), y, menuWidth, rowH };
        item.action = std::move(action);
        m_FilterMenuItems.push_back(std::move(item));
        y += rowH;
    };

    addItem("Show Recent Only", m_ShowRecentOnly, [this]() {
        m_ShowRecentOnly = !m_ShowRecentOnly;
        RefreshFilteredContent();
    });

    addItem(m_ViewMode == PlaceActorsViewMode::List ? "Grid View" : "List View", false, [this]() {
        m_ViewMode = (m_ViewMode == PlaceActorsViewMode::Grid) ? PlaceActorsViewMode::List : PlaceActorsViewMode::Grid;
        RefreshFilteredContent();
    });

    switch (m_SortMode) {
    case PlaceActorsSortMode::Category:
        addItem("Sort: Category", true, [this]() { m_SortMode = PlaceActorsSortMode::Name; RefreshFilteredContent(); });
        break;
    case PlaceActorsSortMode::Recent:
        addItem("Sort: Recent", true, [this]() { m_SortMode = PlaceActorsSortMode::Name; RefreshFilteredContent(); });
        break;
    default:
        addItem("Sort: Name", true, [this]() { m_SortMode = PlaceActorsSortMode::Category; RefreshFilteredContent(); });
        break;
    }

    const auto labels = PlaceActorsCatalog::Get().GetCategoryFilterLabels();
    for (const auto& label : labels) {
        addItem(label == "All" ? "Category: All" : "Category: " + label, m_CategoryFilter == label, [this, label]() {
            m_CategoryFilter = label;
            RefreshFilteredContent();
        });
    }

    m_FilterMenuRect = Rect{
        m_FilterMenuItems.front().geometry.x,
        m_FilterMenuItems.front().geometry.y,
        menuWidth,
        rowH * static_cast<float>(m_FilterMenuItems.size())
    };
}

void PlaceActorsPanel::CloseFilterMenu() {
    m_FilterMenuOpen = false;
    m_FilterMenuItems.clear();
    m_FilterMenuHovered = -1;
}

void PlaceActorsPanel::OpenContextMenu(const std::string& toolId, const Point& position) {
    m_ContextMenuOpen = true;
    m_ContextMenuItems.clear();
    const float itemHeight = ActorsPanelLayout::ActorRowHeight();
    const float menuWidth = 168.0f;
    float y = position.y;
    auto addItem = [&](const std::string& label, auto action) {
        ContextMenuItem item;
        item.label = label;
        item.geometry = Rect{ position.x, y, menuWidth, itemHeight };
        item.action = std::move(action);
        m_ContextMenuItems.push_back(std::move(item));
        y += itemHeight;
    };

    addItem("Place in Level", [this, toolId]() { SpawnItem(toolId); });
    addItem("Drag to Viewport", [toolId]() { PlaceActorsPlacement::Get().BeginDragPlacement(toolId); });
    addItem("Toggle Favorite", [this, toolId]() { ToggleFavorite(toolId); });
    m_ContextMenuRect = Rect{ position.x, position.y, menuWidth, itemHeight * static_cast<float>(m_ContextMenuItems.size()) };
}

void PlaceActorsPanel::CloseContextMenu() {
    m_ContextMenuOpen = false;
    m_ContextMenuItems.clear();
    m_ContextMenuHovered = -1;
}

void PlaceActorsPanel::ShowTooltip(const PlaceActorsItemData& item, const Rect& anchor) {
    if (!PlaceActorsConfig::Get().showDescriptions) {
        return;
    }
    m_TooltipToolId = item.toolId;
    m_TooltipText = item.description;
    m_TooltipRect = Rect{ anchor.x, anchor.y + anchor.height + 4.0f, 220.0f, 28.0f };
}

void PlaceActorsPanel::HideTooltip() {
    m_TooltipText.clear();
    m_TooltipToolId.clear();
    m_TooltipRect = {};
}

bool PlaceActorsPanel::ShowsPointerCursor(const Point& position) const {
    return m_ScrollMetrics.showsScrollbar &&
        (m_ScrollMetrics.thumb.Contains(position) || m_ScrollMetrics.track.Contains(position));
}

void PlaceActorsPanel::OnMouseDown(const MouseEvent& event) {
    if (m_FilterMenuOpen) {
        if (m_FilterMenuRect.Contains(event.position)) {
            if (m_FilterMenuHovered >= 0 && m_FilterMenuHovered < static_cast<int>(m_FilterMenuItems.size())) {
                m_FilterMenuItems[static_cast<size_t>(m_FilterMenuHovered)].action();
            }
            CloseFilterMenu();
            return;
        }
        CloseFilterMenu();
        return;
    }

    if (m_ContextMenuOpen) {
        if (m_ContextMenuRect.Contains(event.position)) {
            if (m_ContextMenuHovered >= 0 && m_ContextMenuHovered < static_cast<int>(m_ContextMenuItems.size())) {
                m_ContextMenuItems[static_cast<size_t>(m_ContextMenuHovered)].action();
            }
            CloseContextMenu();
            return;
        }
        CloseContextMenu();
        return;
    }

    SyncScrollMetrics();
    if (m_Scroll.OnMouseDown(event, m_ScrollMetrics, m_ContentRect.height, m_ContentHeight)) {
        RebuildLayout();
        return;
    }

    if (m_SearchRowRect.Contains(event.position)) {
        if (m_SearchBox->GetGeometry().Contains(event.position)) m_SearchBox->OnMouseDown(event);
        else if (m_FilterButton->GetGeometry().Contains(event.position)) m_FilterButton->OnMouseDown(event);
        return;
    }

    if (!m_ContentRect.Contains(event.position)) {
        return;
    }

    if (event.button == MouseButton::Right) {
        if (LayoutEntry* entry = HitEntry(event.position)) {
            if (entry->type == LayoutEntry::Type::Item) {
                OpenContextMenu(entry->toolId, event.position);
            }
        }
        return;
    }

    if (LayoutEntry* entry = HitEntry(event.position)) {
        if (entry->type == LayoutEntry::Type::CategoryHeader) {
            if (IsPinnedCategory(entry->categoryId)) {
                return;
            }
            m_CategoryExpanded[entry->categoryId] = !m_CategoryExpanded[entry->categoryId];
            SaveCategoryState();
            m_NeedsLayout = true;
            RebuildLayout();
            return;
        }

        if (entry->type == LayoutEntry::Type::Item) {
            const bool hitStar = (m_ViewMode == PlaceActorsViewMode::Grid)
                ? PlaceActorsActorCard::HitFavoriteStar(entry->geometry, event.position)
                : (event.position.x >= ActorsPanelLayout::StarIconX(entry->geometry.x, entry->geometry.width) - 4.0f);
            if (hitStar) {
                ToggleFavorite(entry->toolId);
                return;
            }
        }

        entry->pressAnim = 1.0f;
        for (auto& e : m_Layout) {
            e.selected = (&e == entry);
        }
        m_FocusedIndex = static_cast<int>(std::distance(m_Layout.data(), entry));

        if (entry->type == LayoutEntry::Type::Item) {
            if (const PlaceActorsItemData* item = PlaceActorsCatalog::Get().FindItem(entry->toolId)) {
                m_PendingDragItem = item;
                m_DragStartPosition = event.position;
                m_DragStarted = false;
            }
        }
    }
}

void PlaceActorsPanel::OnMouseMove(const MouseEvent& event) {
    if (m_SearchRowRect.Contains(event.position)) {
        m_SearchBox->OnMouseMove(event);
        m_FilterButton->OnMouseMove(event);
    }

    SyncScrollMetrics();
    m_Scroll.OnMouseMove(event, m_ScrollMetrics, m_ContentRect.height, m_ContentHeight);
    if (m_Scroll.IsDraggingThumb()) {
        RebuildLayout();
        return;
    }

    HideTooltip();
    for (auto& entry : m_Layout) {
        const bool hovered = entry.geometry.Contains(event.position);
        if (PlaceActorsConfig::Get().enableAnimations) {
            entry.hoverAnim = we::runtime::kindui::Animator::Damp(entry.hoverAnim, hovered ? 1.0f : 0.0f, 14.0f);
        } else {
            entry.hoverAnim = hovered ? 1.0f : 0.0f;
        }
        if (hovered && entry.type == LayoutEntry::Type::Item) {
            if (const PlaceActorsItemData* item = PlaceActorsCatalog::Get().FindItem(entry.toolId)) {
                ShowTooltip(*item, entry.geometry);
            }
        }
    }

    if (m_PendingDragItem && !m_DragStarted) {
        const float dx = event.position.x - m_DragStartPosition.x;
        const float dy = event.position.y - m_DragStartPosition.y;
        if ((dx * dx + dy * dy) >= (kDragThreshold * kDragThreshold)) {
            m_DragStarted = true;
            m_LastClickToolId = m_PendingDragItem->toolId;
            PlaceActorsPlacement::Get().BeginDragPlacement(m_PendingDragItem->toolId);
            m_PendingDragItem = nullptr;
        }
    }

    if (m_ContextMenuOpen) {
        m_ContextMenuHovered = -1;
        for (size_t i = 0; i < m_ContextMenuItems.size(); ++i) {
            if (m_ContextMenuItems[i].geometry.Contains(event.position)) {
                m_ContextMenuHovered = static_cast<int>(i);
                break;
            }
        }
    }

    if (m_FilterMenuOpen) {
        m_FilterMenuHovered = -1;
        for (size_t i = 0; i < m_FilterMenuItems.size(); ++i) {
            if (m_FilterMenuItems[i].geometry.Contains(event.position)) {
                m_FilterMenuHovered = static_cast<int>(i);
                break;
            }
        }
    }
}

void PlaceActorsPanel::OnMouseUp(const MouseEvent& event) {
    m_Scroll.OnMouseUp(event);

    if (m_PendingDragItem && !m_DragStarted && event.button == MouseButton::Left) {
        const std::string toolId = m_PendingDragItem->toolId;
        const double now = we::platform::Platform::Get().GetTimeSeconds();
        const bool isDoubleClick = toolId == m_LastClickToolId && (now - m_LastClickTime) < 0.35;
        if (isDoubleClick) {
            SpawnItem(toolId);
            m_LastClickToolId.clear();
            m_LastClickTime = 0.0;
        } else {
            m_LastClickToolId = toolId;
            m_LastClickTime = now;
        }
    }

    if (event.button == MouseButton::Left) {
        if (LayoutEntry* dropTarget = HitEntry(event.position)) {
            if (dropTarget->type == LayoutEntry::Type::CategoryHeader && IsFavoritesCategory(dropTarget->categoryId)) {
                std::string toolId;
                if (m_PendingDragItem) {
                    toolId = m_PendingDragItem->toolId;
                } else if (!m_LastClickToolId.empty()) {
                    toolId = m_LastClickToolId;
                }
                if (!toolId.empty() && !PlaceActorsFavoritesManager::Get().IsFavorite(toolId)) {
                    PlaceActorsFavoritesManager::Get().SetFavorite(toolId, true);
                    SaveFavorites();
                    RefreshFilteredContent();
                }
            }
        }
    }
    m_PendingDragItem = nullptr;
    m_DragStarted = false;
}

void PlaceActorsPanel::OnMouseWheel(const MouseEvent& event) {
    if (!m_ContentRect.Contains(event.position)) {
        return;
    }
    SyncScrollMetrics();
    const float wheelStep = (m_ViewMode == PlaceActorsViewMode::Grid)
        ? ActorsPanelLayout::GridMinCardWidth() * 0.55f
        : ActorsPanelLayout::ActorRowHeight() * 0.75f;
    m_Scroll.ApplyWheel(event.wheelDeltaY, wheelStep, m_ContentRect.height, m_ContentHeight);
    RebuildLayout();
}

void PlaceActorsPanel::OnKeyDown(const KeyEvent& event) {
    if (m_Layout.empty()) {
        return;
    }

    if (event.key == we::platform::KeyCode::Enter && m_FocusedIndex >= 0 && m_FocusedIndex < static_cast<int>(m_Layout.size())) {
        const auto& entry = m_Layout[static_cast<size_t>(m_FocusedIndex)];
        if (entry.type == LayoutEntry::Type::Item) {
            SpawnItem(entry.toolId);
        }
        return;
    }

    int step = 0;
    if (event.key == we::platform::KeyCode::Down) {
        step = (m_ViewMode == PlaceActorsViewMode::Grid && m_FocusedIndex >= 0
            && m_FocusedIndex < static_cast<int>(m_Layout.size()))
            ? std::max(1, m_Layout[static_cast<size_t>(m_FocusedIndex)].gridColumns)
            : 1;
    } else if (event.key == we::platform::KeyCode::Up) {
        step = (m_ViewMode == PlaceActorsViewMode::Grid && m_FocusedIndex >= 0
            && m_FocusedIndex < static_cast<int>(m_Layout.size()))
            ? -std::max(1, m_Layout[static_cast<size_t>(m_FocusedIndex)].gridColumns)
            : -1;
    } else if (event.key == we::platform::KeyCode::Right) {
        step = 1;
    } else if (event.key == we::platform::KeyCode::Left) {
        step = -1;
    } else {
        return;
    }

    int next = m_FocusedIndex;
    if (next < 0) next = 0;
    next += step;
    next = std::clamp(next, 0, static_cast<int>(m_Layout.size()) - 1);

    const int direction = step >= 0 ? 1 : -1;
    while (next >= 0 && next < static_cast<int>(m_Layout.size()) &&
           m_Layout[static_cast<size_t>(next)].type != LayoutEntry::Type::Item) {
        next += direction;
    }
    if (next < 0 || next >= static_cast<int>(m_Layout.size())) {
        return;
    }

    m_FocusedIndex = next;
    for (auto& entry : m_Layout) {
        entry.selected = false;
    }
    m_Layout[static_cast<size_t>(m_FocusedIndex)].selected = true;
    ScrollFocusedIntoView();
}

} // namespace we::programs::editor
