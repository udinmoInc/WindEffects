#include "ContentBrowser/Widgets/ContentBrowserToolbar.h"
#include "WindEffects/Editor/UI/Panel/PanelChrome.h"
#include "KindUI/Core/LayoutMetrics.h"
#include "ContentBrowser/Widgets/SearchBox.h"
#include "ContentBrowser/Widgets/ContentBrowser.h"
#include "KindUI/Core/ControlChrome.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Core/Widgets/DesignSystemControls.h"
#include "KindUI/Core/DPIContext.h"
#include "KindUI/Rendering/IconMetrics.h"
#include "KindUI/Theming/ThemeAccess.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Theming/StyleRole.h"
#include "KindUI/Core/WindIcon.h"
#include "KindUI/Core/Icon.h"
#include "KindUI/Core/Animator.h"
#include "KindUI/Input/InputEvents.h"
#include "KindUI/Core/Widgets/VerticalDivider.h"
#include "KindUI/Layout/Flex.h"
#include "KindUI/Profiling/UiGeometryDebug.h"
#include "WindEffects/Editor/UI/Layout/EditorMetrics.h"
#include <algorithm>

using ::we::runtime::kindui::ColorToken;
using ::we::runtime::kindui::MetricToken;
using ::we::runtime::kindui::PaddingToken;

