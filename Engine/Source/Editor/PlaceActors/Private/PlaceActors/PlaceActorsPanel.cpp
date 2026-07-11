#include "PlaceActors/PlaceActorsPanel.h"

#include "PlaceActors/PlaceActorsCatalog.h"
#include "PlaceActors/PlaceActorsConfig.h"
#include "PlaceActors/PlaceActorsSearch.h"
#include "PlaceActors/PlaceActorsPlacement.h"
#include "PlaceActors/PlaceActorsItem.h"
#include "PlaceActors/PlaceActorsCategory.h"

#include "EditorToolsRegistry.h"
#include "Widgets/SearchBox.h"
#include "Widgets/ToolButton.h"
#include "Core/PaintContext.h"
#include "WindEffects/Editor/UI/Theming/ThemeToken.h"
#include "Core/Icon.h"
#include "Core/Animator.h"
#include "Core/EditorConfigPaths.h"

#include <algorithm>
#include <cmath>
#include <fstream>

#include <SDL3/SDL.h>

namespace we::programs::editor {

namespace {
constexpr float kDragThreshold = 6.0f;

std::string CategoryStatePath() {
    return we::core::ResolveEditorConfigPath("place_actors.ini").string();
}

} // namespace

using WindEffects::Editor::UI::Color;
using WindEffects::Editor::UI::KeyEvent;
using WindEffects::Editor::UI::MouseButton;
using WindEffects::Editor::UI::MouseEvent;
using WindEffects::Editor::UI::PaintContext;
using WindEffects::Editor::UI::Point;
using WindEffects::Editor::UI::Rect;
using WindEffects::Editor::UI::Size;
using WindEffects::Editor::UI::ThemeToken;

PlaceActorsPanel::PlaceActorsPanel() {
    auto& config = PlaceActorsConfig::Get();
    config.EnsureLoaded();
    m_ViewMode = config.defaultView;

    m_SearchBox = std::make_shared<WindEffects::Editor::UI::SearchBox>();
    m_SearchBox->SetFillWidth(true);
    m_SearchBox->SetPlaceholder("Search Actors...");
    m_SearchBox->SetOnTextChanged([this](const std::string& text) {
        m_SearchText = text;
        m_NeedsLayout = true;
    });

    m_CategoryFilterButton = std::make_shared<WindEffects::Editor::UI::ToolButton>(
        WindEffects::Editor::UI::Icons::LayersName, "All", [this]() {
            const auto labels = PlaceActorsCatalog::Get().GetCategoryFilterLabels();
            auto it = std::find(labels.begin(), labels.end(), m_CategoryFilter);
            const size_t next = (it == labels.end()) ? 0 : static_cast<size_t>((it - labels.begin() + 1) % labels.size());
            m_CategoryFilter = labels[next];
            m_CategoryFilterButton->SetLabel(m_CategoryFilter);
            m_NeedsLayout = true;
        }, "Filter category");
    m_CategoryFilterButton->SetButtonStyle(WindEffects::Editor::UI::ToolButtonStyle::ToolbarInline);

    m_SortButton = std::make_shared<WindEffects::Editor::UI::ToolButton>(
        WindEffects::Editor::UI::Icons::ListName, "Sort", [this]() {
            switch (m_SortMode) {
            case PlaceActorsSortMode::Name: m_SortMode = PlaceActorsSortMode::Category; break;
            case PlaceActorsSortMode::Category: m_SortMode = PlaceActorsSortMode::Recent; break;
            default: m_SortMode = PlaceActorsSortMode::Name; break;
            }
            m_NeedsLayout = true;
        }, "Sort");
    m_SortButton->SetButtonStyle(WindEffects::Editor::UI::ToolButtonStyle::ToolbarInline);

    m_ViewToggleButton = std::make_shared<WindEffects::Editor::UI::ToolButton>(
        WindEffects::Editor::UI::Icons::GridName, "Grid", [this]() {
            m_ViewMode = (m_ViewMode == PlaceActorsViewMode::Grid)
                ? PlaceActorsViewMode::List
                : PlaceActorsViewMode::Grid;
            m_ViewToggleButton->SetLabel(m_ViewMode == PlaceActorsViewMode::Grid ? "Grid" : "List");
            m_NeedsLayout = true;
        }, "Toggle view");
    m_ViewToggleButton->SetButtonStyle(WindEffects::Editor::UI::ToolButtonStyle::ToolbarInline);

    m_RecentButton = std::make_shared<WindEffects::Editor::UI::ToolButton>(
        WindEffects::Editor::UI::Icons::RefreshName, "Recent", [this]() {
            m_ShowRecentOnly = !m_ShowRecentOnly;
            m_NeedsLayout = true;
        }, "Recently used");
    m_RecentButton->SetButtonStyle(WindEffects::Editor::UI::ToolButtonStyle::ToolbarInline);

    PlaceActorsCatalog::Get().Refresh();
    RebuildData();
}

PlaceActorsPanel::~PlaceActorsPanel() {
    SaveCategoryState();
}

void PlaceActorsPanel::RebuildData() {
    auto& catalog = PlaceActorsCatalog::Get();
    if (!catalog.GetCategories().empty()) {
        catalog.Refresh();
    }

    m_DisplayCategories.clear();
    auto& registry = EditorToolsRegistry::Get();
    const auto& config = PlaceActorsConfig::Get();

    std::vector<PlaceActorsItemData> recentItems;
    if (config.showRecent && m_ShowRecentOnly) {
        for (const std::string& toolId : registry.GetRecentToolIds()) {
            if (const PlaceActorsItemData* item = catalog.FindItem(toolId)) {
                recentItems.push_back(*item);
            }
        }
        if (!recentItems.empty()) {
            PlaceActorsCategoryData recentCategory;
            recentCategory.id = "__Recent";
            recentCategory.label = "Recently Used";
            recentCategory.iconName = WindEffects::Editor::UI::Icons::RefreshName;
            recentCategory.defaultExpanded = true;
            recentCategory.items = std::move(recentItems);
            m_DisplayCategories.push_back(std::move(recentCategory));
        }
    }

    if (config.showFavorites) {
        std::vector<PlaceActorsItemData> favoriteItems;
        for (const EditorToolAction* tool : registry.GetFavoriteTools("Actors")) {
            if (const PlaceActorsItemData* item = catalog.FindItem(tool->id)) {
                favoriteItems.push_back(*item);
            }
        }
        if (!favoriteItems.empty()) {
            PlaceActorsCategoryData favCategory;
            favCategory.id = "__Favorites";
            favCategory.label = "Favorites";
            favCategory.iconName = WindEffects::Editor::UI::Icons::StarFilledName;
            favCategory.defaultExpanded = true;
            favCategory.items = std::move(favoriteItems);
            m_DisplayCategories.push_back(std::move(favCategory));
        }
    }

    const std::string query = !m_ExternalSearchFilter.empty() ? m_ExternalSearchFilter : m_SearchText;
    for (const auto& sourceCategory : catalog.GetCategories()) {
        auto items = PlaceActorsSearch::FilterItems(sourceCategory.items, query, m_CategoryFilter);
        PlaceActorsSearch::SortItems(items, m_SortMode);
        if (items.empty()) {
            continue;
        }

        PlaceActorsCategoryData category = sourceCategory;
        category.items = std::move(items);
        if (m_CategoryExpanded.find(category.id) == m_CategoryExpanded.end()) {
            m_CategoryExpanded[category.id] = category.defaultExpanded;
        }
        m_DisplayCategories.push_back(std::move(category));
    }

    m_NeedsLayout = true;
}

void PlaceActorsPanel::RebuildLayout() {
    m_Layout.clear();
    m_ContentHeight = 0.0f;
    const float pad = ThemeMetric(ThemeToken::Space2);
    const auto& config = PlaceActorsConfig::Get();
    PlaceActorsItemMetrics metrics;
    metrics.iconSize = config.iconSize;
    metrics.cardSize = config.cardSize;
    metrics.listRowHeight = config.listRowHeight;

    float y = m_ContentRect.y - m_ScrollOffset;
    const float width = m_ContentRect.width;

    for (const auto& category : m_DisplayCategories) {
        const bool expanded = m_CategoryExpanded[category.id];
        const float headerHeight = PlaceActorsCategory::MeasureHeaderHeight(config.categoryHeaderHeight);
        LayoutEntry header;
        header.type = LayoutEntry::Type::CategoryHeader;
        header.categoryId = category.id;
        header.geometry = Rect{ m_ContentRect.x, y, width, headerHeight };
        m_Layout.push_back(header);
        y += headerHeight;
        m_ContentHeight += headerHeight;

        if (!expanded) {
            continue;
        }

        if (m_ViewMode == PlaceActorsViewMode::Grid) {
            const float cell = metrics.cardSize + pad;
            const int columns = std::max(1, static_cast<int>((width - pad * 2.0f) / cell));
            float x = m_ContentRect.x + pad;
            int column = 0;
            float rowHeight = metrics.cardSize + pad;
            for (const auto& item : category.items) {
                LayoutEntry entry;
                entry.type = LayoutEntry::Type::Item;
                entry.categoryId = category.id;
                entry.toolId = item.toolId;
                entry.geometry = Rect{ x, y, metrics.cardSize, metrics.cardSize };
                m_Layout.push_back(entry);

                ++column;
                x += metrics.cardSize + pad;
                if (column >= columns) {
                    column = 0;
                    x = m_ContentRect.x + pad;
                    y += rowHeight;
                    m_ContentHeight += rowHeight;
                }
            }
            if (column != 0) {
                y += rowHeight;
                m_ContentHeight += rowHeight;
            }
        } else {
            for (const auto& item : category.items) {
                LayoutEntry entry;
                entry.type = LayoutEntry::Type::Item;
                entry.categoryId = category.id;
                entry.toolId = item.toolId;
                entry.geometry = Rect{ m_ContentRect.x + pad, y, width - pad * 2.0f, metrics.listRowHeight };
                m_Layout.push_back(entry);
                y += metrics.listRowHeight + ThemeMetric(ThemeToken::Space1);
                m_ContentHeight += metrics.listRowHeight + ThemeMetric(ThemeToken::Space1);
            }
        }
    }

    const float maxScroll = std::max(0.0f, m_ContentHeight - m_ContentRect.height);
    m_ScrollOffset = std::clamp(m_ScrollOffset, 0.0f, maxScroll);
    m_NeedsLayout = false;
}

void PlaceActorsPanel::SaveCategoryState() const {
    if (!PlaceActorsConfig::Get().rememberCategoryState) {
        return;
    }
    std::ofstream file(CategoryStatePath());
    if (!file.is_open()) {
        return;
    }
    for (const auto& [categoryId, expanded] : m_CategoryExpanded) {
        file << "category." << categoryId << "=" << (expanded ? "1" : "0") << "\n";
    }
}

Size PlaceActorsPanel::Measure(const Size& availableSize) {
    m_DesiredSize = availableSize;
    return m_DesiredSize;
}

void PlaceActorsPanel::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
    m_ToolbarRect = Rect{ allottedRect.x, allottedRect.y, allottedRect.width, ThemeMetric(ThemeToken::ToolbarHeight) };
    m_ContentRect = Rect{
        allottedRect.x,
        allottedRect.y + ThemeMetric(ThemeToken::ToolbarHeight),
        allottedRect.width,
        std::max(0.0f, allottedRect.height - ThemeMetric(ThemeToken::ToolbarHeight))
    };

