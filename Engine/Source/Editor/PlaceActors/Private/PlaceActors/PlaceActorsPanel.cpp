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

#include "EditorToolsRegistry.h"
#include "Widgets/SearchBox.h"
#include "Widgets/ToolButton.h"
#include "WindEffects/Editor/UI/Panel/PanelChrome.h"
#include "Core/PaintContext.h"
#include "Core/DPIContext.h"
#include "WindEffects/Editor/UI/Theming/ThemeToken.h"
#include "Core/Icon.h"
#include "Core/Animator.h"
#include "Core/EditorConfigPaths.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>

#include <SDL3/SDL.h>

namespace we::programs::editor {

namespace {
constexpr float kDragThreshold = 6.0f;
constexpr const char* kFavoritesCategoryId = "__Favorites";
constexpr const char* kRecentCategoryId = "__Recent";
constexpr const char* kQuickAccessCategoryId = "__QuickAccess";

const std::vector<std::string>& QuickAccessToolIds() {
    static const std::vector<std::string> kIds = {
        "PlaceEmptyActor",
        "PlaceEmptyCharacter",
        "PlaceEmptyPawn",
        "PlaceCube",
        "PlaceSphere",
        "PlaceCylinder",
        "PlacePlane",
        "PlaceCone",
        "PlaceCapsule",
        "PlaceCamera",
    };
    return kIds;
}

std::string PanelStatePath() {
    return we::core::ResolveEditorConfigPath("place_actors.ini").string();
}

std::unordered_map<std::string, std::string> LoadIniValues(const std::string& path) {
    std::unordered_map<std::string, std::string> values;
    std::ifstream file(path);
    if (!file.is_open()) {
        return values;
    }
    std::string line;
    while (std::getline(file, line)) {
        const size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        values[line.substr(0, eq)] = line.substr(eq + 1);
    }
    return values;
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
namespace PanelChrome = WindEffects::Editor::UI::PanelChrome;

PlaceActorsPanel::PlaceActorsPanel() {
    auto& config = PlaceActorsConfig::Get();
    config.EnsureLoaded();
    m_ViewMode = config.defaultView;

    m_SearchBox = std::make_shared<WindEffects::Editor::UI::SearchBox>();
    m_SearchBox->SetFillWidth(true);
    m_SearchBox->SetPlaceholder("Search Assets...");
    m_SearchBox->SetOnTextChanged([this](const std::string& text) {
        m_SearchText = text;
        RefreshFilteredContent();
    });

    m_FilterButton = std::make_shared<WindEffects::Editor::UI::ToolButton>(
        WindEffects::Editor::UI::Icons::FilterName, "", [this]() {
            const Rect btn = m_FilterButton->GetGeometry();
            ToggleFilterMenu(Point{ btn.x, btn.y + btn.height });
        }, "Filter and view options");
    m_FilterButton->SetButtonStyle(WindEffects::Editor::UI::ToolButtonStyle::ToolbarIconOnly);

    LoadPanelState();
    PlaceActorsCatalog::Get().Refresh();
    RebuildData();
}

PlaceActorsPanel::~PlaceActorsPanel() {
    SaveCategoryState();
    SaveFavorites();
}

PlaceActorsGridMetrics PlaceActorsPanel::MakeGridMetrics() const {
    PlaceActorsGridMetrics metrics;
    metrics.minCardWidth = ActorsPanelLayout::GridMinCardWidth();
    metrics.maxCardWidth = ActorsPanelLayout::GridMaxCardWidth();
    metrics.cardGap = ActorsPanelLayout::GridCardGap();
    metrics.contentPad = ActorsPanelLayout::ContentPadH();
    metrics.scrollbarReserve = ActorsPanelLayout::GridScrollbarReserve();
    metrics.labelHeight = ActorsPanelLayout::GridLabelHeight();
    metrics.labelGap = ActorsPanelLayout::GridLabelGap();
    return metrics;
}

bool PlaceActorsPanel::IsFavoritesCategory(const std::string& categoryId) const {
    return categoryId == kFavoritesCategoryId;
}

bool PlaceActorsPanel::IsQuickAccessCategory(const std::string& categoryId) const {
    return categoryId == kQuickAccessCategoryId;
}

bool PlaceActorsPanel::IsPinnedCategory(const std::string& categoryId) const {
    return IsQuickAccessCategory(categoryId);
}

void PlaceActorsPanel::BuildQuickAccessCategory(const std::string& query) {
    auto& catalog = PlaceActorsCatalog::Get();
    std::vector<PlaceActorsItemData> items;
    items.reserve(QuickAccessToolIds().size());
    for (const std::string& toolId : QuickAccessToolIds()) {
        if (const PlaceActorsItemData* item = catalog.FindItem(toolId)) {
            if (PlaceActorsSearch::FilterItems({ *item }, query, "All").empty()) {
                continue;
            }
            items.push_back(*item);
        }
    }
    if (items.empty() && !query.empty()) {
        return;
    }

    PlaceActorsCategoryData quick;
    quick.id = kQuickAccessCategoryId;
    quick.label = "Quick Access";
    quick.iconName = WindEffects::Editor::UI::Icons::PivotName;
    quick.defaultExpanded = true;
    quick.items = std::move(items);
    m_CategoryExpanded[quick.id] = true;
    m_CategoryExpandAnim[quick.id] = 1.0f;
    m_DisplayCategories.push_back(std::move(quick));
}

void PlaceActorsPanel::LoadPanelState() {
    const auto values = LoadIniValues(PanelStatePath());
    for (const auto& [key, value] : values) {
        if (key.rfind("category.", 0) == 0) {
            m_CategoryExpanded[key.substr(9)] = (value == "1");
        }
    }

    std::unordered_map<std::string, bool> favorites;
    for (const auto& [key, value] : values) {
        if (key.rfind("favorite.", 0) == 0) {
            favorites[key.substr(9)] = (value == "1");
        }
    }
    if (!favorites.empty()) {
        PlaceActorsFavoritesManager::Get().Load(favorites);
    }
}

void PlaceActorsPanel::SaveCategoryState() const {
    if (!PlaceActorsConfig::Get().rememberCategoryState) {
        return;
    }
    const auto existing = LoadIniValues(PanelStatePath());
    std::ofstream file(PanelStatePath());
    if (!file.is_open()) {
        return;
    }
    for (const auto& [key, value] : existing) {
        if (key.rfind("category.", 0) != 0 && key.rfind("favorite.", 0) != 0) {
            file << key << "=" << value << "\n";
        }
    }
    for (const auto& [categoryId, expanded] : m_CategoryExpanded) {
        file << "category." << categoryId << "=" << (expanded ? "1" : "0") << "\n";
    }
    for (const auto& [toolId, enabled] : PlaceActorsFavoritesManager::Get().GetStates()) {
        if (enabled) {
            file << "favorite." << toolId << "=1\n";
        }
    }
}

void PlaceActorsPanel::SaveFavorites() const {
    SaveCategoryState();
}

float PlaceActorsPanel::CategoryExpandAnim(const std::string& categoryId) const {
    if (IsPinnedCategory(categoryId)) {
        return 1.0f;
    }
    const auto it = m_CategoryExpandAnim.find(categoryId);
    return it != m_CategoryExpandAnim.end() ? it->second : (m_CategoryExpanded.count(categoryId) ? (m_CategoryExpanded.at(categoryId) ? 1.0f : 0.0f) : 1.0f);
}

void PlaceActorsPanel::RebuildData() {
    auto& catalog = PlaceActorsCatalog::Get();
    if (!catalog.GetCategories().empty()) {
        catalog.Refresh();
    }

    m_DisplayCategories.clear();
    const auto& config = PlaceActorsConfig::Get();
    const std::string query = !m_ExternalSearchFilter.empty() ? m_ExternalSearchFilter : m_SearchText;

    BuildQuickAccessCategory(query);

    if (config.showFavorites) {
        auto favoriteItems = PlaceActorsFavoritesManager::Get().CollectFavoriteItems(query);
        PlaceActorsSearch::SortItems(favoriteItems, m_SortMode);

        PlaceActorsCategoryData favCategory;
        favCategory.id = kFavoritesCategoryId;
        favCategory.label = "Favorites";
        favCategory.iconName = WindEffects::Editor::UI::Icons::StarName;
        favCategory.defaultExpanded = true;
        favCategory.items = std::move(favoriteItems);
        if (m_CategoryExpanded.find(favCategory.id) == m_CategoryExpanded.end()) {
            m_CategoryExpanded[favCategory.id] = favCategory.defaultExpanded;
        }
        m_DisplayCategories.push_back(std::move(favCategory));
    }

    if (config.showRecent && m_ShowRecentOnly) {
        auto recentItems = PlaceActorsRecentlyUsedManager::Get().CollectRecentItems(query);
        if (!recentItems.empty()) {
            PlaceActorsCategoryData recentCategory;
            recentCategory.id = kRecentCategoryId;
            recentCategory.label = "Recently Used";
            recentCategory.iconName = WindEffects::Editor::UI::Icons::RefreshName;
            recentCategory.defaultExpanded = true;
            recentCategory.items = std::move(recentItems);
            if (m_CategoryExpanded.find(recentCategory.id) == m_CategoryExpanded.end()) {
                m_CategoryExpanded[recentCategory.id] = true;
            }
            m_DisplayCategories.push_back(std::move(recentCategory));
        }
    }

    for (const auto& sourceCategory : catalog.GetCategories()) {
        if (sourceCategory.id == "ActorBasic") {
            continue;
        }
        auto items = PlaceActorsSearch::FilterItems(sourceCategory.items, query, m_CategoryFilter);
        PlaceActorsSearch::SortItems(items, m_SortMode);
        if (items.empty()) {
            continue;
        }

        PlaceActorsCategoryData category = sourceCategory;
        category.items = std::move(items);
        if (m_CategoryExpanded.find(category.id) == m_CategoryExpanded.end()) {
            m_CategoryExpanded[category.id] = false;
        }
        m_DisplayCategories.push_back(std::move(category));
    }

    m_HasSearchResults = false;
    for (const auto& category : m_DisplayCategories) {
        if (!category.items.empty() || IsQuickAccessCategory(category.id) || IsFavoritesCategory(category.id)) {
            m_HasSearchResults = true;
            break;
        }
    }
    if (!query.empty() && !m_HasSearchResults) {
        m_DisplayCategories.clear();
    } else if (m_DisplayCategories.empty() && query.empty()) {
        m_HasSearchResults = false;
    } else {
        m_HasSearchResults = true;
    }

    m_NeedsLayout = true;
}

void PlaceActorsPanel::SyncScrollMetrics() {
    m_Scroll.Sync(m_ContentRect.height, m_ContentHeight);
    const float uiScale = std::max(1.0f, WindEffects::Editor::UI::DPIContext::GetScale());
    m_ScrollMetrics = m_Scroll.ComputeMetrics(m_ContentRect, m_ContentHeight, uiScale);
}

void PlaceActorsPanel::UpdateVisibleRange() {
    m_FirstVisibleIndex = 0;
    m_LastVisibleIndex = static_cast<int>(m_Layout.size()) - 1;
    if (m_Layout.empty()) {
        return;
    }

    const float viewTop = m_ScrollMetrics.viewport.y;
    const float viewBottom = viewTop + m_ScrollMetrics.viewport.height;

    m_FirstVisibleIndex = static_cast<int>(m_Layout.size());
    m_LastVisibleIndex = -1;
    for (int i = 0; i < static_cast<int>(m_Layout.size()); ++i) {
        const auto& entry = m_Layout[static_cast<size_t>(i)];
        if (entry.geometry.y + entry.geometry.height < viewTop) {
            continue;
        }
        if (entry.geometry.y > viewBottom) {
            break;
        }
        m_FirstVisibleIndex = std::min(m_FirstVisibleIndex, i);
        m_LastVisibleIndex = i;
    }
    if (m_LastVisibleIndex < 0) {
        m_FirstVisibleIndex = 0;
        m_LastVisibleIndex = -1;
    }
}

void PlaceActorsPanel::RebuildLayout() {
    m_Layout.clear();
    m_SectionBackgrounds.clear();
    m_ContentHeight = 0.0f;

    const auto& config = PlaceActorsConfig::Get();
    PlaceActorsItemMetrics metrics;
    metrics.iconSize = ActorsPanelLayout::IconSize();
    metrics.cardSize = config.cardSize;
    metrics.listRowHeight = ActorsPanelLayout::ActorRowHeight();
    metrics.cornerRadius = ThemeMetric(ThemeToken::CornerRadiusSmall);

    const float viewportX = m_ScrollMetrics.viewport.x;
    const float viewportWidth = m_ScrollMetrics.viewport.width;
    float y = m_ScrollMetrics.viewport.y - m_Scroll.offset;
    bool firstCategory = true;

    const PlaceActorsGridLayout gridLayout =
        PlaceActorsResponsiveGrid::Compute(viewportWidth, MakeGridMetrics());
    metrics.cardSize = gridLayout.cardWidth;
    metrics.previewSize = gridLayout.previewSize;
    metrics.cardHeight = gridLayout.cardHeight;

    if (!m_HasSearchResults) {
        const float emptyHeight = ActorsPanelLayout::ActorRowHeight() * 3.0f;
        LayoutEntry emptyEntry;
        emptyEntry.type = LayoutEntry::Type::EmptyState;
        emptyEntry.geometry = Rect{ viewportX, y, viewportWidth, emptyHeight };
        m_Layout.push_back(emptyEntry);
        m_ContentHeight = emptyHeight;
        SyncScrollMetrics();
        UpdateVisibleRange();
        m_NeedsLayout = false;
        m_LastViewportWidth = viewportWidth;
        return;
    }

    for (const auto& category : m_DisplayCategories) {
        if (!firstCategory) {
            y += ActorsPanelLayout::CategoryGap();
            m_ContentHeight += ActorsPanelLayout::CategoryGap();
        }
        firstCategory = false;

        const float expandAnim = CategoryExpandAnim(category.id);
        const bool expanded = expandAnim > 0.01f || IsPinnedCategory(category.id);
        const float headerHeight = ActorsPanelLayout::CategoryHeight();

        LayoutEntry header;
        header.type = LayoutEntry::Type::CategoryHeader;
        header.categoryId = category.id;
        header.geometry = Rect{ viewportX, y, viewportWidth, headerHeight };
        header.revealAnim = 1.0f;
        m_Layout.push_back(header);
        y += headerHeight;
        m_ContentHeight += headerHeight;

        if (!expanded) {
            continue;
        }

        y += ActorsPanelLayout::CategoryContentGap();
        m_ContentHeight += ActorsPanelLayout::CategoryContentGap();

        const float sectionStartY = y;

        if (category.items.empty() && IsFavoritesCategory(category.id)) {
            const float emptyHeight = metrics.listRowHeight * 1.5f;
            LayoutEntry emptyEntry;
            emptyEntry.type = LayoutEntry::Type::EmptyState;
            emptyEntry.categoryId = category.id;
            emptyEntry.geometry = Rect{
                viewportX + ActorsPanelLayout::ItemIndent(),
                y,
                viewportWidth - ActorsPanelLayout::ItemIndent(),
                emptyHeight
            };
            emptyEntry.revealAnim = expandAnim;
            m_Layout.push_back(emptyEntry);
            y += emptyHeight * expandAnim;
            m_ContentHeight += emptyHeight * expandAnim;
        } else if (m_ViewMode == PlaceActorsViewMode::Grid) {
            const float sectionOriginY = y;
            for (int i = 0; i < static_cast<int>(category.items.size()); ++i) {
                const auto& item = category.items[static_cast<size_t>(i)];
                const Rect card = PlaceActorsResponsiveGrid::CardRect(
                    gridLayout, viewportX, sectionOriginY, i);

                LayoutEntry entry;
                entry.type = LayoutEntry::Type::Item;
                entry.categoryId = category.id;
                entry.toolId = item.toolId;
                entry.geometry = card;
                entry.previewGeometry = PlaceActorsResponsiveGrid::PreviewRect(gridLayout, card);
                entry.revealAnim = expandAnim;
                entry.gridColumn = i % gridLayout.columns;
                entry.gridColumns = gridLayout.columns;
                m_Layout.push_back(entry);
            }

            const float gridHeight =
                PlaceActorsResponsiveGrid::ContentHeight(gridLayout, static_cast<int>(category.items.size()));
            y += gridHeight * expandAnim;
            m_ContentHeight += gridHeight * expandAnim;
        } else {
            for (const auto& item : category.items) {
                LayoutEntry entry;
                entry.type = LayoutEntry::Type::Item;
                entry.categoryId = category.id;
                entry.toolId = item.toolId;
                entry.geometry = Rect{ viewportX, y, viewportWidth, metrics.listRowHeight };
                entry.previewGeometry = {};
                entry.revealAnim = expandAnim;
                m_Layout.push_back(entry);
                y += metrics.listRowHeight;
                m_ContentHeight += metrics.listRowHeight * expandAnim;
            }
        }

        if (!category.items.empty() || IsFavoritesCategory(category.id)) {
            const float sectionHeight = std::max(0.0f, y - sectionStartY);
            if (sectionHeight > 0.0f) {
                m_SectionBackgrounds.push_back(Rect{
                    viewportX + ActorsPanelLayout::ContentPadH() * 0.25f,
                    sectionStartY,
                    viewportWidth - ActorsPanelLayout::ContentPadH() * 0.5f,
                    sectionHeight
                });
            }
        }
    }

    m_ContentHeight += ActorsPanelLayout::ContentPadV();
    SyncScrollMetrics();
    UpdateVisibleRange();
    m_NeedsLayout = false;
    m_LastViewportWidth = viewportWidth;
}

void PlaceActorsPanel::RefreshFilteredContent() {
    RebuildData();
    if (m_ContentRect.width > 0.0f) {
        m_ScrollMetrics.viewport = m_ContentRect;
        RebuildLayout();
    }
}

void PlaceActorsPanel::ScrollFocusedIntoView() {
    if (m_FocusedIndex < 0 || m_FocusedIndex >= static_cast<int>(m_Layout.size())) {
        return;
    }
    const auto& entry = m_Layout[static_cast<size_t>(m_FocusedIndex)];
    const float top = entry.geometry.y + m_Scroll.offset - m_ScrollMetrics.viewport.y;
    const float bottom = top + entry.geometry.height;
    if (m_Scroll.ScrollToRange(top, bottom, m_ContentRect.height, m_ContentHeight)) {
        RebuildLayout();
    }
}

Size PlaceActorsPanel::Measure(const Size& availableSize) {
    m_DesiredSize = availableSize;
    return m_DesiredSize;
}

void PlaceActorsPanel::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;

    const float padH = ActorsPanelLayout::ContentPadH();
    const float searchRowH = ActorsPanelLayout::SearchRowHeight();
    const float filterBtn = ActorsPanelLayout::ToolbarIconSize();
    const float filterGap = ActorsPanelLayout::FilterButtonGap();

    m_SearchRowRect = Rect{ allottedRect.x, allottedRect.y, allottedRect.width, searchRowH };
    m_ContentRect = Rect{
        allottedRect.x,
        allottedRect.y + searchRowH,
        allottedRect.width,
        std::max(0.0f, allottedRect.height - searchRowH)
    };

    const float searchWidth = std::max(80.0f, m_SearchRowRect.width - padH * 2.0f - filterBtn - filterGap);
    const float searchY = m_SearchRowRect.y + (searchRowH - ActorsPanelLayout::SearchHeight()) * 0.5f;

    m_SearchBox->Measure(Size{ searchWidth, ActorsPanelLayout::SearchHeight() });
    m_SearchBox->Arrange(Rect{ m_SearchRowRect.x + padH, searchY, searchWidth, ActorsPanelLayout::SearchHeight() });

    const float filterX = m_SearchRowRect.x + m_SearchRowRect.width - padH - filterBtn;
    const float filterY = m_SearchRowRect.y + (searchRowH - filterBtn) * 0.5f;
    m_FilterButton->Measure(Size{ filterBtn, filterBtn });
    m_FilterButton->Arrange(Rect{ filterX, filterY, filterBtn, filterBtn });

    const bool widthChanged = std::abs(m_ContentRect.width - m_LastViewportWidth) > 0.5f;
    if (m_NeedsLayout || widthChanged) {
        m_ScrollMetrics.viewport = m_ContentRect;
        if (m_NeedsLayout) {
            RebuildData();
        }
        RebuildLayout();
    }
}

void PlaceActorsPanel::Tick(float deltaTime) {
    m_SearchBox->Tick(deltaTime);
    WindEffects::Editor::UI::Animator::Tick(deltaTime);
    const float expandSpeed = PlaceActorsConfig::Get().enableAnimations ? 16.0f : 1000.0f;
    const float pressSpeed = PlaceActorsConfig::Get().enableAnimations ? 18.0f : 1000.0f;

    bool expandChanged = false;
    for (const auto& category : m_DisplayCategories) {
        if (IsPinnedCategory(category.id)) {
            m_CategoryExpanded[category.id] = true;
            m_CategoryExpandAnim[category.id] = 1.0f;
            continue;
        }
        const float target = m_CategoryExpanded[category.id] ? 1.0f : 0.0f;
        float& anim = m_CategoryExpandAnim[category.id];
        const float previous = anim;
        anim = WindEffects::Editor::UI::Animator::Damp(anim, target, expandSpeed);
        if (std::abs(anim - previous) > 0.001f) {
            expandChanged = true;
        }
    }

    for (auto& entry : m_Layout) {
        entry.pressAnim = WindEffects::Editor::UI::Animator::Damp(entry.pressAnim, 0.0f, pressSpeed);
    }

    if (expandChanged) {
        RebuildLayout();
    }
}

void PlaceActorsPanel::Paint(PaintContext& context) {
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
    metrics.cornerRadius = ThemeMetric(ThemeToken::CornerRadiusSmall);

    // Warm thumbnail cache lazily for visible grid items.
    (void)PlaceActorsThumbnailProvider::Get();

    SyncScrollMetrics();
    UpdateVisibleRange();
    const std::string query = !m_ExternalSearchFilter.empty() ? m_ExternalSearchFilter : m_SearchText;
    const float textSize = ThemeMetric(ThemeToken::TextSizeSmall);
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
        context.DrawShadow(m_TooltipRect, ThemeColor(ThemeToken::ShadowPopup), 3.0f, 6.0f);
        context.DrawRoundedRect(m_TooltipRect, ThemeColor(ThemeToken::PopupBackground), ThemeMetric(ThemeToken::CornerRadiusSmall));
        context.DrawText(m_TooltipText,
            Point{ m_TooltipRect.x + ActorsPanelLayout::ContentPadH(), m_TooltipRect.y + (m_TooltipRect.height - textSize) * 0.5f },
            ThemeColor(ThemeToken::TextSecondary), textSize);
    }

