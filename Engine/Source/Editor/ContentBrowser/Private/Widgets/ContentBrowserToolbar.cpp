#include "Widgets/ContentBrowserToolbar.hpp"
#include "Widgets/SearchBox.hpp"
#include "Core/PaintContext.hpp"
#include "Core/Theme.hpp"
#include "Core/Icon.hpp"
#include "Core/Animator.hpp"
#include <algorithm>

namespace we::UI {

namespace {

constexpr float kToolbarPadH = 16.0f;
constexpr float kToolbarPadV = 6.0f;
constexpr float kGroupGap = 20.0f;
constexpr float kButtonGap = 8.0f;
constexpr float kControlHeight = 32.0f;
constexpr float kSearchHeight = 35.0f;
constexpr float kIconToggleSize = 30.0f;
constexpr float kSearchWidthRatio = 0.625f;

constexpr Color kToolbarText{ 0.816f, 0.816f, 0.816f, 1.0f }; // #D0D0D0

void PaintToolbarButtonChrome(PaintContext& context, const Rect& rect, float hoverAnim, float pressAnim,
    bool selected, bool primary)
{
    const auto& theme = Theme::Get();
    Color bg = Color{ 0.0f, 0.0f, 0.0f, 0.0f };
    if (selected) {
        bg = Color{ 1.0f, 1.0f, 1.0f, 0.08f };
    }
    if (hoverAnim > 0.01f) {
        bg = Color::Lerp(bg, theme.HoverOverlay, hoverAnim * (selected ? 0.55f : 1.0f));
    }
    if (pressAnim > 0.01f) {
        bg = Color::Lerp(bg, theme.PressedOverlay, pressAnim * 0.65f);
    }

    const float radius = 6.0f;
    if (bg.a > 0.01f) {
        context.DrawRoundedRect(rect, bg, radius);
    }

    if (primary) {
        Color border = theme.SelectedAccent;
        border.a = 0.45f + hoverAnim * 0.15f;
        context.DrawRoundedRectOutline(rect, border, 1.0f, radius);
    } else if (selected) {
        context.DrawRoundedRectOutline(rect, Color{ 1.0f, 1.0f, 1.0f, 0.10f }, 1.0f, radius);
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

Size ToolbarSeparator::Measure(const Size& availableSize) {
    (void)availableSize;
    m_DesiredSize = Size{ kGroupGap, kControlHeight };
    return m_DesiredSize;
}

void ToolbarSeparator::Arrange(const Rect& allottedRect) {
    const float lineH = 20.0f;
    m_Geometry = Rect{
        allottedRect.x + (allottedRect.width - 1.0f) * 0.5f,
        allottedRect.y + (allottedRect.height - lineH) * 0.5f,
        1.0f,
        lineH
    };
}

void ToolbarSeparator::Paint(PaintContext& context) {
    context.DrawRect(m_Geometry, Theme::Get().Separator);
}

ToolbarIconToggle::ToolbarIconToggle(const std::string& iconName, const char*)
    : m_IconName(iconName)
{}

Size ToolbarIconToggle::Measure(const Size& availableSize) {
    (void)availableSize;
    m_DesiredSize = Size{ kIconToggleSize, kIconToggleSize };
    return m_DesiredSize;
}

void ToolbarIconToggle::Arrange(const Rect& allottedRect) {
    m_Geometry = CenterRect(allottedRect, kIconToggleSize, kIconToggleSize);
}

void ToolbarIconToggle::Paint(PaintContext& context) {
    m_HoverAnim = Animator::Damp(m_HoverAnim, m_Hovered ? 1.0f : 0.0f, 15.0f);
    m_PressAnim = Animator::Damp(m_PressAnim, m_Pressed ? 1.0f : 0.0f, 25.0f);
    PaintToolbarButtonChrome(context, m_Geometry, m_HoverAnim, m_PressAnim, m_Selected, false);

    const float iconSize = 16.0f;
    const Rect iconRect = CenterRect(m_Geometry, iconSize, iconSize);
    Color iconColor = m_Selected ? Theme::Get().TextPrimary : Theme::Get().IconMuted;
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
    float width = m_HorizontalPadding * 2.0f;
    if (!m_IconName.empty()) width += 14.0f + 6.0f;
    width += static_cast<float>(m_Label.size()) * 7.6f;
    if (m_ShowChevron) width += 6.0f + 12.0f;
    m_DesiredSize = Size{ width, kControlHeight };
    return m_DesiredSize;
}

void ToolbarLabeledButton::Arrange(const Rect& allottedRect) {
    const float h = std::min(kControlHeight, allottedRect.height);
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

    const float hPad = m_HorizontalPadding;
    float x = m_Geometry.x + hPad;
    const float textSize = 13.0f;
    const float textY = m_Geometry.y + (m_Geometry.height - textSize) * 0.5f;

    if (!m_IconName.empty()) {
        const float iconSize = 14.0f;
        const float iconY = m_Geometry.y + (m_Geometry.height - iconSize) * 0.5f;
        IconPainter::DrawIcon(context, m_IconName, Rect{ x, iconY, iconSize, iconSize }, Theme::Get().IconMuted);
        x += iconSize + 6.0f;
    }

    Color textColor = kToolbarText;
    if (m_Variant == Variant::Primary) {
        textColor = Color::Lerp(kToolbarText, Theme::Get().SelectedAccent, 0.18f);
    }
    context.DrawText(m_Label, Point{ x, textY }, textColor, textSize, m_Variant == Variant::Primary);

    if (m_ShowChevron) {
        const float chevronSize = 12.0f;
        const float chevronX = m_Geometry.x + m_Geometry.width - hPad - chevronSize;
        const float chevronY = m_Geometry.y + (m_Geometry.height - chevronSize) * 0.5f;
        IconPainter::DrawIcon(context, Icons::ChevronDownName, Rect{ chevronX, chevronY, chevronSize, chevronSize },
            Theme::Get().TextSecondary);
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

std::shared_ptr<ContentBrowserToolbarControls> ContentBrowserToolbarControls::Create() {
    auto toolbar = std::shared_ptr<ContentBrowserToolbarControls>(new ContentBrowserToolbarControls());
    toolbar->InitializeChildren();
    return toolbar;
}

void ContentBrowserToolbarControls::InitializeChildren() {
    m_SearchBox = std::make_shared<SearchBox>();
    m_SearchBox->SetPlaceholder("Search assets...");

    m_ViewLarge = std::make_shared<ToolbarIconToggle>("layout-grid");
    m_ViewMedium = std::make_shared<ToolbarIconToggle>("grid-2x2");
    m_ViewSmall = std::make_shared<ToolbarIconToggle>("grid-3x3");
    m_ViewList = std::make_shared<ToolbarIconToggle>(Icons::ListName);
    m_ViewDetails = std::make_shared<ToolbarIconToggle>("layout-list");

    m_SortBtn = std::make_shared<ToolbarLabeledButton>("Sort", "arrow-up-down", true);
    m_FilterBtn = std::make_shared<ToolbarLabeledButton>("Filter", "filter", true);
    m_ImportBtn = std::make_shared<ToolbarLabeledButton>("Import", "import", false, ToolbarLabeledButton::Variant::Standard, 14.0f);
    m_CreateBtn = std::make_shared<ToolbarLabeledButton>("Create", Icons::PlusName, false, ToolbarLabeledButton::Variant::Primary, 14.0f);
    m_Sep1 = std::make_shared<ToolbarSeparator>();
    m_Sep2 = std::make_shared<ToolbarSeparator>();
    m_Sep3 = std::make_shared<ToolbarSeparator>();
    m_Sep4 = std::make_shared<ToolbarSeparator>();
    m_Sep5 = std::make_shared<ToolbarSeparator>();

    AddChild(m_SearchBox);
    const std::shared_ptr<Widget> toolbarChildren[] = {
        m_ViewLarge, m_ViewMedium, m_ViewSmall, m_ViewList, m_ViewDetails,
        m_SortBtn, m_FilterBtn, m_ImportBtn, m_CreateBtn,
        m_Sep1, m_Sep2, m_Sep3, m_Sep4, m_Sep5
    };
    for (const auto& w : toolbarChildren) {
        AddChild(w);
    }

    m_ViewLarge->SetOnClicked([this]() {
        SyncViewToggles(ContentViewMode::LargeIcons);
        if (m_OnViewModeChanged) m_OnViewModeChanged(ContentViewMode::LargeIcons);
    });
    m_ViewMedium->SetOnClicked([this]() {
        SyncViewToggles(ContentViewMode::MediumIcons);
        if (m_OnViewModeChanged) m_OnViewModeChanged(ContentViewMode::MediumIcons);
    });
    m_ViewSmall->SetOnClicked([this]() {
        SyncViewToggles(ContentViewMode::SmallIcons);
        if (m_OnViewModeChanged) m_OnViewModeChanged(ContentViewMode::SmallIcons);
    });
    m_ViewList->SetOnClicked([this]() {
        SyncViewToggles(ContentViewMode::List);
        if (m_OnViewModeChanged) m_OnViewModeChanged(ContentViewMode::List);
    });
    m_ViewDetails->SetOnClicked([this]() {
        SyncViewToggles(ContentViewMode::Details);
        if (m_OnViewModeChanged) m_OnViewModeChanged(ContentViewMode::Details);
    });

    SyncViewToggles(ContentViewMode::LargeIcons);
}

Size ContentBrowserToolbarControls::Measure(const Size& availableSize) {
    m_DesiredSize = Size{ availableSize.width, kSearchHeight + kToolbarPadV * 2.0f };
    return m_DesiredSize;
}

void ContentBrowserToolbarControls::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;

    const float centerY = allottedRect.y + allottedRect.height * 0.5f;
    const float contentLeft = allottedRect.x + kToolbarPadH;
    const float contentRight = allottedRect.x + allottedRect.width - kToolbarPadH;
    const float contentWidth = std::max(0.0f, contentRight - contentLeft);

    const Size measureSize{ contentWidth, kControlHeight };
    const std::vector<std::shared_ptr<Widget>> tailWidgets = {
        m_Sep1, m_ViewLarge, m_ViewMedium, m_ViewSmall, m_ViewList, m_ViewDetails,
        m_Sep2, m_SortBtn, m_Sep3, m_FilterBtn, m_Sep4, m_ImportBtn, m_Sep5, m_CreateBtn
    };

    float tailWidth = 0.0f;
    for (const auto& widget : tailWidgets) {
        tailWidth += widget->Measure(measureSize).width;
        if (widget == m_ViewLarge || widget == m_ViewMedium || widget == m_ViewSmall || widget == m_ViewList) {
            tailWidth += kButtonGap;
        }
    }

    float searchWidth = std::max(160.0f, contentWidth * kSearchWidthRatio);
    if (searchWidth + tailWidth > contentWidth) {
        searchWidth = std::max(140.0f, contentWidth - tailWidth);
    }

    m_SearchBox->Arrange(Rect{
        contentLeft,
        centerY - kSearchHeight * 0.5f,
        searchWidth,
        kSearchHeight
    });

    float x = contentLeft + searchWidth;
    for (const auto& widget : tailWidgets) {
        const Size desired = widget->Measure(measureSize);
        const float slotH = std::max(desired.height, kControlHeight);
        widget->Arrange(Rect{ x, centerY - slotH * 0.5f, desired.width, slotH });
        x += desired.width;
        if (widget == m_ViewLarge || widget == m_ViewMedium || widget == m_ViewSmall || widget == m_ViewList) {
            x += kButtonGap;
        }
    }
}

void ContentBrowserToolbarControls::Paint(PaintContext& context) {
    context.DrawRect(m_Geometry, Theme::Get().ToolbarBackground);
    context.DrawRect(Rect{ m_Geometry.x, m_Geometry.y, m_Geometry.width, 1.0f }, Theme::Get().Separator);
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

void ContentBrowserToolbarControls::SetViewMode(ContentViewMode mode) {
    SyncViewToggles(mode);
}

void ContentBrowserToolbarControls::SyncViewToggles(ContentViewMode mode) {
    m_ViewLarge->SetSelected(mode == ContentViewMode::LargeIcons);
    m_ViewMedium->SetSelected(mode == ContentViewMode::MediumIcons);
    m_ViewSmall->SetSelected(mode == ContentViewMode::SmallIcons);
    m_ViewList->SetSelected(mode == ContentViewMode::List);
    m_ViewDetails->SetSelected(mode == ContentViewMode::Details);
}

void ContentBrowserToolbarControls::SetOnFilterClicked(std::function<void()> callback) {
    m_FilterBtn->SetOnClicked(std::move(callback));
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

} // namespace we::UI
