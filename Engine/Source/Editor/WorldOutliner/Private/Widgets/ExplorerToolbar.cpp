#include "Platform/Platform.h"
#include "Widgets/ExplorerToolbar.h"

#include "KindUI/Core/PaintContext.h"
#include "KindUI/Rendering/IconMetrics.h"
#include "KindUI/Theming/ThemeAccess.h"
#include "KindUI/Core/Icon.h"

#include <algorithm>
#include <cmath>

namespace we::runtime::kindui {

ExplorerToolbar::ExplorerToolbar() {
}

Size ExplorerToolbar::Measure(const Size& availableSize) {
    m_DesiredSize = Size{ availableSize.width, ThemeMetric(ThemeToken::PanelHeaderHeight) };
    return m_DesiredSize;
}

void ExplorerToolbar::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;

    float searchBoxWidth = std::min(ThemeMetric(ThemeToken::Space6) * 11.67f, std::max(ThemeMetric(ThemeToken::Space6) * 5.83f, allottedRect.width - 200.0f));

    const float searchX = allottedRect.x + ThemeMetric(ThemeToken::Space2);
    const float searchY = allottedRect.y + (ThemeMetric(ThemeToken::PanelHeaderHeight) - ThemeMetric(ThemeToken::SearchBoxHeight)) * 0.5f;
    m_SearchBoxGeometry = Rect{ searchX, searchY, searchBoxWidth, ThemeMetric(ThemeToken::SearchBoxHeight) };

    float x = allottedRect.x + allottedRect.width - ThemeMetric(ThemeToken::Space2);

    x -= ThemeMetric(ThemeToken::IconButtonSize);
    const float buttonY = allottedRect.y + (ThemeMetric(ThemeToken::PanelHeaderHeight) - ThemeMetric(ThemeToken::IconButtonSize)) * 0.5f;
    m_RefreshButtonGeometry = Rect{ x, buttonY, ThemeMetric(ThemeToken::IconButtonSize), ThemeMetric(ThemeToken::IconButtonSize) };
    x -= ThemeMetric(ThemeToken::Space1);

    x -= ThemeMetric(ThemeToken::IconButtonSize);
    m_NewFolderButtonGeometry = Rect{ x, buttonY, ThemeMetric(ThemeToken::IconButtonSize), ThemeMetric(ThemeToken::IconButtonSize) };
    x -= ThemeMetric(ThemeToken::Space1);

    x -= ThemeMetric(ThemeToken::IconButtonSize);
    m_FilterButtonGeometry = Rect{ x, buttonY, ThemeMetric(ThemeToken::IconButtonSize), ThemeMetric(ThemeToken::IconButtonSize) };
}

void ExplorerToolbar::Paint(PaintContext& context) {
    const float iconSize = static_cast<float>(IconMetrics::NativeIconTierPx(ThemeMetric(ThemeToken::IconSizeSearch)));
    const float fontSize = ThemeMetric(ThemeToken::TextSizeBody);

    context.DrawRect(m_Geometry, ThemeColor(ThemeToken::HeaderBackground));

    context.DrawRect(
        Rect{ m_Geometry.x, m_Geometry.y + m_Geometry.height - ThemeMetric(ThemeToken::BorderWidth), m_Geometry.width, ThemeMetric(ThemeToken::BorderWidth) },
        ThemeColor(ThemeToken::Separator));

    const Color searchBg = ThemeColor(ThemeToken::SearchBoxBackground);
    const Color searchBorder = m_SearchFocused
        ? Color::Lerp(ThemeColor(ThemeToken::BorderDefault), ThemeColor(ThemeToken::BorderFocus), 0.35f)
        : ThemeColor(ThemeToken::BorderDefault);

    context.DrawRoundedRect(m_SearchBoxGeometry, searchBg, ThemeMetric(ThemeToken::CornerRadiusSmall));
    context.DrawRoundedRectOutline(m_SearchBoxGeometry, searchBorder, ThemeMetric(ThemeToken::BorderWidth), ThemeMetric(ThemeToken::CornerRadiusSmall));

    const float searchIconX = m_SearchBoxGeometry.x + ThemeMetric(ThemeToken::Space2);
    const float searchIconY = m_SearchBoxGeometry.y + (m_SearchBoxGeometry.height - iconSize) * 0.5f;
    IconPainter::DrawIcon(context, Icons::SearchName,
                          Rect{ searchIconX, searchIconY, iconSize, iconSize },
                          ThemeColor(ThemeToken::IconPrimary));

    const float textX = searchIconX + iconSize + ThemeMetric(ThemeToken::Space2) - 2.0f;
    const float textY = m_SearchBoxGeometry.y + (m_SearchBoxGeometry.height - fontSize) * 0.5f;

    if (m_SearchQuery.empty()) {
        context.DrawText("Search...", Point{ textX, textY }, ThemeColor(ThemeToken::SearchPlaceholder), fontSize);
    } else {
        context.DrawText(m_SearchQuery, Point{ textX, textY }, ThemeColor(ThemeToken::TextPrimary), fontSize);

        if (m_SearchFocused) {
            const float cursorX = textX + context.GetTextWidth(m_SearchQuery, fontSize) + ThemeMetric(ThemeToken::BorderWidth);
            if (static_cast<int>(m_CursorBlink * 3.0f) % 2 == 0) {
                context.DrawRect(Rect{ cursorX, textY, ThemeMetric(ThemeToken::BorderWidth), fontSize }, ThemeColor(ThemeToken::TextPrimary));
            }
        }
    }

    if (!m_SearchQuery.empty()) {
        const float clearX = m_SearchBoxGeometry.x + m_SearchBoxGeometry.width - iconSize - ThemeMetric(ThemeToken::Space2) + 2.0f;
        const float clearY = m_SearchBoxGeometry.y + (m_SearchBoxGeometry.height - iconSize) * 0.5f;
        IconPainter::DrawIcon(context, Icons::XName,
                              Rect{ clearX, clearY, iconSize, iconSize },
                              ThemeColor(ThemeToken::IconPrimary));
    }

    const float separatorX = m_SearchBoxGeometry.x + m_SearchBoxGeometry.width + ThemeMetric(ThemeToken::Space2);
    context.DrawRect(Rect{ separatorX, m_Geometry.y + ThemeMetric(ThemeToken::Space2) - 2.0f, ThemeMetric(ThemeToken::BorderWidth), m_Geometry.height - ThemeMetric(ThemeToken::Space3) + 4.0f }, ThemeColor(ThemeToken::Separator));

    PaintToolbarButton(context, m_FilterButtonGeometry, Icons::FilterName,
                      m_HoveredButton == 0 || m_PressedButton == 0);
    PaintToolbarButton(context, m_NewFolderButtonGeometry, Icons::PlusName,
                      m_HoveredButton == 1 || m_PressedButton == 1);
    PaintToolbarButton(context, m_RefreshButtonGeometry, Icons::RefreshName,
                      m_HoveredButton == 2 || m_PressedButton == 2);
}

