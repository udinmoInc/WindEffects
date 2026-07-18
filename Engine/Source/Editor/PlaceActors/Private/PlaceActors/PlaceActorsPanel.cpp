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
using ::we::runtime::kindui::DPIContext;
using ::we::runtime::kindui::Animator;
namespace Icons = ::we::runtime::kindui::Icons;
namespace PanelChrome = ::we::editor::panels::PanelChrome;

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

using ::we::runtime::kindui::Color;
using ::we::runtime::kindui::KeyEvent;
using ::we::runtime::kindui::MouseButton;
using ::we::runtime::kindui::MouseEvent;
using ::we::runtime::kindui::PaintContext;
using ::we::runtime::kindui::Point;
using ::we::runtime::kindui::Rect;
using ::we::runtime::kindui::Size;
using ::we::runtime::kindui::ColorToken;
using ::we::runtime::kindui::MetricToken;
using ::we::runtime::kindui::PaddingToken;

PlaceActorsPanel::PlaceActorsPanel() {
    auto& config = PlaceActorsConfig::Get();
    config.EnsureLoaded();
    m_ViewMode = config.defaultView;

    m_SearchBox = std::make_shared<::we::editor::widgets::SearchBox>();
    m_SearchBox->SetFillWidth(true);
    m_SearchBox->SetPlaceholder("Search Assets...");
    m_SearchBox->SetOnTextChanged([this](const std::string& text) {
        m_SearchText = text;
        RefreshFilteredContent();
    });

    m_FilterButton = std::make_shared<::we::editor::toolbar::ToolButton>(
        we::runtime::kindui::Icons::FilterName, "", [this]() {
            const Rect btn = m_FilterButton->GetGeometry();
            ToggleFilterMenu(Point{ btn.x, btn.y + btn.height });
        }, "Filter and view options");
    m_FilterButton->SetButtonStyle(::we::editor::toolbar::ToolButtonStyle::ToolbarIconOnly);

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
    quick.iconName = we::runtime::kindui::Icons::PivotName;
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
        favCategory.iconName = we::runtime::kindui::Icons::StarName;
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
            recentCategory.iconName = we::runtime::kindui::Icons::RefreshName;
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
    const float uiScale = std::max(1.0f, we::runtime::kindui::DPIContext::GetScale());
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
    metrics.cornerRadius = ThemeMetric(MetricToken::CornerRadiusSmall);

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
    we::runtime::kindui::Animator::Tick(deltaTime);
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
        anim = we::runtime::kindui::Animator::Damp(anim, target, expandSpeed);
        if (std::abs(anim - previous) > 0.001f) {
            expandChanged = true;
        }
    }

    for (auto& entry : m_Layout) {
        entry.pressAnim = we::runtime::kindui::Animator::Damp(entry.pressAnim, 0.0f, pressSpeed);
    }

    if (expandChanged) {
        RebuildLayout();
    }
}

} // namespace we::programs::editor
