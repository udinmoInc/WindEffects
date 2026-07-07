#include "Widgets/ExplorerPanelHeader.h"

#include "Explorer/ExplorerPanelAssets.h"
#include "Core/PaintContext.h"
#include "Core/Theme.h"
#include "Core/Icon.h"

#include <algorithm>
#include <cmath>

namespace we::UI {

namespace {
constexpr float kPadLeft = 8.0f;
constexpr float kPadRight = 8.0f;
constexpr float kButtonSize = 24.0f;
constexpr float kIconSize = 14.0f;
constexpr float kButtonGap = 2.0f;
constexpr float kSearchBoxHeight = 24.0f;
constexpr float kSearchBoxMinWidth = 140.0f;
constexpr float kSearchBoxMaxWidth = 280.0f;
constexpr float kCornerRadius = 4.0f;
} // namespace

ExplorerPanelHeader::ExplorerPanelHeader() {
}

Size ExplorerPanelHeader::Measure(const Size& availableSize) {
    m_DesiredSize = Size{ availableSize.width, kDefaultHeight };
    return m_DesiredSize;
}

void ExplorerPanelHeader::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;

    // Calculate search box width based on available space
    float searchBoxWidth = std::min(kSearchBoxMaxWidth, std::max(kSearchBoxMinWidth, allottedRect.width - 200.0f));
    
    // Search box on the left
    const float searchX = allottedRect.x + kPadLeft;
    const float searchY = allottedRect.y + (kDefaultHeight - kSearchBoxHeight) * 0.5f;
    m_SearchBoxGeometry = Rect{ searchX, searchY, searchBoxWidth, kSearchBoxHeight };

    // Buttons on the right
    float x = allottedRect.x + allottedRect.width - kPadRight;
    
    // Refresh button
    x -= kButtonSize;
    const float refreshY = allottedRect.y + (kDefaultHeight - kButtonSize) * 0.5f;
    m_RefreshButtonGeometry = Rect{ x, refreshY, kButtonSize, kButtonSize };
    x -= kButtonGap;

    // New Folder button
    x -= kButtonSize;
    const float newFolderY = allottedRect.y + (kDefaultHeight - kButtonSize) * 0.5f;
    m_NewFolderButtonGeometry = Rect{ x, newFolderY, kButtonSize, kButtonSize };
    x -= kButtonGap;

    // Filter button
    x -= kButtonSize;
    const float filterY = allottedRect.y + (kDefaultHeight - kButtonSize) * 0.5f;
    m_FilterButtonGeometry = Rect{ x, filterY, kButtonSize, kButtonSize };
}

void ExplorerPanelHeader::Paint(PaintContext& context) {
    const auto& theme = Theme::Get();

    // Draw toolbar background
    context.DrawRect(m_Geometry, theme.HeaderBackground);
    
    // Draw separator at bottom
    context.DrawRect(
        Rect{ m_Geometry.x, m_Geometry.y + m_Geometry.height - 1.0f, m_Geometry.width, 1.0f },
        theme.Separator);

    // Draw search box
    const Color searchBg = m_SearchFocused ? theme.InputBackground : theme.SearchBoxBg;
    const Color searchBorder = m_SearchFocused ? theme.SelectedAccent : theme.BorderDefault;
    
    context.DrawRoundedRect(m_SearchBoxGeometry, searchBg, kCornerRadius);
    context.DrawRoundedRectOutline(m_SearchBoxGeometry, searchBorder, 1.0f, kCornerRadius);

    // Draw search icon
    const float searchIconSize = 14.0f;
    const float searchIconX = m_SearchBoxGeometry.x + 8.0f;
    const float searchIconY = m_SearchBoxGeometry.y + (m_SearchBoxGeometry.height - searchIconSize) * 0.5f;
    IconPainter::DrawIcon(context, Icons::SearchName, 
                          Rect{ searchIconX, searchIconY, searchIconSize, searchIconSize }, 
                          theme.TextSecondary);

    // Draw search text or placeholder
    const float textX = searchIconX + searchIconSize + 6.0f;
    const float textY = m_SearchBoxGeometry.y + (m_SearchBoxGeometry.height - theme.TextSizeNormal) * 0.5f;
    
    if (m_SearchQuery.empty()) {
        context.DrawText("Search...", Point{ textX, textY }, theme.SearchPlaceholder, theme.TextSizeNormal);
    } else {
        context.DrawText(m_SearchQuery, Point{ textX, textY }, theme.TextPrimary, theme.TextSizeNormal);
        
        // Draw cursor if focused
        if (m_SearchFocused) {
            const float cursorX = textX + context.GetTextWidth(m_SearchQuery, theme.TextSizeNormal) + 1.0f;
            if (static_cast<int>(m_CursorBlink * 3.0f) % 2 == 0) {
                context.DrawRect(Rect{ cursorX, textY, 1.0f, theme.TextSizeNormal }, theme.TextPrimary);
            }
        }
    }

    // Draw clear button if there's text
    if (!m_SearchQuery.empty()) {
        const float clearSize = 14.0f;
        const float clearX = m_SearchBoxGeometry.x + m_SearchBoxGeometry.width - clearSize - 6.0f;
        const float clearY = m_SearchBoxGeometry.y + (m_SearchBoxGeometry.height - clearSize) * 0.5f;
        IconPainter::DrawIcon(context, Icons::XName,
                              Rect{ clearX, clearY, clearSize, clearSize },
                              theme.TextSecondary);
    }

    // Draw separator between search and buttons
    const float separatorX = m_SearchBoxGeometry.x + m_SearchBoxGeometry.width + 8.0f;
    context.DrawRect(Rect{ separatorX, m_Geometry.y + 6.0f, 1.0f, m_Geometry.height - 12.0f }, theme.Separator);

    // Draw Filter button
    PaintToolbarButton(context, m_FilterButtonGeometry, Icons::FilterName, 
                      m_HoveredButton == 0 || m_PressedButton == 0, theme);

    // Draw New Folder button
    PaintToolbarButton(context, m_NewFolderButtonGeometry, Icons::PlusName,
                      m_HoveredButton == 1 || m_PressedButton == 1, theme);

    // Draw Refresh button
    PaintToolbarButton(context, m_RefreshButtonGeometry, Icons::RefreshName,
                      m_HoveredButton == 2 || m_PressedButton == 2, theme);
}

