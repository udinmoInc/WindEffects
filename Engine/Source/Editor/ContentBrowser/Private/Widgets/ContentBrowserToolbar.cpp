#include "Widgets/ContentBrowserToolbar.h"
#include "WindEffects/Editor/UI/Panel/PanelChrome.h"
#include "Widgets/SearchBox.h"
#include "Widgets/ContentBrowser.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Core/DPIContext.h"
#include "KindUI/Rendering/IconMetrics.h"
#include "KindUI/Theming/ThemeAccess.h"
#include "KindUI/Theming/ThemeColors.h"
#include "KindUI/Theming/ThemeToken.h"
#include "KindUI/Core/Icon.h"
#include "KindUI/Core/Animator.h"
#include <algorithm>

namespace we::runtime::kindui {

namespace {

void PaintToolbarButtonChrome(PaintContext& context, const Rect& rect, float hoverAnim, float pressAnim,
    bool selected, bool primary)
{
    Color bg = ResolveThemeInteractiveBackground(hoverAnim, pressAnim, selected);

    const float radius = ResolveThemeMetric(ThemeToken::CornerRadiusSmall);
    if (bg.a > 0.01f) {
        context.DrawRoundedRect(rect, bg, radius);
    }

    if (primary) {
        Color border = ResolveThemeColor(ThemeToken::AccentPrimary);
        border.a = 0.55f + hoverAnim * 0.2f;
        context.DrawRoundedRectOutline(rect, border, ResolveThemeMetric(ThemeToken::BorderWidth), radius);
        Color fill = ResolveThemeColor(ThemeToken::AccentPrimary);
        fill.a = 0.08f + hoverAnim * 0.06f;
        context.DrawRoundedRect(rect, fill, radius);
    } else if (selected) {
        context.DrawRoundedRectOutline(rect, ResolveThemeColor(ThemeToken::PressedBackground), ResolveThemeMetric(ThemeToken::BorderWidth), radius);
    }
}

Rect CenterRect(const Rect& parent, float w, float h) {
    return Rect{
        parent.x + (parent.width - w) * 0.5f,
        parent.y + (parent.height - h) * 0.5f,
        w,
        h
    };
}

} // namespace

ToolbarIconToggle::ToolbarIconToggle(const std::string& iconName, const char*)
    : m_IconName(iconName)
{}

Size ToolbarIconToggle::Measure(const Size& availableSize) {
    (void)availableSize;
    const float h = ThemeMetric(ThemeToken::ButtonHeight);
    m_DesiredSize = Size{ h, h };
    return m_DesiredSize;
}

void ToolbarIconToggle::Arrange(const Rect& allottedRect) {
    const float h = ThemeMetric(ThemeToken::ButtonHeight);
    m_Geometry = CenterRect(allottedRect, h, h);
}

void ToolbarIconToggle::Paint(PaintContext& context) {
    m_HoverAnim = Animator::Damp(m_HoverAnim, m_Hovered ? 1.0f : 0.0f, 15.0f);
    m_PressAnim = Animator::Damp(m_PressAnim, m_Pressed ? 1.0f : 0.0f, 25.0f);
    PaintToolbarButtonChrome(context, m_Geometry, m_HoverAnim, m_PressAnim, m_Selected, false);

    const float iconSize = static_cast<float>(IconMetrics::NativeIconTierPx(ThemeMetric(ThemeToken::IconSizeToolbar)));
    const Rect iconRect = CenterRect(m_Geometry, iconSize, iconSize);
    Color iconColor = m_Selected
        ? ThemeColor(ThemeToken::IconAccent)
        : ResolveIconColor(IconColorRole::Secondary, m_HoverAnim, m_PressAnim);
    IconPainter::DrawIcon(context, m_IconName, iconRect, iconColor);
}

void ToolbarIconToggle::OnMouseDown(const MouseEvent& event) {
    if (event.button == MouseButton::Left) m_Pressed = true;
}

void ToolbarIconToggle::OnMouseUp(const MouseEvent& event) {
    if (event.button == MouseButton::Left && m_Pressed) {
        m_Pressed = false;
        if (m_Geometry.Contains(event.position) && m_OnClicked) {
            m_OnClicked();
        }
    }
}

ToolbarLabeledButton::ToolbarLabeledButton(const std::string& label, const std::string& iconName,
    bool showChevron, Variant variant, float horizontalPadding)
    : m_Label(label)
    , m_IconName(iconName)
    , m_ShowChevron(showChevron)
    , m_Variant(variant)
    , m_HorizontalPadding(horizontalPadding)
{}

Size ToolbarLabeledButton::Measure(const Size& availableSize) {
    (void)availableSize;
    float width = m_HorizontalPadding * 2.0f;
    if (!m_IconName.empty()) width += ThemeMetric(ThemeToken::IconSizeToolbar) + ThemeMetric(ThemeToken::Space1) + 1.0f;
    width += static_cast<float>(m_Label.size()) * 7.2f;
    if (m_ShowChevron) width += ThemeMetric(ThemeToken::Space2) + IconMetrics::CompactDisplayPx();
    m_DesiredSize = Size{ width, ThemeMetric(ThemeToken::ButtonHeight) };
    return m_DesiredSize;
}

void ToolbarLabeledButton::Arrange(const Rect& allottedRect) {
    const float h = std::min(ThemeMetric(ThemeToken::ButtonHeight), allottedRect.height);
    m_Geometry = Rect{
        allottedRect.x,
        allottedRect.y + (allottedRect.height - h) * 0.5f,
        allottedRect.width,
        h
    };
}

void ToolbarLabeledButton::Paint(PaintContext& context) {
    m_HoverAnim = Animator::Damp(m_HoverAnim, m_Hovered ? 1.0f : 0.0f, 15.0f);
    m_PressAnim = Animator::Damp(m_PressAnim, m_Pressed ? 1.0f : 0.0f, 25.0f);
    PaintToolbarButtonChrome(context, m_Geometry, m_HoverAnim, m_PressAnim, false, m_Variant == Variant::Primary);

    if (m_Label == "Create" && m_Variant == Variant::Standard) {
        const float radius = ThemeMetric(ThemeToken::CornerRadiusSmall);

        context.DrawRoundedRectOutline(
            Rect{ m_Geometry.x + ThemeMetric(ThemeToken::BorderWidth), m_Geometry.y + ThemeMetric(ThemeToken::BorderWidth),
                m_Geometry.width - 2.0f * ThemeMetric(ThemeToken::BorderWidth), m_Geometry.height - 2.0f * ThemeMetric(ThemeToken::BorderWidth) },
            ThemeColor(ThemeToken::HighlightSubtle), ThemeMetric(ThemeToken::BorderWidth), radius
        );

        context.DrawRoundedRectOutline(
            Rect{ m_Geometry.x - ThemeMetric(ThemeToken::BorderWidth), m_Geometry.y - ThemeMetric(ThemeToken::BorderWidth),
                m_Geometry.width + 2.0f * ThemeMetric(ThemeToken::BorderWidth), m_Geometry.height + 2.0f * ThemeMetric(ThemeToken::BorderWidth) },
            ThemeColor(ThemeToken::ShadowSubtle), ThemeMetric(ThemeToken::BorderWidth), radius
        );
    }
    const float hPad = m_HorizontalPadding;
    float x = m_Geometry.x + hPad;
    const float textSize = ThemeMetric(ThemeToken::TextSizeBody);
    const float textY = m_Geometry.y + (m_Geometry.height - textSize) * 0.5f;

    if (!m_IconName.empty()) {
        const float iconSize = static_cast<float>(IconMetrics::NativeIconTierPx(ThemeMetric(ThemeToken::IconSizeToolbar)));
        Color iconColor = m_Variant == Variant::Primary
            ? ThemeColor(ThemeToken::IconAccent)
            : ResolveIconColor(IconColorRole::Secondary, m_HoverAnim, m_PressAnim);
        Rect iconBand{ x, m_Geometry.y, iconSize, m_Geometry.height };
        IconPainter::DrawIcon(context, m_IconName, IconMetrics::PlaceGlyphCentered(iconBand, iconSize), iconColor);
        x += iconSize + ThemeMetric(ThemeToken::Space2);
    }

    Color textColor = ThemeColor(ThemeToken::TextPrimary);
    if (m_Variant == Variant::Primary) {
        textColor = Color::Lerp(ThemeColor(ThemeToken::TextPrimary), ThemeColor(ThemeToken::AccentPrimary), 0.25f);
    }
    context.DrawText(m_Label, Point{ x, textY }, textColor, textSize, m_Variant == Variant::Primary);

    if (m_ShowChevron) {
        const float display = IconMetrics::CompactDisplayPx();
        const float chevronX = m_Geometry.x + m_Geometry.width - hPad - display;
        const float centerY = m_Geometry.y + m_Geometry.height * 0.5f;
        Rect chevronControl{ chevronX, centerY - display * 0.5f, display, display };
            IconPainter::DrawCompactIcon(context, Icons::ChevronDownName, chevronControl,
            ResolveIconColor(IconColorRole::Secondary, m_HoverAnim, m_PressAnim));
    }
}

void ToolbarLabeledButton::OnMouseDown(const MouseEvent& event) {
    if (event.button == MouseButton::Left) m_Pressed = true;
}

void ToolbarLabeledButton::OnMouseUp(const MouseEvent& event) {
    if (event.button == MouseButton::Left && m_Pressed) {
        m_Pressed = false;
        if (m_Geometry.Contains(event.position) && m_OnClicked) {
            m_OnClicked();
        }
    }
}

std::shared_ptr<ContentBrowserToolbarControls> ContentBrowserToolbarControls::Create(ToolbarMode mode) {
    auto toolbar = std::shared_ptr<ContentBrowserToolbarControls>(new ContentBrowserToolbarControls(mode));
    toolbar->InitializeChildren();
    return toolbar;
}

ContentBrowserToolbarControls::ContentBrowserToolbarControls(ToolbarMode mode)
    : m_Mode(mode)
{}

void ContentBrowserToolbarControls::InitializeChildren() {
    if (m_Mode == ToolbarMode::Full) {
        m_CreateBtn = std::make_shared<PrimaryToolbarButton>("Add", Icons::PlusName);
        m_ImportBtn = std::make_shared<SecondaryToolbarButton>("Import", "import");
        m_SaveBtn = std::make_shared<ToolbarLabeledButton>("Save All", Icons::SaveAllName, false, ToolbarLabeledButton::Variant::Standard, ThemeMetric(ThemeToken::Space3));

        AddChild(m_CreateBtn);
        AddChild(m_ImportBtn);
        AddChild(m_SaveBtn);
    } else {
        // Asset pane toolbar: search, save all, filter icon
        m_SearchBox = std::make_shared<SearchBox>();
        m_SearchBox->SetPlaceholder("Search Assets...");
        m_SearchBox->SetWidth(ThemeMetric(ThemeToken::Space6) * 15.0f);

        m_SaveBtn = std::make_shared<ToolbarLabeledButton>("Save All", Icons::SaveAllName, false, ToolbarLabeledButton::Variant::Standard, ThemeMetric(ThemeToken::Space3));
        m_FilterIconBtn = std::make_shared<ToolbarIconToggle>(Icons::FilterName, "Filter");

        AddChild(m_SearchBox);
        AddChild(m_SaveBtn);
        AddChild(m_FilterIconBtn);
    }
}

Size ContentBrowserToolbarControls::Measure(const Size& availableSize) {
    m_DesiredSize = Size{ availableSize.width, ThemeMetric(ThemeToken::PanelToolbarHeight) };
    return m_DesiredSize;
}

void ContentBrowserToolbarControls::ArrangeControlRow(const Rect& row, float contentLeft, float contentRight) {
    const float centerY = row.y + row.height * 0.5f;
    const float contentWidth = std::max(0.0f, contentRight - contentLeft);
    const Size measureSize{ contentWidth, ThemeMetric(ThemeToken::ButtonHeight) };

    if (m_Mode == ToolbarMode::Full) {
        float x = contentLeft;

        if (m_CreateBtn) {
            const Size desired = m_CreateBtn->Measure(measureSize);
            m_CreateBtn->Arrange(Rect{ x, centerY - desired.height * 0.5f, desired.width, desired.height });
            x += desired.width + ThemeMetric(ThemeToken::ButtonSpacing);
        }

        if (m_ImportBtn) {
            const Size desired = m_ImportBtn->Measure(measureSize);
            m_ImportBtn->Arrange(Rect{ x, centerY - desired.height * 0.5f, desired.width, desired.height });
            x += desired.width + ThemeMetric(ThemeToken::ButtonGroupSpacing);
        }

        if (m_SaveBtn) {
            const Size desired = m_SaveBtn->Measure(measureSize);
            m_SaveBtn->Arrange(Rect{ x, centerY - desired.height * 0.5f, desired.width, desired.height });
        }
    } else {
        // Asset pane toolbar: search, save all, filter icon
        float searchWidth = std::min(ThemeMetric(ThemeToken::Space6) * 15.0f, std::max(ThemeMetric(ThemeToken::Space6) * 10.0f, contentWidth * 0.5f));

        const std::vector<std::shared_ptr<Widget>> actionButtons = {
            m_SaveBtn, m_FilterIconBtn
        };

        float actionsWidth = 0.0f;
        for (const auto& widget : actionButtons) {
            actionsWidth += widget->Measure(measureSize).width + ThemeMetric(ThemeToken::Space1);
        }

        const float fixedWidth = searchWidth + ThemeMetric(ThemeToken::Space2) + actionsWidth;

        if (fixedWidth > contentWidth) {
            searchWidth = std::max(ThemeMetric(ThemeToken::Space6) * 6.67f, contentWidth - (fixedWidth - searchWidth));
        }

        float x = contentLeft;

        m_SearchBox->Arrange(Rect{
            x,
            centerY - ThemeMetric(ThemeToken::SearchBoxHeight) * 0.5f,
            searchWidth,
            ThemeMetric(ThemeToken::SearchBoxHeight)
        });
        x += searchWidth + ThemeMetric(ThemeToken::Space2);

        for (const auto& widget : actionButtons) {
            const Size desired = widget->Measure(measureSize);
            widget->Arrange(Rect{ x, centerY - desired.height * 0.5f, desired.width, desired.height });
            x += desired.width + ThemeMetric(ThemeToken::Space1);
        }
    }
}

void ContentBrowserToolbarControls::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
    const float contentLeft = allottedRect.x + ThemeMetric(ThemeToken::Space3);
    const float contentRight = allottedRect.x + allottedRect.width - ThemeMetric(ThemeToken::Space3);
    ArrangeControlRow(allottedRect, contentLeft, contentRight);
}

