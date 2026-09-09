#include "ContentBrowser/Widgets/ContentBrowserToolbar.h"
#include "KindUI/Panel/PanelChrome.h"
#include "KindUI/Core/LayoutMetrics.h"
#include "ContentBrowser/Widgets/SearchBox.h"
#include "ContentBrowser/Widgets/ContentBrowser.h"
#include "Widgets/DropdownMenu.h"
#include "Widgets/MenuBar.h"
#include "KindUI/Core/ControlChrome.h"
#include "KindUI/Core/ToolbarButtonChrome.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Core/Widgets/DesignSystemControls.h"
#include "KindUI/Core/DPIContext.h"
#include "KindUI/Rendering/IconMetrics.h"
#include "KindUI/Theming/ThemeAccess.h"
#include "KindUI/Core/ColorSpace.h"
#include "KindUI/Theming/Palette.h"
#include "KindUI/Theming/PaletteRuntime.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Theming/StyleRole.h"
#include "KindUI/Core/WindIcon.h"
#include "KindUI/Core/Icon.h"
#include "KindUI/Core/Animator.h"
#include "KindUI/Input/InputEvents.h"
#include "KindUI/Core/Widgets/VerticalDivider.h"
#include "KindUI/Layout/Flex.h"
#include "KindUI/Layout/Spacer.h"
#include "KindUI/Layout/IPopupHost.h"
#include "KindUI/Profiling/UiGeometryDebug.h"
#include "WindEffects/Editor/UI/Shell/EditorWorkspaceController.h"
#include <algorithm>

using ::we::runtime::kindui::ColorToken;
using ::we::runtime::kindui::MetricToken;
using ::we::runtime::kindui::PaddingToken;