namespace we::editor::contentbrowser {
using ::we::runtime::kindui::ResolveIconColor;
using ::we::runtime::kindui::MouseButton;
using ::we::runtime::kindui::IconPainter;
using ::we::runtime::kindui::Row;
using ::we::runtime::kindui::Margin;
using ::we::runtime::kindui::AlignItems;
namespace WindIcons = ::we::runtime::kindui::WindIcons;
using ::we::runtime::kindui::kWindIconNone;
namespace IconMetrics = ::we::runtime::kindui::IconMetrics;
using ::we::runtime::kindui::MakePrimaryAction;
using ::we::runtime::kindui::MakeSecondaryAction;
using ::we::runtime::kindui::Animator;
using ::we::runtime::kindui::IconColorRole;
using ::we::editor::widgets::SearchBox;
namespace PanelChrome = ::we::editor::panels::PanelChrome;

namespace {

std::shared_ptr<we::runtime::kindui::VerticalDivider> MakeToolbarDivider() {
    auto divider = std::make_shared<we::runtime::kindui::VerticalDivider>();
    divider->SetFlexShrink(0.0f);
    return divider;
}

void PaintToolbarButtonChrome(PaintContext& context, const Rect& rect, float hoverAnim, float pressAnim,
    bool selected, bool primary)
{
    const float radius = we::runtime::kindui::ResolveMetric(MetricToken::CornerRadiusSmall);
    we::runtime::kindui::ControlChrome::PaintInteractiveFill(
        context,
        rect,
        radius,
        hoverAnim,
        pressAnim,
        selected,
        ColorToken::ControlBackground);

    if (primary) {
        context.DrawRoundedRectOutline(
            rect,
            we::runtime::kindui::ResolveColor(ColorToken::AccentPrimary),
            we::runtime::kindui::ResolveMetric(MetricToken::BorderWidth),
            radius);
    } else if (selected) {
        context.DrawRoundedRectOutline(
            rect,
            we::runtime::kindui::ResolveColor(ColorToken::PressedBackground),
            we::runtime::kindui::ResolveMetric(MetricToken::BorderWidth),
            radius);
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

ToolbarIconToggle::ToolbarIconToggle(we::runtime::kindui::WindIconRef icon, const char*)
    : m_Icon(icon)
{}

Size ToolbarIconToggle::Measure(const Size& availableSize) {
    (void)availableSize;
    const float h = ThemeMetric(MetricToken::ButtonHeight);
    m_DesiredSize = Size{ h, h };
    return m_DesiredSize;
}

void ToolbarIconToggle::Arrange(const Rect& allottedRect) {
    const float h = ThemeMetric(MetricToken::ButtonHeight);
    m_Geometry = CenterRect(allottedRect, h, h);
}

void ToolbarIconToggle::Paint(PaintContext& context) {
    m_HoverAnim = Animator::Damp(m_HoverAnim, m_Hovered ? 1.0f : 0.0f, 15.0f);
    m_PressAnim = Animator::Damp(m_PressAnim, m_Pressed ? 1.0f : 0.0f, 25.0f);
    PaintToolbarButtonChrome(context, m_Geometry, m_HoverAnim, m_PressAnim, m_Selected, false);

    const float iconSize = ThemeMetric(MetricToken::IconSizeToolbar);
    IconPainter::Draw(context, m_Icon, m_Geometry, static_cast<uint32_t>(iconSize));
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

ToolbarLabeledButton::ToolbarLabeledButton(const std::string& label, we::runtime::kindui::WindIconRef icon,
    bool showChevron, Variant variant, float horizontalPadding)
    : m_Label(label)
    , m_Icon(icon)
    , m_ShowChevron(showChevron)
    , m_Variant(variant)
    , m_HorizontalPadding(horizontalPadding)
{}

Size ToolbarLabeledButton::Measure(const Size& availableSize) {
    (void)availableSize;
    float width = m_HorizontalPadding * 2.0f;
    if (m_Icon.IsValid()) width += ThemeMetric(MetricToken::IconSizeToolbar) + ThemeMetric(MetricToken::Space1) + 1.0f;
    width += static_cast<float>(m_Label.size()) * 7.2f;
    if (m_ShowChevron) width += ThemeMetric(MetricToken::Space2) + static_cast<float>(16u);
    m_DesiredSize = Size{ width, ThemeMetric(MetricToken::ButtonHeight) };
    return m_DesiredSize;
}

void ToolbarLabeledButton::Arrange(const Rect& allottedRect) {
    const float h = std::min(ThemeMetric(MetricToken::ButtonHeight), allottedRect.height);
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
    const float textSize = ThemeMetric(MetricToken::TextSizeBody);
    const float textY = m_Geometry.y + (m_Geometry.height - textSize) * 0.5f;

    if (m_Icon.IsValid()) {
        const float iconSize = ThemeMetric(MetricToken::IconSizeToolbar);
        Rect iconBand{ x, m_Geometry.y, iconSize, m_Geometry.height };
        IconPainter::Draw(context, m_Icon, iconBand, static_cast<uint32_t>(iconSize));
        x += iconSize + ThemeMetric(MetricToken::Space2);
    }

    Color textColor = ThemeColor(ColorToken::TextPrimary);
    if (m_Variant == Variant::Primary) {
        textColor = Color::Pick(ThemeColor(ColorToken::TextPrimary), ThemeColor(ColorToken::AccentPrimary), 0.25f);
    }
    context.DrawText(m_Label, Point{ x, textY }, textColor, textSize, m_Variant == Variant::Primary);

    if (m_ShowChevron) {
        const float tier = ThemeMetric(MetricToken::IconSizeToolbar);
        const float chevronX = m_Geometry.x + m_Geometry.width - hPad - tier;
        Rect chevronBand{ chevronX, m_Geometry.y, tier, m_Geometry.height };
        IconPainter::Draw(context, WindIcons::ChevronDown16, chevronBand, static_cast<uint32_t>(tier));
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
    : Row()
    , m_Mode(mode)
{
    const float padV = ThemeMetric(MetricToken::Space1);
    Padding(Margin{0.0f, padV, 0.0f, padV});
    Gap(ThemeMetric(MetricToken::Space1));
    Align(AlignItems::Center);
}

void ContentBrowserToolbarControls::InitializeChildren() {
    if (m_Mode == ToolbarMode::Full) {
        m_CreateBtn = MakePrimaryAction("Add", WindIcons::Plus16);
        m_ImportBtn = MakeSecondaryAction("Import", WindIcons::FolderPlus16);
        m_SaveBtn = std::make_shared<ToolbarLabeledButton>("Save All", WindIcons::Check16, false, ToolbarLabeledButton::Variant::Standard, ThemeMetric(MetricToken::Space3));

        m_CreateBtn->SetFlexShrink(0.0f);
        m_ImportBtn->SetFlexShrink(0.0f);
        m_SaveBtn->SetFlexShrink(0.0f);

        AddChild(m_CreateBtn);
        AddChild(m_ImportBtn);
        AddChild(MakeToolbarDivider());
        AddChild(m_SaveBtn);
    } else {
        // Asset pane toolbar: add, import, search, save all, filter icon
        m_CreateBtn = MakePrimaryAction("Add", WindIcons::Plus16);
        m_ImportBtn = MakeSecondaryAction("Import", WindIcons::FolderPlus16);
        m_SearchBox = std::make_shared<SearchBox>();
        m_SearchBox->SetPlaceholder("Search Assets...");
        m_SearchBox->SetToolbarInset(true);
        m_SearchBox->SetFillWidth(false);
        m_SearchBox->SetWidth(ThemeMetric(MetricToken::Space6) * 8.0f);
        m_SearchBox->SetFlexGrow(0.0f);
        m_SearchBox->SetFlexShrink(0.0f);

        m_SaveBtn = std::make_shared<ToolbarLabeledButton>("Save All", WindIcons::Check16, false, ToolbarLabeledButton::Variant::Standard, ThemeMetric(MetricToken::Space3));
        m_FilterIconBtn = std::make_shared<ToolbarIconToggle>(WindIcons::ListFilter16, "Filter");

        m_CreateBtn->SetFlexShrink(0.0f);
        m_ImportBtn->SetFlexShrink(0.0f);
        m_SaveBtn->SetFlexShrink(0.0f);
        m_FilterIconBtn->SetFlexShrink(0.0f);

        AddChild(m_CreateBtn);
        AddChild(m_ImportBtn);
        AddChild(m_SearchBox);
        AddChild(m_SaveBtn);
        AddChild(m_FilterIconBtn);
    }
}

Size ContentBrowserToolbarControls::Measure(const Size& availableSize) {
    Size size = Row::Measure(availableSize);
    size.height = PanelChrome::ToolbarRowHeight();
    m_DesiredSize = size;
    return m_DesiredSize;
}

void ContentBrowserToolbarControls::ArrangeControlRow(const Rect& row, float contentLeft, float contentRight) {
    Row::Arrange(row);
}

void ContentBrowserToolbarControls::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
    Row::Arrange(allottedRect);
}

void ContentBrowserToolbarControls::Paint(PaintContext& context) {
    Row::Paint(context);

    if (we::runtime::kindui::UiGeometryDebug::IsEnabled()) {
        we::runtime::kindui::UiGeometryDebug::Get().TraceRegion(
            "ContentBrowserToolbar",
            m_Geometry,
            "ContentBrowser",
            we::editor::layout::EditorMetrics::Scaled(we::runtime::kindui::MetricToken::Space2),
            0.0f,
            we::runtime::kindui::ResolveMetric(we::runtime::kindui::MetricToken::TextSizeSmall),
            we::runtime::kindui::ResolveMetric(we::runtime::kindui::MetricToken::IconSizeToolbar));
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

} // namespace we::editor::contentbrowser