void ContentBrowserToolbarControls::Paint(PaintContext& context) {
    PanelChrome::PaintToolbarRegion(context, m_Geometry);

    for (const auto& child : m_Children) {
        if (child->IsVisible()) child->Paint(context);
    }
}

void ContentBrowserToolbarControls::OnMouseDown(const MouseEvent& event) {
    for (const auto& child : m_Children) {
        if (child->GetGeometry().Contains(event.position)) {
            child->OnMouseDown(event);
            return;
        }
    }
}

void ContentBrowserToolbarControls::OnMouseUp(const MouseEvent& event) {
    for (const auto& child : m_Children) {
        child->OnMouseUp(event);
    }
}

void ContentBrowserToolbarControls::OnMouseMove(const MouseEvent& event) {
    for (const auto& child : m_Children) {
        const bool hovered = child->GetGeometry().Contains(event.position);
        child->SetHovered(hovered);
        child->OnMouseMove(event);
    }
}

void ContentBrowserToolbarControls::SetOnFilterClicked(std::function<void()> callback) {
    if (m_FilterBtn) {
        m_FilterBtn->SetOnClicked(std::move(callback));
    }
    if (m_FilterIconBtn) {
        m_FilterIconBtn->SetOnClicked(std::move(callback));
    }
}

void ContentBrowserToolbarControls::SetOnSortClicked(std::function<void()> callback) {
    m_SortBtn->SetOnClicked(std::move(callback));
}