    float x = m_ToolbarRect.x + ThemeMetric(ThemeToken::Space2);
    const float buttonWidth = 64.0f;
    const float searchWidth = std::max(120.0f, m_ToolbarRect.width - buttonWidth * 4.0f - ThemeMetric(ThemeToken::Space2) * 6.0f);

    m_SearchBox->Measure(Size{ searchWidth, ThemeMetric(ThemeToken::SearchBoxHeight) });
    m_SearchBox->Arrange(Rect{ x, m_ToolbarRect.y + ThemeMetric(ThemeToken::Space1), searchWidth, ThemeMetric(ThemeToken::SearchBoxHeight) });
    x += searchWidth + ThemeMetric(ThemeToken::Space2);

    auto placeButton = [&](const std::shared_ptr<WindEffects::Editor::UI::ToolButton>& button) {
        button->Measure(Size{ buttonWidth, ThemeMetric(ThemeToken::SearchBoxHeight) });
        button->Arrange(Rect{ x, m_ToolbarRect.y + ThemeMetric(ThemeToken::Space1), buttonWidth, ThemeMetric(ThemeToken::SearchBoxHeight) });
        x += buttonWidth + ThemeMetric(ThemeToken::Space1);
    };
    placeButton(m_CategoryFilterButton);
    placeButton(m_SortButton);
    placeButton(m_ViewToggleButton);
    placeButton(m_RecentButton);