    if (m_FilterMenuOpen) {
        context.DrawShadow(m_FilterMenuRect, ThemeColor(ThemeToken::ShadowPopup), 3.0f, 8.0f);
        context.DrawRoundedRect(m_FilterMenuRect, ThemeColor(ThemeToken::PopupBackground), ThemeMetric(ThemeToken::CornerRadiusSmall));
        for (size_t i = 0; i < m_FilterMenuItems.size(); ++i) {
            if (static_cast<int>(i) == m_FilterMenuHovered) {
                context.DrawRect(m_FilterMenuItems[i].geometry, ThemeColor(ThemeToken::HoverBackground));
            }
            const Color textColor = m_FilterMenuItems[i].checked
                ? ThemeColor(ThemeToken::AccentPrimary)
                : ThemeColor(ThemeToken::TextPrimary);
            context.DrawText(m_FilterMenuItems[i].label,
                Point{ m_FilterMenuItems[i].geometry.x + ActorsPanelLayout::ContentPadH(), m_FilterMenuItems[i].geometry.y + (m_FilterMenuItems[i].geometry.height - textSize) * 0.5f },
                textColor, textSize);
        }
    }

    if (m_ContextMenuOpen) {
        context.DrawShadow(m_ContextMenuRect, ThemeColor(ThemeToken::ShadowPopup), 3.0f, 8.0f);
        context.DrawRoundedRect(m_ContextMenuRect, ThemeColor(ThemeToken::PopupBackground), ThemeMetric(ThemeToken::CornerRadiusSmall));
        const float rowH = ActorsPanelLayout::ActorRowHeight();
        for (size_t i = 0; i < m_ContextMenuItems.size(); ++i) {
            if (static_cast<int>(i) == m_ContextMenuHovered) {
                context.DrawRect(m_ContextMenuItems[i].geometry, ThemeColor(ThemeToken::HoverBackground));
            }
            context.DrawText(m_ContextMenuItems[i].label,
                Point{ m_ContextMenuItems[i].geometry.x + ActorsPanelLayout::ContentPadH(), m_ContextMenuItems[i].geometry.y + (rowH - textSize) * 0.5f },
                ThemeColor(ThemeToken::TextPrimary), textSize);
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
        const double now = SDL_GetTicks() / 1000.0;
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

    if (event.keycode == SDLK_RETURN && m_FocusedIndex >= 0 && m_FocusedIndex < static_cast<int>(m_Layout.size())) {
        const auto& entry = m_Layout[static_cast<size_t>(m_FocusedIndex)];
        if (entry.type == LayoutEntry::Type::Item) {
            SpawnItem(entry.toolId);
        }
        return;
    }

    int step = 0;
    if (event.keycode == SDLK_DOWN) {
        step = (m_ViewMode == PlaceActorsViewMode::Grid && m_FocusedIndex >= 0
            && m_FocusedIndex < static_cast<int>(m_Layout.size()))
            ? std::max(1, m_Layout[static_cast<size_t>(m_FocusedIndex)].gridColumns)
            : 1;
    } else if (event.keycode == SDLK_UP) {
        step = (m_ViewMode == PlaceActorsViewMode::Grid && m_FocusedIndex >= 0
            && m_FocusedIndex < static_cast<int>(m_Layout.size()))
            ? -std::max(1, m_Layout[static_cast<size_t>(m_FocusedIndex)].gridColumns)
            : -1;
    } else if (event.keycode == SDLK_RIGHT) {
        step = 1;
    } else if (event.keycode == SDLK_LEFT) {
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
