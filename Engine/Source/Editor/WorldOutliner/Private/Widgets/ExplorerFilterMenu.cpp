#include "Widgets/ExplorerFilterMenu.h"
#include "Widgets/ExplorerToolbar.h"

#include "KindUI/Core/PaintContext.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Theming/StyleRole.h"
#include "KindUI/Core/Icon.h"
#include "KindUI/Layout/OverlayManager.h"

#include <algorithm>

using we::runtime::kindui::ColorToken;
using we::runtime::kindui::MetricToken;
using we::runtime::kindui::PaddingToken;

namespace we::runtime::kindui {

namespace {
constexpr float kMenuItemHeight = 26.0f;
constexpr float kMenuPadding = 4.0f;
constexpr float kCheckSize = 16.0f;
constexpr float kTextOffset = 24.0f;
} // namespace

ExplorerFilterMenu::ExplorerFilterMenu(const FilterOptions& initialOptions, OnFilterChanged callback)
    : m_FilterOptions(initialOptions)
    , m_OnFilterChanged(std::move(callback))
{
    BuildMenuItems();
}

void ExplorerFilterMenu::BuildMenuItems() {
    m_MenuItems.clear();
    
    // Show options
    MenuItem item1 = { "Show Folders", &m_FilterOptions.showFolders, false, false, 0, {} };
    m_MenuItems.push_back(item1);
    
    MenuItem item2 = { "Show Assets", &m_FilterOptions.showAssets, false, false, 0, {} };
    m_MenuItems.push_back(item2);
    
    MenuItem item3 = { "Show Actors", &m_FilterOptions.showActors, false, false, 0, {} };
    m_MenuItems.push_back(item3);
    
    MenuItem item4 = { "Show Components", &m_FilterOptions.showComponents, false, false, 0, {} };
    m_MenuItems.push_back(item4);
    
    MenuItem sep1 = { "", nullptr, true, false, 0, {} };
    m_MenuItems.push_back(sep1); // Separator
    
    // Visibility options
    MenuItem item5 = { "Show Hidden", &m_FilterOptions.showHidden, false, false, 0, {} };
    m_MenuItems.push_back(item5);
    
    MenuItem item6 = { "Show Locked", &m_FilterOptions.showLocked, false, false, 0, {} };
    m_MenuItems.push_back(item6);
    
    MenuItem item7 = { "Show Empty Folders", &m_FilterOptions.showEmptyFolders, false, false, 0, {} };
    m_MenuItems.push_back(item7);
    
    MenuItem item8 = { "Favorites", &m_FilterOptions.showFavorites, false, false, 0, {} };
    m_MenuItems.push_back(item8);
    
    MenuItem sep2 = { "", nullptr, true, false, 0, {} };
    m_MenuItems.push_back(sep2); // Separator
    
    // Sort options (radio group)
    MenuItem item9 = { "Sort A–Z", nullptr, false, true, 0, {} };
    m_MenuItems.push_back(item9);
    
    MenuItem item10 = { "Sort Z–A", nullptr, false, true, 0, {} };
    m_MenuItems.push_back(item10);
    
    MenuItem item11 = { "Modified Recently", nullptr, false, true, 0, {} };
    m_MenuItems.push_back(item11);
}

Size ExplorerFilterMenu::Measure(const Size& availableSize) {
    (void)availableSize;
    float maxWidth = 180.0f;
    for (const auto& item : m_MenuItems) {
        if (!item.isSeparator) {
            maxWidth = std::max(maxWidth, kTextOffset + static_cast<float>(item.label.size()) * 7.5f + 16.0f);
        }
    }
    m_DesiredSize = Size{ maxWidth, kMenuPadding * 2.0f + m_MenuItems.size() * kMenuItemHeight };
    return m_DesiredSize;
}

void ExplorerFilterMenu::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
    
    float y = m_Geometry.y + kMenuPadding;
    for (auto& item : m_MenuItems) {
        item.geometry = Rect{ m_Geometry.x + kMenuPadding, y, m_Geometry.width - kMenuPadding * 2.0f, kMenuItemHeight };
        y += kMenuItemHeight;
    }
}

