#include "Widgets/ExplorerPanelHeader.h"

#include "Explorer/ExplorerPanelAssets.h"
#include "Core/PaintContext.h"
#include "Core/Theme.h"
#include "Core/Icon.h"
#include "Core/DPIContext.h"

#include <SDL3/SDL.h>
#include <algorithm>
#include <cmath>

namespace we::UI {

ExplorerPanelHeader::ExplorerPanelHeader() {
}

Size ExplorerPanelHeader::Measure(const Size& availableSize) {
    m_DesiredSize = Size{ availableSize.width, DefaultHeight() };
    return m_DesiredSize;
}

void ExplorerPanelHeader::Arrange(const Rect& allottedRect) {
    const auto& theme = Theme::Get();
    m_Geometry = allottedRect;

    float searchBoxWidth = std::min(theme.Space6 * 11.67f, std::max(theme.Space6 * 5.83f, allottedRect.width - 200.0f));

    const float searchX = allottedRect.x + theme.Space2;
    const float searchY = allottedRect.y + (DefaultHeight() - theme.SearchBoxHeight) * 0.5f;
    m_SearchBoxGeometry = Rect{ searchX, searchY, searchBoxWidth, theme.SearchBoxHeight };

    float x = allottedRect.x + allottedRect.width - theme.Space2;

    x -= theme.IconButtonSize;
    const float buttonY = allottedRect.y + (DefaultHeight() - theme.IconButtonSize) * 0.5f;
    m_RefreshButtonGeometry = Rect{ x, buttonY, theme.IconButtonSize, theme.IconButtonSize };
    x -= theme.Space1;

    x -= theme.IconButtonSize;
    m_NewFolderButtonGeometry = Rect{ x, buttonY, theme.IconButtonSize, theme.IconButtonSize };
    x -= theme.Space1;

    x -= theme.IconButtonSize;
    m_FilterButtonGeometry = Rect{ x, buttonY, theme.IconButtonSize, theme.IconButtonSize };
}

void ExplorerPanelHeader::Paint(PaintContext& context) {
    const auto& theme = Theme::Get();
    const float iconSize = theme.IconSizeSearch;
    const float fontSize = theme.TextSizeBody;

    context.DrawRect(m_Geometry, theme.HeaderBackground);

    context.DrawRect(
        Rect{ m_Geometry.x, m_Geometry.y + m_Geometry.height - theme.BorderWidth, m_Geometry.width, theme.BorderWidth },
        theme.Separator);

    const Color searchBg = theme.SearchBoxBg;
    const Color searchBorder = m_SearchFocused
        ? Color::Lerp(theme.BorderDefault, theme.BorderFocus, 0.35f)
        : theme.BorderDefault;

    context.DrawRoundedRect(m_SearchBoxGeometry, searchBg, theme.CornerRadiusSmall);
    context.DrawRoundedRectOutline(m_SearchBoxGeometry, searchBorder, theme.BorderWidth, theme.CornerRadiusSmall);

    const float searchIconX = m_SearchBoxGeometry.x + theme.Space2;
    const float searchIconY = m_SearchBoxGeometry.y + (m_SearchBoxGeometry.height - iconSize) * 0.5f;
    IconPainter::DrawIcon(context, Icons::SearchName,
                          Rect{ searchIconX, searchIconY, iconSize, iconSize },
                          theme.SearchIcon);

    const float textX = searchIconX + iconSize + theme.Space2 - 2.0f;
    const float textY = m_SearchBoxGeometry.y + (m_SearchBoxGeometry.height - fontSize) * 0.5f;

    if (m_SearchQuery.empty()) {
        context.DrawText("Search...", Point{ textX, textY }, theme.SearchPlaceholder, fontSize);
    } else {
        context.DrawText(m_SearchQuery, Point{ textX, textY }, theme.TextPrimary, fontSize);

        if (m_SearchFocused) {
            const float cursorX = textX + context.GetTextWidth(m_SearchQuery, fontSize) + theme.BorderWidth;
            if (static_cast<int>(m_CursorBlink * 3.0f) % 2 == 0) {
                context.DrawRect(Rect{ cursorX, textY, theme.BorderWidth, fontSize }, theme.TextPrimary);
            }
        }
    }

    if (!m_SearchQuery.empty()) {
        const float clearX = m_SearchBoxGeometry.x + m_SearchBoxGeometry.width - iconSize - theme.Space2 + 2.0f;
        const float clearY = m_SearchBoxGeometry.y + (m_SearchBoxGeometry.height - iconSize) * 0.5f;
        IconPainter::DrawIcon(context, Icons::XName,
                              Rect{ clearX, clearY, iconSize, iconSize },
                              theme.SearchIcon);
    }

    const float separatorX = m_SearchBoxGeometry.x + m_SearchBoxGeometry.width + theme.Space2;
    context.DrawRect(Rect{ separatorX, m_Geometry.y + theme.Space2 - 2.0f, theme.BorderWidth, m_Geometry.height - theme.Space3 + 4.0f }, theme.Separator);

    PaintToolbarButton(context, m_FilterButtonGeometry, Icons::FilterName,
                      m_HoveredButton == 0 || m_PressedButton == 0, theme);
    PaintToolbarButton(context, m_NewFolderButtonGeometry, Icons::PlusName,
                      m_HoveredButton == 1 || m_PressedButton == 1, theme);
    PaintToolbarButton(context, m_RefreshButtonGeometry, Icons::RefreshName,
                      m_HoveredButton == 2 || m_PressedButton == 2, theme);
}

void ExplorerPanelHeader::PaintToolbarButton(PaintContext& context, const Rect& geometry,
                                         const std::string& iconName, bool hovered, const Theme& theme) {
    if (hovered) {
        context.DrawRoundedRect(geometry, theme.HoverButton, theme.CornerRadiusSmall);
    }

    const float iconSize = theme.IconSizeToolbar;
    const float iconX = geometry.x + (geometry.width - iconSize) * 0.5f;
    const float iconY = geometry.y + (geometry.height - iconSize) * 0.5f;

    IconPainter::DrawIcon(context, iconName, Rect{ iconX, iconY, iconSize, iconSize },
                          theme.IconForState(hovered));
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

    const auto& theme = Theme::Get();
    const float iconSize = theme.IconSizeSearch;

    if (m_SearchBoxGeometry.Contains(event.position)) {
        m_SearchFocused = true;

        if (!m_SearchQuery.empty()) {
            const float clearX = m_SearchBoxGeometry.x + m_SearchBoxGeometry.width - iconSize - theme.Space2 + 2.0f;
            const float clearY = m_SearchBoxGeometry.y + (m_SearchBoxGeometry.height - iconSize) * 0.5f;
            Rect clearRect{ clearX, clearY, iconSize, iconSize };
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

    m_PressedButton = HitButtonIndex(event.position);
    if (m_PressedButton < 0) {
        return;
    }

    switch (m_PressedButton) {
    case 0:
        if (m_OnFilterClicked) m_OnFilterClicked();
        break;
    case 1:
        if (m_OnNewFolder) m_OnNewFolder();
        break;
    case 2:
        if (m_OnRefresh) m_OnRefresh();
        break;
    default:
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
    }
}

void ExplorerPanelHeader::OnTextInput(const std::string& text) {
    if (!m_SearchFocused) {
        return;
    }

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