namespace we::editor::contentbrowser {
using ::we::runtime::kindui::ResolveIconColor;
using ::we::runtime::kindui::MouseButton;
using ::we::runtime::kindui::DPIContext;
namespace LayoutMetrics = ::we::runtime::kindui::LayoutMetrics;
namespace ControlChrome = ::we::runtime::kindui::ControlChrome;
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
namespace PanelChrome = ::we::runtime::kindui::panels::PanelChrome;

namespace {

std::shared_ptr<we::runtime::kindui::VerticalDivider> MakeToolbarDivider() {
    auto divider = std::make_shared<we::runtime::kindui::VerticalDivider>();
    divider->SetFlexShrink(0.0f);
    return divider;
}

void PaintToolbarButtonChrome(PaintContext& context, const Rect& rect, float hoverAnim, float pressAnim,
    bool selected, bool primary)
{
    const float uiScale = (std::max)(1.0f, DPIContext::GetScale());
    const float radius = we::runtime::kindui::ResolveMetric(MetricToken::CornerRadiusSmall) * uiScale;

    Color bgIdle = Color(0.21f, 0.21f, 0.21f, 1.0f);
    Color bgHover = Color(0.27f, 0.27f, 0.27f, 1.0f);
    Color bgPress = Color(0.13f, 0.13f, 0.13f, 1.0f);
    Color bgSelected = we::runtime::kindui::ResolveColor(ColorToken::SelectInactiveBackground);

    Color bgColor = bgIdle;
    if (selected) {
        bgColor = bgSelected;
    } else {
        if (hoverAnim > 0.001f) {
            bgColor = Color::Pick(bgColor, bgHover, std::clamp(hoverAnim, 0.0f, 1.0f));
        }
        if (pressAnim > 0.001f) {
            bgColor = Color::Pick(bgColor, bgPress, std::clamp(pressAnim, 0.0f, 1.0f));
        }
    }

    // Bake press ShadowOverlay into the opaque face — same Src-over result, one less alpha quad.
    if (pressAnim > 0.01f) {
        Color pressShadow = we::runtime::kindui::ResolveColor(ColorToken::ShadowOverlay);
        pressShadow.a *= pressAnim;
        bgColor = we::runtime::kindui::ColorSpace::CompositeSrcOverOpaque(bgColor, pressShadow);
    }

    // Main button surface - all corners rounded
    context.DrawRoundedRect(rect, bgColor, radius);

    // Crisp black border from palette around all corners
    Color borderColor = we::runtime::kindui::palette::GraphiteDarkLive().Black;
    if (primary) {
        borderColor = we::runtime::kindui::ResolveColor(ColorToken::AccentPrimary);
    }
    const float borderW = 1.0f * uiScale;
    context.DrawControlOutline(rect, borderColor, borderW, radius);
}

Rect CenterRect(const Rect& parent, float w, float h) {
    return Rect{
        parent.x + (parent.width - w) * 0.5f,
        parent.y + (parent.height - h) * 0.5f,
        w,
        h
    };
}

struct ToolbarMenuItem {
    std::string label;
    bool isSeparator = false;
    bool isChecked = false;
    we::runtime::kindui::WindIconRef icon = we::runtime::kindui::kWindIconNone;
    bool enabled = true;
    std::function<void()> onClick;
};

} // namespace

ToolbarIconToggle::ToolbarIconToggle(we::runtime::kindui::WindIconRef icon, const char*)
    : m_Icon(icon)
{}

Size ToolbarIconToggle::Measure(const Size& availableSize) {
    (void)availableSize;
    const float uiScale = (std::max)(1.0f, DPIContext::GetScale());
    const float h = ThemeMetric(MetricToken::IconButtonSize) * uiScale;
    m_DesiredSize = Size{ h, h };
    return m_DesiredSize;
}

void ToolbarIconToggle::Arrange(const Rect& allottedRect) {
    const float uiScale = (std::max)(1.0f, DPIContext::GetScale());
    const float h = std::min(ThemeMetric(MetricToken::IconButtonSize) * uiScale, allottedRect.height);
    m_Geometry = CenterRect(allottedRect, h, h);
}

void ToolbarIconToggle::Tick(float deltaTime) {
    (void)deltaTime;
    const bool enabled = IsEnabled();
    m_HoverAnim = Animator::Damp(m_HoverAnim, (enabled && m_Hovered) ? 1.0f : 0.0f, 15.0f);
    m_PressAnim = Animator::Damp(m_PressAnim, (enabled && m_Pressed) ? 1.0f : 0.0f, 25.0f);
    Widget::Tick(deltaTime);
}

void ToolbarIconToggle::Paint(PaintContext& context) {
    const bool enabled = IsEnabled();

    // Framed primary actions keep chrome; icon-only toggles stay floating glyphs.
    if (!m_Frameless) {
        PaintToolbarButtonChrome(context, m_Geometry, m_HoverAnim, m_PressAnim, m_Selected, false);
    }

    if (!m_Icon.IsValid()) {
        return;
    }

    const float iconPx = static_cast<float>(m_Icon.sizePx > 0 ? m_Icon.sizePx : 16u);
    if (!enabled) {
        Color disabled = we::runtime::kindui::ToolbarButtonChrome::ResolveIconColor(0.0f, 0.0f, false);
        disabled.a = 0.35f;
        IconPainter::Draw(context, m_Icon, m_Geometry, static_cast<uint32_t>(iconPx), disabled);
        return;
    }

    we::runtime::kindui::ToolbarButtonChrome::PaintFloatingIcon(
        context,
        m_Icon,
        m_Geometry,
        iconPx,
        m_HoverAnim,
        m_PressAnim,
        m_Selected);
}

void ToolbarIconToggle::OnMouseDown(const MouseEvent& event) {
    if (event.button == MouseButton::Left && IsEnabled()) {
        m_Pressed = true;
        if (m_OnClicked) {
            m_OnClicked();
        }
    }
}

void ToolbarIconToggle::OnMouseUp(const MouseEvent& event) {
    if (event.button == MouseButton::Left) {
        m_Pressed = false;
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

void ToolbarLabeledButton::UpdateTextMetrics(const float textSize) const {
    if (m_CachedTextSize == textSize) {
        return;
    }
    PaintContext ctx;
    m_CachedTextWidth = ctx.GetTextWidth(m_Label, textSize);
    m_CachedTextSize = textSize;
}

Size ToolbarLabeledButton::Measure(const Size& availableSize) {
    (void)availableSize;
    const float uiScale = (std::max)(1.0f, DPIContext::GetScale());
    const float hPad = (m_HorizontalPadding > 0.0f
        ? m_HorizontalPadding
        : ThemeMetric(MetricToken::ButtonPaddingHorizontal)) * uiScale;
    const float iconGap = ThemeMetric(MetricToken::Space1) * uiScale;
    const float textSize = ThemeMetric(MetricToken::TextSizeToolbar) * uiScale;

    UpdateTextMetrics(textSize);
    const float textWidth = m_CachedTextWidth;

    float width = hPad * 2.0f + textWidth;
    if (m_Icon.IsValid()) {
        width += ThemeMetric(MetricToken::IconSizeToolbar) * uiScale + iconGap;
    }
    if (m_ShowChevron) {
        width += iconGap + 12.0f * uiScale;
    }
    const float h = ThemeMetric(MetricToken::ToolbarLabeledHeight) * uiScale;
    m_DesiredSize = Size{ width, h };
    return m_DesiredSize;
}

void ToolbarLabeledButton::Arrange(const Rect& allottedRect) {
    const float uiScale = (std::max)(1.0f, DPIContext::GetScale());
    const float h = std::min(ThemeMetric(MetricToken::ToolbarLabeledHeight) * uiScale, allottedRect.height);
    m_Geometry = Rect{
        allottedRect.x,
        allottedRect.y + (allottedRect.height - h) * 0.5f,
        allottedRect.width,
        h
    };
}

void ToolbarLabeledButton::Tick(float deltaTime) {
    (void)deltaTime;
    const bool enabled = IsEnabled();
    m_HoverAnim = Animator::Damp(m_HoverAnim, (enabled && m_Hovered) ? 1.0f : 0.0f, 15.0f);
    m_PressAnim = Animator::Damp(m_PressAnim, (enabled && m_Pressed) ? 1.0f : 0.0f, 25.0f);
    Widget::Tick(deltaTime);
}

void ToolbarLabeledButton::Paint(PaintContext& context) {
    const bool enabled = IsEnabled();
    const float uiScale = (std::max)(1.0f, DPIContext::GetScale());

    if (!m_Frameless) {
        PaintToolbarButtonChrome(context, m_Geometry, m_HoverAnim, m_PressAnim, false, m_Variant == Variant::Primary);
    }

    const Color kHighlightColor = Color(0.8392f, 0.8510f, 0.8667f, 1.0f); // #D6D9DD
    const float hPad = (m_HorizontalPadding > 0.0f
        ? m_HorizontalPadding
        : ThemeMetric(MetricToken::ButtonPaddingHorizontal)) * uiScale;
    const float iconGap = ThemeMetric(MetricToken::Space1) * uiScale;
    float x = m_Geometry.x + hPad;
    const float textSize = ThemeMetric(MetricToken::TextSizeToolbar) * uiScale;
    const float textY = LayoutMetrics::AlignTextTopY(m_Geometry, textSize);
    UpdateTextMetrics(textSize);

    if (m_Icon.IsValid()) {
        const float iconSize = ThemeMetric(MetricToken::IconSizeToolbar) * uiScale;
        const float iconY = m_Geometry.y + (m_Geometry.height - iconSize) * 0.5f;
        Rect iconBand{ x, iconY, iconSize, iconSize };

        if (!enabled) {
            Color iconColor = we::runtime::kindui::ToolbarButtonChrome::ResolveIconColor(0.0f, 0.0f, false);
            iconColor.a = 0.35f;
            IconPainter::Draw(context, m_Icon, iconBand, static_cast<uint32_t>(iconSize), iconColor);
        } else if (m_Variant == Variant::AddAction) {
            IconPainter::Draw(
                context,
                m_Icon,
                iconBand,
                static_cast<uint32_t>(iconSize),
                ThemeColor(ColorToken::Success));
        } else {
            we::runtime::kindui::ToolbarButtonChrome::PaintFloatingIcon(
                context,
                m_Icon,
                iconBand,
                iconSize,
                m_HoverAnim,
                m_PressAnim,
                false);
        }
        x += iconSize + iconGap;
    }

    Color textColor = ThemeColor(ColorToken::TextSecondary);
    if (!enabled) {
        textColor = ThemeColor(ColorToken::TextDisabled);
    } else if (m_Variant == Variant::Primary) {
        textColor = Color::Pick(ThemeColor(ColorToken::TextPrimary), ThemeColor(ColorToken::AccentPrimary), 0.25f);
    } else if (m_Variant == Variant::AddAction) {
        textColor = ThemeColor(ColorToken::TextPrimary);
    } else if (m_HoverAnim > 0.01f || m_PressAnim > 0.01f) {
        float t = (std::max)(m_HoverAnim, m_PressAnim);
        textColor = Color::Pick(textColor, kHighlightColor, std::clamp(t, 0.0f, 1.0f));
    }
    context.DrawText(m_Label, Point{ x, textY }, textColor, textSize, we::runtime::text::layout::FontWeight::Regular);

    if (m_ShowChevron) {
        const float tier = 12.0f * uiScale;
        const float chevronX = m_Geometry.x + m_Geometry.width - hPad - tier;
        Rect chevronBand{ chevronX, m_Geometry.y + (m_Geometry.height - tier) * 0.5f, tier, tier };
        IconPainter::Draw(context, WindIcons::ChevronDownV212, chevronBand, ThemeColor(ColorToken::TextSecondary));
    }
}

void ToolbarLabeledButton::OnMouseDown(const MouseEvent& event) {
    if (event.button == MouseButton::Left && IsEnabled()) {
        m_Pressed = true;
        if (m_OnClicked) {
            m_OnClicked();
        }
    }
}

void ToolbarLabeledButton::OnMouseUp(const MouseEvent& event) {
    if (event.button == MouseButton::Left) {
        m_Pressed = false;
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
    const float padH = ThemeMetric(MetricToken::Space2);
    Padding(Margin{padH, padV, padH + 6.0f, padV});
    Gap(ThemeMetric(MetricToken::Space1));
    Align(AlignItems::Center);
}

void ContentBrowserToolbarControls::InitializeChildren() {
    auto showMenuBelow = [this](const std::shared_ptr<Widget>& anchor, const std::vector<ToolbarMenuItem>& items) {
        if (!anchor) return;
        auto* overlay = GetPopupHost();
        if (!overlay) {
            overlay = ::we::programs::editor::GetEditorPopupHost();
        }
        if (overlay) {
            overlay->CloseAllPopups();
            const Rect geom = anchor->GetGeometry();
            std::vector<std::shared_ptr<::we::editor::menus::MenuItem>> menuItems;
            for (const auto& item : items) {
                auto mi = std::make_shared<::we::editor::menus::MenuItem>();
                if (item.isSeparator) {
                    mi->label = "";
                } else {
                    mi->label = item.label;
                    mi->icon = item.icon;
                    mi->checked = item.isChecked;
                    mi->enabled = item.enabled;
                    mi->onClick = item.onClick;
                }
                menuItems.push_back(mi);
            }
            auto menu = std::make_shared<::we::editor::menus::DropdownMenu>(menuItems);
            overlay->ShowPopup(menu, Point{ geom.x, geom.y + geom.height + 2.0f });
        }
    };

    m_CreateBtn = std::make_shared<ToolbarLabeledButton>("Add", WindIcons::Plus16, false, ToolbarLabeledButton::Variant::AddAction);
    m_CreateBtn->SetFrameless(false);
    m_CreateBtn->SetFlexShrink(0.0f);

    m_ImportBtn = std::make_shared<ToolbarLabeledButton>("Import", WindIcons::Import16, false, ToolbarLabeledButton::Variant::Standard);
    m_ImportBtn->SetFrameless(true);
    m_ImportBtn->SetFlexShrink(0.0f);

    m_SaveBtn = std::make_shared<ToolbarLabeledButton>("Save All", WindIcons::SaveAll16, false, ToolbarLabeledButton::Variant::Standard);
    m_SaveBtn->SetFrameless(true);
    m_SaveBtn->SetFlexShrink(0.0f);

    m_ImportBtn->SetOnClicked([this]() {
        if (m_OnImportClicked) m_OnImportClicked();
    });

    m_SaveBtn->SetOnClicked([this]() {
        if (m_OnSaveClicked) m_OnSaveClicked();
    });

    m_BackBtn = std::make_shared<ToolbarIconToggle>(WindIcons::CircleArrowLeft16, "Back");
    m_BackBtn->SetFrameless(true);
    m_BackBtn->SetFlexShrink(0.0f);

    m_ForwardBtn = std::make_shared<ToolbarIconToggle>(WindIcons::CircleArrowRight16, "Forward");
    m_ForwardBtn->SetFrameless(true);
    m_ForwardBtn->SetFlexShrink(0.0f);

    m_FolderBtn = std::make_shared<ToolbarIconToggle>(WindIcons::Folder16, "Folder");
    m_FolderBtn->SetFrameless(true);
    m_FolderBtn->SetFlexShrink(0.0f);

    m_Breadcrumb = std::make_shared<Breadcrumb>();
    m_Breadcrumb->SetFlexShrink(0.0f);
    m_Breadcrumb->SetPath({ "All", "Content" });

    m_SettingsBtn = std::make_shared<ToolbarIconToggle>(WindIcons::Settings16, "Settings");
    m_SettingsBtn->SetFrameless(true);
    m_SettingsBtn->SetFlexShrink(0.0f);

    m_MoreBtn = std::make_shared<ToolbarIconToggle>(WindIcons::EllipsisVertical16, "More Options");
    m_MoreBtn->SetFrameless(true);
    m_MoreBtn->SetFlexShrink(0.0f);

    m_SettingsBtn->SetOnClicked([this, showMenuBelow]() {
        std::vector<ToolbarMenuItem> items;
        items.push_back({ "Tiles View", false, false, WindIcons::Grid16, true, [this]() {
            if (m_OnViewModeChanged) m_OnViewModeChanged(ContentViewMode::Tiles);
        }});
        items.push_back({ "List View", false, false, WindIcons::ListFilter16, true, [this]() {
            if (m_OnViewModeChanged) m_OnViewModeChanged(ContentViewMode::List);
        }});
        items.push_back({ "Large Icons", false, false, WindIcons::Square16, true, [this]() {
            if (m_OnViewModeChanged) m_OnViewModeChanged(ContentViewMode::LargeIcons);
        }});
        items.push_back({ "Medium Icons", false, false, WindIcons::Square16, true, [this]() {
            if (m_OnViewModeChanged) m_OnViewModeChanged(ContentViewMode::MediumIcons);
        }});
        items.push_back({ "Small Icons", false, false, WindIcons::Square16, true, [this]() {
            if (m_OnViewModeChanged) m_OnViewModeChanged(ContentViewMode::SmallIcons);
        }});
        items.push_back({ "", true, false, kWindIconNone, true, nullptr });
        items.push_back({ "Show Folders", false, true, kWindIconNone, true, nullptr });
        items.push_back({ "Show Hidden Assets", false, false, kWindIconNone, true, nullptr });
        items.push_back({ "Show Engine Content", false, false, kWindIconNone, true, nullptr });
        items.push_back({ "Show Plugin Content", false, false, kWindIconNone, true, nullptr });

        showMenuBelow(m_SettingsBtn, items);
        if (m_OnSettingsClicked) m_OnSettingsClicked();
    });

    m_MoreBtn->SetOnClicked([this, showMenuBelow]() {
        std::vector<ToolbarMenuItem> items;
        items.push_back({ "Refresh", false, false, WindIcons::Refresh16, true, nullptr });
        items.push_back({ "Expand All", false, false, WindIcons::ChevronDown16, true, nullptr });
        items.push_back({ "Collapse All", false, false, WindIcons::ChevronUp16, true, nullptr });
        items.push_back({ "", true, false, kWindIconNone, true, nullptr });
        items.push_back({ "Dock in Layout", false, false, WindIcons::Window16, true, nullptr });
        items.push_back({ "Open in New Tab", false, false, WindIcons::Plus16, true, nullptr });

        showMenuBelow(m_MoreBtn, items);
        if (m_OnMoreClicked) m_OnMoreClicked();
    });

    m_CreateBtn->SetOnClicked([this, showMenuBelow]() {
        std::vector<ToolbarMenuItem> items;
        items.push_back({ "Import Asset...", false, false, WindIcons::FolderCreate16, true, [this]() {
            if (m_OnImportClicked) m_OnImportClicked();
        }});
        items.push_back({ "", true, false, kWindIconNone, true, nullptr });
        items.push_back({ "New Folder", false, false, WindIcons::FolderCreate16, true, nullptr });
        items.push_back({ "", true, false, kWindIconNone, true, nullptr });
        items.push_back({ "Blueprint Class", false, false, WindIcons::Blueprint16, true, nullptr });
        items.push_back({ "Material", false, false, WindIcons::ColorPalette16, true, nullptr });
        items.push_back({ "Particle System", false, false, WindIcons::Sun16, true, nullptr });
        items.push_back({ "Sound Cue", false, false, WindIcons::Speaker16, true, nullptr });
        items.push_back({ "Level", false, false, WindIcons::Globe16, true, nullptr });

        showMenuBelow(m_CreateBtn, items);
        if (m_OnCreateClicked) m_OnCreateClicked();
    });

    auto spacer = std::make_shared<we::runtime::kindui::Spacer>();
    spacer->SetFlexGrow(1.0f);
    spacer->SetFlexShrink(1.0f);

    AddChild(m_CreateBtn);
    AddChild(m_ImportBtn);
    AddChild(m_SaveBtn);
    AddChild(m_BackBtn);
    AddChild(m_ForwardBtn);
    AddChild(m_FolderBtn);
    AddChild(m_Breadcrumb);
    AddChild(spacer);
    AddChild(m_SettingsBtn);
    AddChild(m_MoreBtn);
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

    // Existing background separator separating toolbar from content
    const float uiScale = (std::max)(1.0f, DPIContext::GetScale());
    const float thickness = ThemeMetric(MetricToken::PanelDividerWidth) * uiScale;
    const Rect bottomBorder{ m_Geometry.x, m_Geometry.y + m_Geometry.height - thickness, m_Geometry.width, thickness };
    context.DrawSurface(bottomBorder, we::runtime::kindui::SurfaceRole::Separator, 0.0f, "ContentBrowserToolbarSeparator");

    if (we::runtime::kindui::UiGeometryDebug::IsEnabled()) {
        we::runtime::kindui::UiGeometryDebug::Get().TraceRegion(
            "ContentBrowserToolbar",
            m_Geometry,
            "ContentBrowser",
            we::runtime::kindui::ResolveMetric(we::runtime::kindui::MetricToken::Space2)
                * (std::max)(1.0f, DPIContext::GetScale()),
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
    m_OnFilterClicked = std::move(callback);
}

void ContentBrowserToolbarControls::SetOnSortClicked(std::function<void()> callback) {
    m_SortBtn->SetOnClicked(std::move(callback));
}

void ContentBrowserToolbarControls::SetOnImportClicked(std::function<void()> callback) {
    m_OnImportClicked = std::move(callback);
}

void ContentBrowserToolbarControls::SetOnCreateClicked(std::function<void()> callback) {
    m_OnCreateClicked = std::move(callback);
}

void ContentBrowserToolbarControls::SetOnViewModeChanged(std::function<void(ContentViewMode)> callback) {
    m_OnViewModeChanged = std::move(callback);
}

void ContentBrowserToolbarControls::SetOnSettingsClicked(std::function<void()> callback) {
    m_OnSettingsClicked = std::move(callback);
}

void ContentBrowserToolbarControls::SetOnMoreClicked(std::function<void()> callback) {
    m_OnMoreClicked = std::move(callback);
}

void ContentBrowserToolbarControls::SetOnSaveClicked(std::function<void()> callback) {
    m_OnSaveClicked = std::move(callback);
}

void ContentBrowserToolbarControls::SetOnFabClicked(std::function<void()> callback) {
    if (m_FabBtn) {
        m_FabBtn->SetOnClicked(std::move(callback));
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
 