void ExplorerFilterMenu::Paint(PaintContext& context) {
    
    // Draw menu background with shadow
    context.DrawShadow(m_Geometry, ThemeColor(ColorToken::ShadowOverlay), 6.0f, 12.0f);
    context.DrawRoundedRect(m_Geometry, ThemeColor(ColorToken::PopupBackground), ThemeMetric(MetricToken::CornerRadiusSmall));
    context.DrawRoundedRectOutline(m_Geometry, ThemeColor(ColorToken::BorderDefault), 1.0f, ThemeMetric(MetricToken::CornerRadiusSmall));
    
    for (size_t i = 0; i < m_MenuItems.size(); ++i) {
        const auto& item = m_MenuItems[i];
        
        if (item.isSeparator) {
            const float sepY = item.geometry.y + kMenuItemHeight * 0.5f;
            context.DrawRect(Rect{ item.geometry.x + 4.0f, sepY, item.geometry.width - 8.0f, 1.0f }, ThemeColor(ColorToken::Separator));
            continue;
        }
        
        // Draw hover background
        if (static_cast<int>(i) == m_HoveredItem) {
            context.DrawRoundedRect(item.geometry, ThemeColor(ColorToken::HoverBackground), 3.0f);
        }
        
        // Draw check/radio indicator
        const float checkX = item.geometry.x + 4.0f;
        const float checkY = item.geometry.y + (kMenuItemHeight - kCheckSize) * 0.5f;
        
        if (item.isRadio) {
            // Radio button style - check if this sort option is selected
            const size_t sortStartIndex = 10; // Index where sort options begin
            if (i >= sortStartIndex) {
                const bool isSelected = (m_FilterOptions.sortOrder == static_cast<int>(i - sortStartIndex));
                if (isSelected) {
                    IconPainter::DrawIcon(context, Icons::CheckName, Rect{ checkX, checkY, kCheckSize, kCheckSize }, ThemeColor(ColorToken::AccentPrimary));
                }
            }
        } else {
            // Checkbox style
            if (item.value && *item.value) {
                IconPainter::DrawIcon(context, Icons::CheckName, Rect{ checkX, checkY, kCheckSize, kCheckSize }, ThemeColor(ColorToken::AccentPrimary));
            }
        }
        
        // Draw label
        const float textX = item.geometry.x + kTextOffset;
        const float textY = item.geometry.y + (kMenuItemHeight - ThemeMetric(MetricToken::TextSizeNormal)) * 0.5f;
        context.DrawText(item.label, Point{ textX, textY }, ThemeColor(ColorToken::TextPrimary), ThemeMetric(MetricToken::TextSizeNormal));
    }
}

int ExplorerFilterMenu::HitMenuItemIndex(const Point& position) const {
    for (size_t i = 0; i < m_MenuItems.size(); ++i) {
        if (!m_MenuItems[i].isSeparator && m_MenuItems[i].geometry.Contains(position)) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

void ExplorerFilterMenu::OnMouseDown(const MouseEvent& event) {
    if (event.button != MouseButton::Left) {
        return;
    }
    
    m_PressedItem = HitMenuItemIndex(event.position);
}

void ExplorerFilterMenu::OnMouseMove(const MouseEvent& event) {
    m_HoveredItem = HitMenuItemIndex(event.position);
}

void ExplorerFilterMenu::OnMouseUp(const MouseEvent& event) {
    if (event.button != MouseButton::Left) {
        return;
    }
    
    const int clickedIndex = HitMenuItemIndex(event.position);
    if (clickedIndex >= 0 && clickedIndex == m_PressedItem) {
        auto& item = m_MenuItems[static_cast<size_t>(clickedIndex)];
        
        if (item.isRadio) {
            // Handle radio button selection for sort options
            const size_t sortStartIndex = 10; // Index where sort options begin
            if (clickedIndex >= static_cast<int>(sortStartIndex)) {
                m_FilterOptions.sortOrder = clickedIndex - static_cast<int>(sortStartIndex);
            }
        } else if (item.value) {
            // Toggle checkbox
            *item.value = !*item.value;
        } else {
            // Handle radio buttons without value pointer
            const size_t sortStartIndex = 10;
            if (clickedIndex >= static_cast<int>(sortStartIndex)) {
                m_FilterOptions.sortOrder = clickedIndex - static_cast<int>(sortStartIndex);
            }
        }
        
        if (m_OnFilterChanged) {
            m_OnFilterChanged(m_FilterOptions);
        }
        
        // Close the menu
        if (auto* overlay = GetPopupHost()) {
            overlay->CloseAllPopups();
        }
    }
    
    m_PressedItem = -1;
}

bool ExplorerFilterMenu::ShowsPointerCursor(const Point& position) const {
    return HitMenuItemIndex(position) >= 0;
}

} // namespace we::runtime::kindui