void ExplorerToolbar::PaintToolbarButton(PaintContext& context, const Rect& geometry,
                                         const std::string& iconName, bool hovered) {
    if (hovered) {
        context.DrawRoundedRect(geometry, ThemeColor(ThemeToken::HoverBackground), ThemeMetric(ThemeToken::CornerRadiusSmall));
    }

    const float iconSize = static_cast<float>(IconMetrics::NativeIconTierPx(ThemeMetric(ThemeToken::IconSizeToolbar)));
    const Rect iconRect = IconMetrics::PlaceGlyphCentered(geometry, iconSize);
    IconPainter::DrawIcon(context, iconName, iconRect, ThemeIconForState(hovered));
}

void ExplorerToolbar::Tick(float deltaTime) {
    Widget::Tick(deltaTime);
    if (m_SearchFocused) {
        m_CursorBlink += deltaTime;
    }
}

int ExplorerToolbar::HitButtonIndex(const Point& position) const {
    if (m_FilterButtonGeometry.Contains(position)) return 0;
    if (m_NewFolderButtonGeometry.Contains(position)) return 1;
    if (m_RefreshButtonGeometry.Contains(position)) return 2;
    return -1;
}

void ExplorerToolbar::OnMouseDown(const MouseEvent& event) {
    if (event.button != MouseButton::Left) {
        return;
    }
    const float iconSize = static_cast<float>(IconMetrics::NativeIconTierPx(ThemeMetric(ThemeToken::IconSizeSearch)));

    if (m_SearchBoxGeometry.Contains(event.position)) {
        m_SearchFocused = true;

        if (!m_SearchQuery.empty()) {
            const float clearX = m_SearchBoxGeometry.x + m_SearchBoxGeometry.width - iconSize - ThemeMetric(ThemeToken::Space2) + 2.0f;
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
    }
}

void ExplorerToolbar::OnMouseMove(const MouseEvent& event) {
    m_HoveredButton = HitButtonIndex(event.position);
}

void ExplorerToolbar::OnMouseUp(const MouseEvent& event) {
    (void)event;
    m_PressedButton = -1;
}

void ExplorerToolbar::OnKeyDown(const KeyEvent& event) {
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

void ExplorerToolbar::OnTextInput(const std::string& text) {
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

bool ExplorerToolbar::ShowsPointerCursor(const Point& position) const {
    return m_SearchBoxGeometry.Contains(position) || HitButtonIndex(position) >= 0;
}

void ExplorerToolbar::SetSearchQuery(const std::string& query) {
    m_SearchQuery = query;
    if (m_OnSearchChanged) {
        m_OnSearchChanged(m_SearchQuery);
    }
}

} // namespace we::runtime::kindui