void ExplorerPanelHeader::PaintToolbarButton(PaintContext& context, const Rect& geometry, 
                                         const std::string& iconName, bool hovered, const Theme& theme) {
    if (hovered) {
        context.DrawRoundedRect(geometry, theme.HoverOverlay, kCornerRadius);
    }

    const float iconSize = 14.0f;
    const float iconX = geometry.x + (geometry.width - iconSize) * 0.5f;
    const float iconY = geometry.y + (geometry.height - iconSize) * 0.5f;
    
    Color iconColor = hovered ? theme.TextPrimary : theme.TextSecondary;
    IconPainter::DrawIcon(context, iconName, Rect{ iconX, iconY, iconSize, iconSize }, iconColor);
}

void ExplorerPanelHeader::Tick(float deltaTime) {
    Widget::Tick(deltaTime);
    if (m_SearchFocused) {
        m_CursorBlink += deltaTime;
    }
}

int ExplorerPanelHeader::HitButtonIndex(const Point& position) const {
    if (m_FilterButtonGeometry.Contains(position)) return 0;
    if (m_NewFolderButtonGeometry.Contains(position)) return 1;
    if (m_RefreshButtonGeometry.Contains(position)) return 2;
    return -1;
}

void ExplorerPanelHeader::OnMouseDown(const MouseEvent& event) {
    if (event.button != MouseButton::Left) {
        return;
    }

    // Check if clicking on search box
    if (m_SearchBoxGeometry.Contains(event.position)) {
        m_SearchFocused = true;
        
        // Check if clicking on clear button
        if (!m_SearchQuery.empty()) {
            const float clearSize = 14.0f;
            const float clearX = m_SearchBoxGeometry.x + m_SearchBoxGeometry.width - clearSize - 6.0f;
            const float clearY = m_SearchBoxGeometry.y + (m_SearchBoxGeometry.height - clearSize) * 0.5f;
            Rect clearRect{ clearX, clearY, clearSize, clearSize };
            if (clearRect.Contains(event.position)) {
                m_SearchQuery.clear();
                if (m_OnSearchChanged) {
                    m_OnSearchChanged(m_SearchQuery);
                }
            }
        }
        return;
    } else {
        m_SearchFocused = false;
    }

    // Check toolbar buttons
    m_PressedButton = HitButtonIndex(event.position);
    if (m_PressedButton < 0) {
        return;
    }

    switch (m_PressedButton) {
    case 0: // Filter
        if (m_OnFilterClicked) m_OnFilterClicked();
        break;
    case 1: // New Folder
        if (m_OnNewFolder) m_OnNewFolder();
        break;
    case 2: // Refresh
        if (m_OnRefresh) m_OnRefresh();
        break;
    }
}

void ExplorerPanelHeader::OnMouseMove(const MouseEvent& event) {
    m_HoveredButton = HitButtonIndex(event.position);
}

void ExplorerPanelHeader::OnMouseUp(const MouseEvent& event) {
    (void)event;
    m_PressedButton = -1;
}

void ExplorerPanelHeader::OnKeyDown(const KeyEvent& event) {
    if (!m_SearchFocused) {
        return;
    }

    if (event.keycode == SDLK_ESCAPE) {
        m_SearchFocused = false;
        return;
    }
    
    if (event.keycode == SDLK_BACKSPACE && !m_SearchQuery.empty()) {
        m_SearchQuery.pop_back();
        if (m_OnSearchChanged) {
            m_OnSearchChanged(m_SearchQuery);
        }
        return;
    }
}

void ExplorerPanelHeader::OnTextInput(const std::string& text) {
    if (!m_SearchFocused) {
        return;
    }

    // Only accept printable characters
    for (char c : text) {
        if (c >= 32 && c <= 126 && m_SearchQuery.size() < 64) {
            m_SearchQuery.push_back(c);
        }
    }
    if (m_OnSearchChanged) {
        m_OnSearchChanged(m_SearchQuery);
    }
}

bool ExplorerPanelHeader::ShowsPointerCursor(const Point& position) const {
    return m_SearchBoxGeometry.Contains(position) || HitButtonIndex(position) >= 0;
}

void ExplorerPanelHeader::SetSearchQuery(const std::string& query) {
    m_SearchQuery = query;
    if (m_OnSearchChanged) {
        m_OnSearchChanged(m_SearchQuery);
    }
}

} // namespace we::UI
