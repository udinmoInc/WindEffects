#include "Widgets/ContentBrowserToolbar.h"
#include "Widgets/SearchBox.h"
#include "Widgets/ContentBrowser.h"
#include "Core/PaintContext.h"
#include "Core/Theme.h"
#include "Core/Icon.h"
#include "Core/Animator.h"
#include "Core/ToolbarDesignTokens.h"
#include <algorithm>

namespace WindEffects::Editor::UI {

namespace {

void PaintToolbarButtonChrome(PaintContext& context, const Rect& rect, float hoverAnim, float pressAnim,
    bool selected, bool primary)
{
    const auto& theme = Theme::Get();
    Color bg = theme.InteractiveBackground(hoverAnim, pressAnim, selected);

    const float radius = theme.CornerRadiusSmall;
    if (bg.a > 0.01f) {
        context.DrawRoundedRect(rect, bg, radius);
    }

    if (primary) {
        Color border = theme.SelectedAccent;
        border.a = 0.55f + hoverAnim * 0.2f;
        context.DrawRoundedRectOutline(rect, border, theme.BorderWidth, radius);
        Color fill = theme.SelectedAccent;
        fill.a = 0.08f + hoverAnim * 0.06f;
        context.DrawRoundedRect(rect, fill, radius);
    } else if (selected) {
        context.DrawRoundedRectOutline(rect, theme.PressedOverlay, theme.BorderWidth, radius);
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
    const float h = Theme::Get().ButtonHeight;
    m_DesiredSize = Size{ h, h };
    return m_DesiredSize;
}

void ToolbarIconToggle::Arrange(const Rect& allottedRect) {
    const float h = Theme::Get().ButtonHeight;
    m_Geometry = CenterRect(allottedRect, h, h);
}

void ToolbarIconToggle::Paint(PaintContext& context) {
    m_HoverAnim = Animator::Damp(m_HoverAnim, m_Hovered ? 1.0f : 0.0f, 15.0f);
    m_PressAnim = Animator::Damp(m_PressAnim, m_Pressed ? 1.0f : 0.0f, 25.0f);
    PaintToolbarButtonChrome(context, m_Geometry, m_HoverAnim, m_PressAnim, m_Selected, false);

    const float iconSize = Theme::Get().IconSizeToolbar;
    const Rect iconRect = CenterRect(m_Geometry, iconSize, iconSize);
    Color iconColor = m_Selected ? Theme::Get().TextPrimary : Theme::Get().IconDefault;
    if (m_HoverAnim > 0.01f) {
        iconColor = Color::Lerp(iconColor, Theme::Get().TextPrimary, m_HoverAnim * 0.65f);
    }
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
    const auto& theme = Theme::Get();
    float width = m_HorizontalPadding * 2.0f;
    if (!m_IconName.empty()) width += theme.IconSizeToolbar + theme.Space1 + 1.0f;
    width += static_cast<float>(m_Label.size()) * 7.2f;
    if (m_ShowChevron) width += theme.Space1 + theme.IconSizeTree - 3.0f;
    m_DesiredSize = Size{ width, theme.ButtonHeight };
    return m_DesiredSize;
}

void ToolbarLabeledButton::Arrange(const Rect& allottedRect) {
    const float h = std::min(Theme::Get().ButtonHeight, allottedRect.height);
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
        const auto& theme = Theme::Get();
        const float radius = theme.CornerRadiusSmall;

        context.DrawRoundedRectOutline(
            Rect{ m_Geometry.x + theme.BorderWidth, m_Geometry.y + theme.BorderWidth,
                m_Geometry.width - 2.0f * theme.BorderWidth, m_Geometry.height - 2.0f * theme.BorderWidth },
            theme.HighlightSubtle, theme.BorderWidth, radius
        );

        context.DrawRoundedRectOutline(
            Rect{ m_Geometry.x - theme.BorderWidth, m_Geometry.y - theme.BorderWidth,
                m_Geometry.width + 2.0f * theme.BorderWidth, m_Geometry.height + 2.0f * theme.BorderWidth },
            theme.ShadowSubtle, theme.BorderWidth, radius
        );
    }

    const auto& theme = Theme::Get();
    const float hPad = m_HorizontalPadding;
    float x = m_Geometry.x + hPad;
    const float textSize = theme.TextSizeBody;
    const float textY = m_Geometry.y + (m_Geometry.height - textSize) * 0.5f;

    if (!m_IconName.empty()) {
        const float iconSize = theme.IconSizeToolbar;
        const float iconY = m_Geometry.y + (m_Geometry.height - iconSize) * 0.5f;
        Color iconColor = m_Variant == Variant::Primary ? theme.SelectedAccent : theme.IconDefault;
        IconPainter::DrawIcon(context, m_IconName, Rect{ x, iconY, iconSize, iconSize }, iconColor);
        x += iconSize + theme.Space1 + 1.0f;
    }

    Color textColor = theme.TextPrimary;
    if (m_Variant == Variant::Primary) {
        textColor = Color::Lerp(theme.TextPrimary, theme.SelectedAccent, 0.25f);
    }
    context.DrawText(m_Label, Point{ x, textY }, textColor, textSize, m_Variant == Variant::Primary);

    if (m_ShowChevron) {
        const float chevronSize = theme.IconSizeTree - 3.0f;
        const float chevronX = m_Geometry.x + m_Geometry.width - hPad - chevronSize;
        const float chevronY = m_Geometry.y + (m_Geometry.height - chevronSize) * 0.5f;
        IconPainter::DrawIcon(context, Icons::ChevronDownName, Rect{ chevronX, chevronY, chevronSize, chevronSize },
            theme.TextSecondary);
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
        // Panel toolbar: create, import, back, forward, folder
        m_CreateBtn = std::make_shared<PrimaryToolbarButton>("Create", Icons::PlusName);
        m_ImportBtn = std::make_shared<SecondaryToolbarButton>("Import", "import");
        m_BackBtn = std::make_shared<ToolbarNavigationButton>(Icons::ArrowLeftName, "Back");
        m_ForwardBtn = std::make_shared<ToolbarNavigationButton>(Icons::ArrowRightName, "Forward");
        m_FolderBtn = std::make_shared<ToolbarNavigationButton>(Icons::FolderName, "Folder");

        AddChild(m_CreateBtn);
        AddChild(m_ImportBtn);
        AddChild(m_BackBtn);
        AddChild(m_ForwardBtn);
        AddChild(m_FolderBtn);
    } else {
        // Asset pane toolbar: search, save all, filter icon
        m_SearchBox = std::make_shared<SearchBox>();
        m_SearchBox->SetPlaceholder("Search Assets...");
        m_SearchBox->SetWidth(Theme::Get().Space6 * 15.0f);

        m_SaveBtn = std::make_shared<ToolbarLabeledButton>("Save All", Icons::SaveAllName, false, ToolbarLabeledButton::Variant::Standard, Theme::Get().Space3);
        m_FilterIconBtn = std::make_shared<ToolbarIconToggle>(Icons::FilterName, "Filter");

        AddChild(m_SearchBox);
        AddChild(m_SaveBtn);
        AddChild(m_FilterIconBtn);
    }
}

Size ContentBrowserToolbarControls::Measure(const Size& availableSize) {
    m_DesiredSize = Size{ availableSize.width, Theme::Get().ToolbarHeight };
    return m_DesiredSize;
}

void ContentBrowserToolbarControls::ArrangeControlRow(const Rect& row, float contentLeft, float contentRight) {
    using namespace DesignTokens;
    
    const float centerY = row.y + row.height * 0.5f;
    const float contentWidth = std::max(0.0f, contentRight - contentLeft);
    const Size measureSize{ contentWidth, kButtonHeight };

    if (m_Mode == ToolbarMode::Full) {
        // Panel toolbar: create, import, back, forward, folder (left-aligned)
        float x = contentLeft;
        
        // Action buttons group
        if (m_CreateBtn) {
            const Size desired = m_CreateBtn->Measure(measureSize);
            m_CreateBtn->Arrange(Rect{ x, centerY - desired.height * 0.5f, desired.width, desired.height });
            x += desired.width + kButtonSpacing;
        }
        
        if (m_ImportBtn) {
            const Size desired = m_ImportBtn->Measure(measureSize);
            m_ImportBtn->Arrange(Rect{ x, centerY - desired.height * 0.5f, desired.width, desired.height });
            x += desired.width + kButtonGroupSpacing;
        }
        
        // Navigation buttons group (4px spacing)
        const float navSpacing = Theme::Get().Space1;
        
        if (m_BackBtn) {
            const Size desired = m_BackBtn->Measure(measureSize);
            m_BackBtn->Arrange(Rect{ x, centerY - desired.height * 0.5f, desired.width, desired.height });
            x += desired.width + navSpacing;
        }
        
        if (m_ForwardBtn) {
            const Size desired = m_ForwardBtn->Measure(measureSize);
            m_ForwardBtn->Arrange(Rect{ x, centerY - desired.height * 0.5f, desired.width, desired.height });
            x += desired.width + navSpacing;
        }
        
        if (m_FolderBtn) {
            const Size desired = m_FolderBtn->Measure(measureSize);
            m_FolderBtn->Arrange(Rect{ x, centerY - desired.height * 0.5f, desired.width, desired.height });
        }
    } else {
        // Asset pane toolbar: search, save all, filter icon
        const auto& theme = Theme::Get();
        float searchWidth = std::min(theme.Space6 * 15.0f, std::max(theme.Space6 * 10.0f, contentWidth * 0.5f));

        const std::vector<std::shared_ptr<Widget>> actionButtons = {
            m_SaveBtn, m_FilterIconBtn
        };

        float actionsWidth = 0.0f;
        for (const auto& widget : actionButtons) {
            actionsWidth += widget->Measure(measureSize).width + theme.Space1;
        }

        const float fixedWidth = searchWidth + theme.Space2 + actionsWidth;

        if (fixedWidth > contentWidth) {
            searchWidth = std::max(theme.Space6 * 6.67f, contentWidth - (fixedWidth - searchWidth));
        }

        float x = contentLeft;

        m_SearchBox->Arrange(Rect{
            x,
            centerY - theme.SearchBoxHeight * 0.5f,
            searchWidth,
            theme.SearchBoxHeight
        });
        x += searchWidth + theme.Space2;

        for (const auto& widget : actionButtons) {
            const Size desired = widget->Measure(measureSize);
            widget->Arrange(Rect{ x, centerY - desired.height * 0.5f, desired.width, desired.height });
            x += desired.width + theme.Space1;
        }
    }
}

void ContentBrowserToolbarControls::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
    const auto& theme = Theme::Get();
    const float contentLeft = allottedRect.x + theme.Space3;
    const float contentRight = allottedRect.x + allottedRect.width - theme.Space3;
    ArrangeControlRow(allottedRect, contentLeft, contentRight);
}

void ContentBrowserToolbarControls::Paint(PaintContext& context) {
    using namespace DesignTokens;
    
    const auto& theme = Theme::Get();
    
    // Draw toolbar background with depth
    context.DrawRect(m_Geometry, theme.PanelBackground);
    
    // Top highlight (elevation effect)
    context.DrawRect(Rect{ m_Geometry.x, m_Geometry.y, m_Geometry.width, theme.BorderWidth }, ToolbarTopHighlight());

    context.DrawRect(Rect{ m_Geometry.x, m_Geometry.y + m_Geometry.height - theme.BorderWidth, m_Geometry.width, theme.BorderWidth },
        ToolbarSeparator());

    context.DrawRect(Rect{ m_Geometry.x, m_Geometry.y + m_Geometry.height, m_Geometry.width, theme.Space1 },
        ToolbarBottomShadow());
    
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

} // namespace WindEffects::Editor::UI