void ContentBrowserToolbarControls::SetOnImportClicked(std::function<void()> callback) {
    m_ImportBtn->SetOnClicked(std::move(callback));
}

void ContentBrowserToolbarControls::SetOnCreateClicked(std::function<void()> callback) {
    m_CreateBtn->SetOnClicked(std::move(callback));
}

void ContentBrowserToolbarControls::SetOnViewModeChanged(std::function<void(ContentViewMode)> callback) {
    m_GridViewBtn->SetOnClicked([this, callback]() {
        m_GridViewBtn->SetSelected(true);
        m_ListViewBtn->SetSelected(false);
        if (callback) callback(ContentViewMode::LargeIcons);
    });
    m_ListViewBtn->SetOnClicked([this, callback]() {
        m_ListViewBtn->SetSelected(true);
        m_GridViewBtn->SetSelected(false);
        if (callback) callback(ContentViewMode::List);
    });
}

void ContentBrowserToolbarControls::SetOnSettingsClicked(std::function<void()> callback) {
    m_SettingsBtn->SetOnClicked(std::move(callback));
}

void ContentBrowserToolbarControls::SetOnSaveClicked(std::function<void()> callback) {
    if (m_SaveBtn) {
        m_SaveBtn->SetOnClicked(std::move(callback));
    }
}

void ContentBrowserToolbarControls::SetOnPreviousClicked(std::function<void()> callback) {
    if (m_BackBtn) {
        m_BackBtn->SetOnClicked(std::move(callback));
    }
}

void ContentBrowserToolbarControls::SetOnNextClicked(std::function<void()> callback) {
    if (m_ForwardBtn) {
        m_ForwardBtn->SetOnClicked(std::move(callback));
    }
}

void ContentBrowserToolbarControls::SetOnFolderClicked(std::function<void()> callback) {
    if (m_FolderBtn) {
        m_FolderBtn->SetOnClicked(std::move(callback));
    }
}

} // namespace we::runtime::kindui
