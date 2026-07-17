#include "Platform/Platform.h"
#include "Widgets/ExplorerPanelHeader.h"

#include "Explorer/ExplorerPanelAssets.h"
#include "WindEffects/Editor/UI/Panel/PanelChrome.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Theming/ThemeAccess.h"
#include "KindUI/Core/Icon.h"
#include "KindUI/Rendering/IconMetrics.h"

#include <algorithm>
#include <cmath>

namespace we::runtime::kindui {

ExplorerPanelHeader::ExplorerPanelHeader() {
}

Size ExplorerPanelHeader::Measure(const Size& availableSize) {
    m_DesiredSize = Size{ availableSize.width, DefaultHeight() };
    return m_DesiredSize;
}

void ExplorerPanelHeader::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;

    const float searchBoxWidth = std::min(
        ThemeMetric(ThemeToken::Space6) * 11.67f,
        std::max(ThemeMetric(ThemeToken::Space6) * 5.83f, allottedRect.width - 200.0f));
    m_SearchBoxGeometry = PanelChrome::InsetSearchRect(allottedRect, searchBoxWidth);

    float x = allottedRect.x + allottedRect.width - ThemeMetric(ThemeToken::Space2);

    x -= PanelChrome::HeaderButtonSize();
    const float buttonY = allottedRect.y + (DefaultHeight() - PanelChrome::HeaderButtonSize()) * 0.5f;
    m_RefreshButtonGeometry = Rect{ x, buttonY, PanelChrome::HeaderButtonSize(), PanelChrome::HeaderButtonSize() };
    x -= ThemeMetric(ThemeToken::Space1);

    x -= PanelChrome::HeaderButtonSize();
    m_NewFolderButtonGeometry = Rect{ x, buttonY, PanelChrome::HeaderButtonSize(), PanelChrome::HeaderButtonSize() };
    x -= ThemeMetric(ThemeToken::Space1);

    x -= PanelChrome::HeaderButtonSize();
    m_FilterButtonGeometry = Rect{ x, buttonY, PanelChrome::HeaderButtonSize(), PanelChrome::HeaderButtonSize() };
}

void ExplorerPanelHeader::Paint(PaintContext& context) {
    PanelChrome::PaintToolbarRegion(context, m_Geometry);

    PanelChrome::PaintSearchField(
        context,
        m_SearchBoxGeometry,
        "Search Actors...",
        m_SearchQuery,
        m_SearchFocused,
        static_cast<int>(m_CursorBlink * 3.0f) % 2 == 0);

    PanelChrome::PaintToolbarIconButton(context, m_FilterButtonGeometry, Icons::FilterName,
        m_HoveredButton == 0, m_PressedButton == 0);
    PanelChrome::PaintToolbarIconButton(context, m_NewFolderButtonGeometry, Icons::PlusName,
        m_HoveredButton == 1, m_PressedButton == 1);
    PanelChrome::PaintToolbarIconButton(context, m_RefreshButtonGeometry, Icons::RefreshName,
        m_HoveredButton == 2, m_PressedButton == 2);
}

void ExplorerPanelHeader::PaintToolbarButton(PaintContext& context, const Rect& geometry,
                                         const std::string& iconName, bool hovered) {
    PanelChrome::PaintToolbarIconButton(context, geometry, iconName, hovered, false);
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
    const float iconSize = static_cast<float>(IconMetrics::NativeIconTierPx(ThemeMetric(ThemeToken::IconSizeSearch)));

    if (m_SearchBoxGeometry.Contains(event.position)) {
        m_SearchFocused = true;

        if (!m_SearchQuery.empty()) {
            const float clearX = m_SearchBoxGeometry.x + m_SearchBoxGeometry.width - iconSize - ThemeMetric(ThemeToken::Space2);
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

    if (event.key == we::platform::KeyCode::Escape) {
        m_SearchFocused = false;
        return;
    }

    if (event.key == we::platform::KeyCode::Backspace && !m_SearchQuery.empty()) {
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

} // namespace we::runtime::kindui
