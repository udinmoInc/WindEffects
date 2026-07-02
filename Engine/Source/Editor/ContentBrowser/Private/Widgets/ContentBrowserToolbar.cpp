#include "Widgets/ContentBrowserToolbar.hpp"
#include "Widgets/SearchBox.hpp"
#include "Core/PaintContext.hpp"
#include "Core/Theme.hpp"
#include "Core/Icon.hpp"
#include "Core/Animator.hpp"
#include <algorithm>

namespace we::UI {

namespace {

constexpr float kToolbarHeight = 40.0f;
constexpr float kToolbarPadH = 12.0f;
constexpr float kControlHeight = 28.0f;
constexpr float kSearchHeight = 29.0f;
constexpr float kSearchWidth = 360.0f;
constexpr float kIconToggleSize = 26.0f;
constexpr float kGroupGap = 12.0f;

constexpr Color kToolbarText{ 0.816f, 0.816f, 0.816f, 1.0f };

void PaintToolbarButtonChrome(PaintContext& context, const Rect& rect, float hoverAnim, float pressAnim,
    bool selected, bool primary)
{
    const auto& theme = Theme::Get();
    Color bg = Color{ 0.0f, 0.0f, 0.0f, 0.0f };
    if (selected) {
        bg = Color{ 1.0f, 1.0f, 1.0f, 0.06f };
    }
    if (hoverAnim > 0.01f) {
        bg = Color::Lerp(bg, theme.HoverOverlay, hoverAnim * (selected ? 0.5f : 0.85f));
    }
    if (pressAnim > 0.01f) {
        bg = Color::Lerp(bg, theme.PressedOverlay, pressAnim * 0.55f);
    }

    const float radius = 4.0f;
    if (bg.a > 0.01f) {
        context.DrawRoundedRect(rect, bg, radius);
    }

    if (primary) {
        Color border = theme.SelectedAccent;
        border.a = 0.55f + hoverAnim * 0.2f;
        context.DrawRoundedRectOutline(rect, border, 1.0f, radius);
        Color fill = theme.SelectedAccent;
        fill.a = 0.08f + hoverAnim * 0.06f;
        context.DrawRoundedRect(rect, fill, radius);
    } else if (selected) {
        context.DrawRoundedRectOutline(rect, Color{ 1.0f, 1.0f, 1.0f, 0.08f }, 1.0f, radius);
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
    m_DesiredSize = Size{ kIconToggleSize, kControlHeight };
    return m_DesiredSize;
}

void ToolbarIconToggle::Arrange(const Rect& allottedRect) {
    m_Geometry = CenterRect(allottedRect, kIconToggleSize, kIconToggleSize);
}

void ToolbarIconToggle::Paint(PaintContext& context) {
    m_HoverAnim = Animator::Damp(m_HoverAnim, m_Hovered ? 1.0f : 0.0f, 15.0f);
    m_PressAnim = Animator::Damp(m_PressAnim, m_Pressed ? 1.0f : 0.0f, 25.0f);
    PaintToolbarButtonChrome(context, m_Geometry, m_HoverAnim, m_PressAnim, m_Selected, false);

    const float iconSize = 15.0f;
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
    if (!m_IconName.empty()) width += 14.0f + 5.0f;
    width += static_cast<float>(m_Label.size()) * 7.2f;
    if (m_ShowChevron) width += 4.0f + 11.0f;
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
        Color iconColor = m_Variant == Variant::Primary ? Theme::Get().SelectedAccent : Theme::Get().IconMuted;
        IconPainter::DrawIcon(context, m_IconName, Rect{ x, iconY, iconSize, iconSize }, iconColor);
        x += iconSize + 5.0f;
    }

    Color textColor = kToolbarText;
    if (m_Variant == Variant::Primary) {
        textColor = Color::Lerp(kToolbarText, Theme::Get().SelectedAccent, 0.25f);
    }
    context.DrawText(m_Label, Point{ x, textY }, textColor, textSize, m_Variant == Variant::Primary);

    if (m_ShowChevron) {
        const float chevronSize = 11.0f;
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
    m_SearchBox->SetPlaceholder("Search Assets...");
    m_SearchBox->SetWidth(kSearchWidth);

    m_SortBtn = std::make_shared<ToolbarLabeledButton>("Sort", "arrow-up-down", true, ToolbarLabeledButton::Variant::Standard, 10.0f);
    m_FilterBtn = std::make_shared<ToolbarLabeledButton>("Filter", "filter", true, ToolbarLabeledButton::Variant::Standard, 10.0f);
    m_ImportBtn = std::make_shared<ToolbarLabeledButton>("Import", "import", false, ToolbarLabeledButton::Variant::Standard, 12.0f);
    m_CreateBtn = std::make_shared<ToolbarLabeledButton>("Create", Icons::PlusName, false, ToolbarLabeledButton::Variant::Primary, 12.0f);

    AddChild(m_SearchBox);
    const std::shared_ptr<Widget> toolbarChildren[] = {
        m_SortBtn, m_FilterBtn, m_ImportBtn, m_CreateBtn
    };
    for (const auto& w : toolbarChildren) {
        AddChild(w);
    }
}

Size ContentBrowserToolbarControls::Measure(const Size& availableSize) {
    m_DesiredSize = Size{ availableSize.width, kToolbarHeight };
    return m_DesiredSize;
}

void ContentBrowserToolbarControls::ArrangeControlRow(const Rect& row, float contentLeft, float contentRight) {
    const float centerY = row.y + row.height * 0.5f;
    const float contentWidth = std::max(0.0f, contentRight - contentLeft);
    const Size measureSize{ contentWidth, kControlHeight };

    const std::vector<std::shared_ptr<Widget>> actionButtons = {
        m_SortBtn, m_FilterBtn, m_ImportBtn
    };

    float searchWidth = std::min(kSearchWidth, std::max(320.0f, contentWidth * 0.32f));

    float actionsWidth = 0.0f;
    for (const auto& widget : actionButtons) {
        actionsWidth += widget->Measure(measureSize).width;
    }

    const float createWidth = m_CreateBtn->Measure(measureSize).width;
    const float fixedWidth = searchWidth + kGroupGap + actionsWidth + kGroupGap + createWidth;

    if (fixedWidth > contentWidth) {
        searchWidth = std::max(200.0f, contentWidth - (fixedWidth - searchWidth));
    }

    float x = contentLeft;

    m_SearchBox->Arrange(Rect{
        x,
        centerY - kSearchHeight * 0.5f,
        searchWidth,
        kSearchHeight
    });
    x += searchWidth + kGroupGap;

    for (const auto& widget : actionButtons) {
        const Size desired = widget->Measure(measureSize);
        widget->Arrange(Rect{ x, centerY - desired.height * 0.5f, desired.width, desired.height });
        x += desired.width;
    }

    const Size createSize = m_CreateBtn->Measure(measureSize);
    m_CreateBtn->Arrange(Rect{
        contentRight - createSize.width,
        centerY - createSize.height * 0.5f,
        createSize.width,
        createSize.height
    });
}

void ContentBrowserToolbarControls::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
    const float contentLeft = allottedRect.x + kToolbarPadH;
    const float contentRight = allottedRect.x + allottedRect.width - kToolbarPadH;
    ArrangeControlRow(allottedRect, contentLeft, contentRight);
}

void ContentBrowserToolbarControls::Paint(PaintContext& context) {
    const auto& theme = Theme::Get();
    context.DrawRect(m_Geometry, theme.ToolbarBackground);
    context.DrawRect(Rect{ m_Geometry.x, m_Geometry.y + m_Geometry.height - 1.0f, m_Geometry.width, 1.0f },
        theme.Separator);
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