    if (m_NeedsLayout) {
        RebuildData();
        RebuildLayout();
    }
}

void PlaceActorsPanel::Tick(float deltaTime) {
    WindEffects::Editor::UI::Animator::Tick(deltaTime);
}

void PlaceActorsPanel::Paint(PaintContext& context) {
    context.DrawRect(m_Geometry, ThemeColor(ThemeToken::PanelBackground));
    context.DrawRect(m_ToolbarRect, ThemeColor(ThemeToken::HeaderBackground));
    context.DrawRect(Rect{ m_ToolbarRect.x, m_ToolbarRect.y + m_ToolbarRect.height - ThemeMetric(ThemeToken::BorderWidth), m_ToolbarRect.width, ThemeMetric(ThemeToken::BorderWidth) }, ThemeColor(ThemeToken::Separator));

    m_SearchBox->Paint(context);
    m_CategoryFilterButton->Paint(context);
    m_SortButton->Paint(context);
    m_ViewToggleButton->Paint(context);
    m_RecentButton->Paint(context);

    context.DrawRect(m_ContentRect, ThemeColor(ThemeToken::PanelBackground));

    auto& catalog = PlaceActorsCatalog::Get();
    auto& registry = EditorToolsRegistry::Get();
    const auto& config = PlaceActorsConfig::Get();
    PlaceActorsItemMetrics metrics;
    metrics.iconSize = config.iconSize;
    metrics.cardSize = config.cardSize;
    metrics.listRowHeight = config.listRowHeight;

    const Rect clip = m_ContentRect;
    for (auto& entry : m_Layout) {
        if (entry.geometry.y + entry.geometry.height < clip.y) continue;
        if (entry.geometry.y > clip.y + clip.height) break;

        if (entry.type == LayoutEntry::Type::CategoryHeader) {
            const PlaceActorsCategoryData* category = nullptr;
            for (const auto& candidate : m_DisplayCategories) {
                if (candidate.id == entry.categoryId) {
                    category = &candidate;
                    break;
                }
            }
            if (!category) continue;
            const bool expanded = m_CategoryExpanded[entry.categoryId];
            PlaceActorsCategory::PaintHeader(context, entry.geometry, category->label, category->iconName, expanded, entry.hoverAnim);
            continue;
        }

        const PlaceActorsItemData* item = catalog.FindItem(entry.toolId);
        if (!item) continue;
        const bool favorite = registry.IsFavorite(item->toolId);
        if (m_ViewMode == PlaceActorsViewMode::Grid) {
            PlaceActorsItem::PaintGrid(context, entry.geometry, *item, metrics, entry.hoverAnim, entry.pressAnim, entry.selected, favorite);
        } else {
            PlaceActorsItem::PaintList(context, entry.geometry, *item, metrics, entry.hoverAnim, entry.pressAnim, entry.selected, favorite);
        }
    }

    if (!m_TooltipText.empty() && m_TooltipRect.width > 0.0f) {
        context.DrawShadow(m_TooltipRect, Color{ 0, 0, 0, 0.2f }, 4.0f, 8.0f);
        context.DrawRoundedRect(m_TooltipRect, Color{ 0.18f, 0.18f, 0.18f, 1.0f }, 6.0f);
        context.DrawText(m_TooltipText, Point{ m_TooltipRect.x + 8.0f, m_TooltipRect.y + 6.0f }, ThemeColor(ThemeToken::TextPrimary), 11.0f);
    }

    if (m_ContextMenuOpen) {
        context.DrawShadow(m_ContextMenuRect, Color{ 0, 0, 0, 0.2f }, 4.0f, 10.0f);
        context.DrawRoundedRect(m_ContextMenuRect, ThemeColor(ThemeToken::PanelBackground), 6.0f);
        for (size_t i = 0; i < m_ContextMenuItems.size(); ++i) {
            if (static_cast<int>(i) == m_ContextMenuHovered) {
                context.DrawRect(m_ContextMenuItems[i].geometry, ThemeColor(ThemeToken::HoverBackground));
            }
            context.DrawText(m_ContextMenuItems[i].label,
                Point{ m_ContextMenuItems[i].geometry.x + 10.0f, m_ContextMenuItems[i].geometry.y + 6.0f },
                ThemeColor(ThemeToken::TextPrimary), 11.0f);
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
    EditorToolsRegistry::Get().ToggleFavorite(toolId);
    RebuildData();
}

void PlaceActorsPanel::OpenContextMenu(const std::string& toolId, const Point& position) {
    m_ContextMenuOpen = true;
    m_ContextMenuItems.clear();
    constexpr float itemHeight = 26.0f;
    float y = position.y;
    auto addItem = [&](const std::string& label, auto action) {
        ContextMenuItem item;
        item.label = label;
        item.geometry = Rect{ position.x, y, 160.0f, itemHeight };
        item.action = std::move(action);
        m_ContextMenuItems.push_back(std::move(item));
        y += itemHeight;
    };

    addItem("Place in Level", [this, toolId]() { SpawnItem(toolId); });
    addItem("Drag to Viewport", [toolId]() { PlaceActorsPlacement::Get().BeginDragPlacement(toolId); });
    addItem("Toggle Favorite", [this, toolId]() { ToggleFavorite(toolId); });
    m_ContextMenuRect = Rect{ position.x, position.y, 160.0f, itemHeight * static_cast<float>(m_ContextMenuItems.size()) };
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

void PlaceActorsPanel::OnMouseDown(const MouseEvent& event) {
    if (m_ContextMenuOpen) {
        if (m_ContextMenuRect.Contains(event.position)) {
            if (m_ContextMenuHovered >= 0 && m_ContextMenuHovered < static_cast<int>(m_ContextMenuItems.size())) {
                m_ContextMenuItems[m_ContextMenuHovered].action();
            }
            CloseContextMenu();
            return;
        }
        CloseContextMenu();
        return;
    }

    if (m_ToolbarRect.Contains(event.position)) {
        if (m_SearchBox->GetGeometry().Contains(event.position)) m_SearchBox->OnMouseDown(event);
        else if (m_CategoryFilterButton->GetGeometry().Contains(event.position)) m_CategoryFilterButton->OnMouseDown(event);
        else if (m_SortButton->GetGeometry().Contains(event.position)) m_SortButton->OnMouseDown(event);
        else if (m_ViewToggleButton->GetGeometry().Contains(event.position)) m_ViewToggleButton->OnMouseDown(event);
        else if (m_RecentButton->GetGeometry().Contains(event.position)) m_RecentButton->OnMouseDown(event);
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
            m_CategoryExpanded[entry->categoryId] = !m_CategoryExpanded[entry->categoryId];
            SaveCategoryState();
            m_NeedsLayout = true;
            RebuildLayout();
            return;
        }

        entry->pressAnim = 1.0f;
        for (auto& e : m_Layout) {
            e.selected = (&e == entry);
        }
        m_FocusedIndex = static_cast<int>(std::distance(m_Layout.data(), entry));

        if (const PlaceActorsItemData* item = PlaceActorsCatalog::Get().FindItem(entry->toolId)) {
            m_PendingDragItem = item;
            m_DragStartPosition = event.position;
            m_DragStarted = false;
        }
    }
}

void PlaceActorsPanel::OnMouseMove(const MouseEvent& event) {
    if (m_ToolbarRect.Contains(event.position)) {
        m_SearchBox->OnMouseMove(event);
        m_CategoryFilterButton->OnMouseMove(event);
        m_SortButton->OnMouseMove(event);
        m_ViewToggleButton->OnMouseMove(event);
        m_RecentButton->OnMouseMove(event);
    }

    HideTooltip();
    for (auto& entry : m_Layout) {
        const bool hovered = entry.geometry.Contains(event.position);
        if (PlaceActorsConfig::Get().enableAnimations) {
            entry.hoverAnim = WindEffects::Editor::UI::Animator::Damp(entry.hoverAnim, hovered ? 1.0f : 0.0f, 14.0f);
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
}

void PlaceActorsPanel::OnMouseUp(const MouseEvent& event) {
    if (m_PendingDragItem && !m_DragStarted && event.button == MouseButton::Left) {
        SpawnItem(m_PendingDragItem->toolId);
    }
    m_PendingDragItem = nullptr;
    m_DragStarted = false;
}

void PlaceActorsPanel::OnMouseWheel(const MouseEvent& event) {
    if (!m_ContentRect.Contains(event.position)) {
        return;
    }
    m_ScrollOffset -= event.wheelDeltaY * 24.0f;
    m_NeedsLayout = true;
    RebuildLayout();
}

void PlaceActorsPanel::OnKeyDown(const KeyEvent& event) {
    if (event.keycode == SDLK_RETURN && m_FocusedIndex >= 0 && m_FocusedIndex < static_cast<int>(m_Layout.size())) {
        const auto& entry = m_Layout[m_FocusedIndex];
        if (entry.type == LayoutEntry::Type::Item) {
            SpawnItem(entry.toolId);
        }
    }
}

} // namespace we::programs::editor
