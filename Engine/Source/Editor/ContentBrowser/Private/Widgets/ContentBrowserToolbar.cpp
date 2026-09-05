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
#include "KindUI/Layout/Spacer.h"
#include "KindUI/Layout/IPopupHost.h"
#include "KindUI/Profiling/UiGeometryDebug.h"
#include "WindEffects/Editor/UI/Layout/EditorMetrics.h"
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
    const float uiScale = (std::max)(1.0f, DPIContext::GetScale());
    const float radius = we::runtime::kindui::ResolveMetric(MetricToken::CornerRadiusSmall) * uiScale;

    Color bgIdle = we::runtime::kindui::ResolveColor(ColorToken::ControlBackground);
    Color bgHover = we::runtime::kindui::ResolveColor(ColorToken::ControlBackgroundHover);
    Color bgPress = we::runtime::kindui::ResolveColor(ColorToken::PressedBackground);
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

    // Main button surface - all corners rounded
    context.DrawRoundedRect(rect, bgColor, radius);

    // Subtle black border from palette around all corners
    Color borderColor = we::runtime::kindui::ResolveColor(ColorToken::Separator);
    if (primary) {
        borderColor = we::runtime::kindui::ResolveColor(ColorToken::AccentPrimary);
    }
    const float borderW = we::runtime::kindui::ResolveMetric(MetricToken::BorderWidth) * uiScale;
    context.DrawRoundedRectOutline(rect, borderColor, borderW, radius);

    // Subtle pressed recessed overlay
    if (pressAnim > 0.01f) {
        Color pressShadow = we::runtime::kindui::ResolveColor(ColorToken::ShadowOverlay);
        pressShadow.a *= pressAnim;
        context.DrawRoundedRect(rect, pressShadow, radius);
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

struct ToolbarMenuItem {
    std::string label;
    bool isSeparator = false;
    bool isChecked = false;
    we::runtime::kindui::WindIconRef icon = we::runtime::kindui::kWindIconNone;
    bool enabled = true;
    std::function<void()> onClick;
};

class ToolbarPopupMenu : public Widget {
public:
    ToolbarPopupMenu(std::vector<ToolbarMenuItem> items, std::function<void()> onDismiss = nullptr)
        : m_Items(std::move(items))
        , m_OnDismiss(std::move(onDismiss))
    {}

    Size Measure(const Size& availableSize) override {
        (void)availableSize;
        const float uiScale = (std::max)(1.0f, DPIContext::GetScale());
        const float itemHeight = ThemeMetric(MetricToken::MenuItemHeight) * uiScale;
        const float menuPad = ThemeMetric(MetricToken::MenuPadding) * uiScale;
        float maxWidth = 190.0f * uiScale;
        for (const auto& item : m_Items) {
            if (!item.isSeparator) {
                float itemW = 50.0f * uiScale + static_cast<float>(item.label.size()) * ThemeMetric(MetricToken::TextSizeSmall) * 0.65f * uiScale;
                maxWidth = std::max(maxWidth, itemW);
            }
        }
        float totalHeight = menuPad * 2.0f;
        for (const auto& item : m_Items) {
            totalHeight += item.isSeparator ? (6.0f * uiScale) : itemHeight;
        }
        m_DesiredSize = Size{ maxWidth, totalHeight };
        return m_DesiredSize;
    }

    void Arrange(const Rect& allottedRect) override {
        m_Geometry = allottedRect;
    }

    void Paint(PaintContext& context) override {
        ControlChrome::PaintPopupSurface(context, m_Geometry);

        const float uiScale = (std::max)(1.0f, DPIContext::GetScale());
        const float itemHeight = ThemeMetric(MetricToken::MenuItemHeight) * uiScale;
        const float menuPad = ThemeMetric(MetricToken::MenuPadding) * uiScale;
        const float padX = ThemeMetric(MetricToken::Space3) * uiScale;
        const float textSize = ThemeMetric(MetricToken::TextSizeSmall) * uiScale;

        float y = m_Geometry.y + menuPad;
        for (size_t i = 0; i < m_Items.size(); ++i) {
            const auto& item = m_Items[i];
            if (item.isSeparator) {
                const float sepY = std::floor(y + 2.0f * uiScale);
                const float sepW = ThemeMetric(MetricToken::BorderWidth) * uiScale;
                context.DrawRect(Rect{ m_Geometry.x + padX, sepY, m_Geometry.width - padX * 2.0f, sepW }, ThemeColor(ColorToken::Separator));
                y += 6.0f * uiScale;
                continue;
            }

            Rect row{ m_Geometry.x + menuPad, y, m_Geometry.width - menuPad * 2.0f, itemHeight };
            if (static_cast<int>(i) == m_Hovered && item.enabled) {
                ControlChrome::InteractionState state{};
                state.hoverAnim = 1.0f;
                ControlChrome::PaintListRow(context, row, state);
            }

            float textLeft = row.x + padX;
            if (item.isChecked) {
                const float checkSize = 16.0f * uiScale;
                Rect checkRect{ textLeft, row.y + (row.height - checkSize) * 0.5f, checkSize, checkSize };
                IconPainter::Draw(context, WindIcons::Check16, checkRect, ThemeColor(ColorToken::TextPrimary));
                textLeft += checkSize + 6.0f * uiScale;
            } else if (item.icon.IsValid()) {
                const float iconSize = 16.0f * uiScale;
                Rect iconRect{ textLeft, row.y + (row.height - iconSize) * 0.5f, iconSize, iconSize };
                IconPainter::Draw(context, item.icon, iconRect, ThemeColor(ColorToken::IconSecondary));
                textLeft += iconSize + 6.0f * uiScale;
            }

            const float textY = row.y + (row.height - textSize) * 0.5f;
            Color textCol = item.enabled ? ThemeColor(ColorToken::TextPrimary) : ThemeColor(ColorToken::TextDisabled);
            context.DrawText(item.label, Point{ textLeft, textY }, textCol, textSize);

            y += itemHeight;
        }
    }

    void OnMouseMove(const MouseEvent& event) override {
        m_Hovered = -1;
        const float uiScale = (std::max)(1.0f, DPIContext::GetScale());
        const float itemHeight = ThemeMetric(MetricToken::MenuItemHeight) * uiScale;
        const float menuPad = ThemeMetric(MetricToken::MenuPadding) * uiScale;
        float y = m_Geometry.y + menuPad;
        for (size_t i = 0; i < m_Items.size(); ++i) {
            const auto& item = m_Items[i];
            if (item.isSeparator) {
                y += 6.0f * uiScale;
                continue;
            }
            Rect row{ m_Geometry.x + menuPad, y, m_Geometry.width - menuPad * 2.0f, itemHeight };
            if (row.Contains(event.position)) {
                m_Hovered = static_cast<int>(i);
                break;
            }
            y += itemHeight;
        }
    }

    void OnMouseDown(const MouseEvent& event) override {
        if (event.button != MouseButton::Left) return;

        const float uiScale = (std::max)(1.0f, DPIContext::GetScale());
        const float itemHeight = ThemeMetric(MetricToken::MenuItemHeight) * uiScale;
        const float menuPad = ThemeMetric(MetricToken::MenuPadding) * uiScale;
        float y = m_Geometry.y + menuPad;
        for (size_t i = 0; i < m_Items.size(); ++i) {
            const auto& item = m_Items[i];
            if (item.isSeparator) {
                y += 6.0f * uiScale;
                continue;
            }
            Rect row{ m_Geometry.x + menuPad, y, m_Geometry.width - menuPad * 2.0f, itemHeight };
            if (row.Contains(event.position) && item.enabled) {
                if (item.onClick) {
                    item.onClick();
                }
                auto* overlay = GetPopupHost();
                if (!overlay) {
                    overlay = ::we::programs::editor::GetEditorPopupHost();
                }
                if (overlay) {
                    overlay->CloseAllPopups();
                }
                if (m_OnDismiss) {
                    m_OnDismiss();
                }
                return;
            }
            y += itemHeight;
        }
    }

    bool ShowsPointerCursor(const Point& position) const override {
        return m_Geometry.Contains(position);
    }

private:
    std::vector<ToolbarMenuItem> m_Items;
    std::function<void()> m_OnDismiss;
    int m_Hovered = -1;
};

} // namespace

ToolbarIconToggle::ToolbarIconToggle(we::runtime::kindui::WindIconRef icon, const char*)
    : m_Icon(icon)
{}

Size ToolbarIconToggle::Measure(const Size& availableSize) {
    (void)availableSize;
    const float uiScale = (std::max)(1.0f, DPIContext::GetScale());
    const float h = 24.0f * uiScale;
    m_DesiredSize = Size{ h, h };
    return m_DesiredSize;
}

void ToolbarIconToggle::Arrange(const Rect& allottedRect) {
    const float uiScale = (std::max)(1.0f, DPIContext::GetScale());
    const float h = std::min(24.0f * uiScale, allottedRect.height);
    m_Geometry = CenterRect(allottedRect, h, h);
}

void ToolbarIconToggle::Paint(PaintContext& context) {
    m_HoverAnim = Animator::Damp(m_HoverAnim, m_Hovered ? 1.0f : 0.0f, 15.0f);
    m_PressAnim = Animator::Damp(m_PressAnim, m_Pressed ? 1.0f : 0.0f, 25.0f);

    const float uiScale = (std::max)(1.0f, DPIContext::GetScale());
    const float radius = we::runtime::kindui::ResolveMetric(MetricToken::CornerRadiusSmall) * uiScale;

    if (!m_Frameless) {
        PaintToolbarButtonChrome(context, m_Geometry, m_HoverAnim, m_PressAnim, m_Selected, false);
    } else {
        // Frameless flat icon button (no idle border, no idle background box)
        if (m_Selected) {
            context.DrawRoundedRect(m_Geometry, we::runtime::kindui::ResolveColor(ColorToken::SelectInactiveBackground), radius);
        } else if (m_HoverAnim > 0.001f || m_PressAnim > 0.001f) {
            Color hoverBg = we::runtime::kindui::ResolveColor(ColorToken::ControlBackgroundHover);
            if (m_PressAnim > 0.001f) {
                hoverBg = Color::Pick(hoverBg, we::runtime::kindui::ResolveColor(ColorToken::PressedBackground), std::clamp(m_PressAnim, 0.0f, 1.0f));
            }
            hoverBg.a *= (std::max)(m_HoverAnim, m_PressAnim);
            context.DrawRoundedRect(m_Geometry, hoverBg, radius);
        }
    }

    const Color iconColor = m_Selected
        ? we::runtime::kindui::ResolveColor(ColorToken::IconActive)
        : (m_HoverAnim > 0.01f
            ? we::runtime::kindui::ResolveColor(ColorToken::IconHover)
            : we::runtime::kindui::ResolveColor(ColorToken::IconSecondary));
    IconPainter::Draw(context, m_Icon, m_Geometry, iconColor);
}

void ToolbarIconToggle::OnMouseDown(const MouseEvent& event) {
    if (event.button == MouseButton::Left) {
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

Size ToolbarLabeledButton::Measure(const Size& availableSize) {
    (void)availableSize;
    const float uiScale = (std::max)(1.0f, DPIContext::GetScale());
    const float hPad = (m_HorizontalPadding > 0.0f
        ? m_HorizontalPadding
        : ThemeMetric(MetricToken::ButtonPaddingHorizontal)) * uiScale;
    const float iconGap = ThemeMetric(MetricToken::Space1) * uiScale;
    const float textSize = ThemeMetric(MetricToken::TextSizeToolbar) * uiScale;

    PaintContext ctx;
    const float textWidth = ctx.GetTextWidth(m_Label, textSize);

    float width = hPad * 2.0f + textWidth;
    if (m_Icon.IsValid()) {
        width += 16.0f * uiScale + iconGap;
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

void ToolbarLabeledButton::Paint(PaintContext& context) {
    m_HoverAnim = Animator::Damp(m_HoverAnim, m_Hovered ? 1.0f : 0.0f, 15.0f);
    m_PressAnim = Animator::Damp(m_PressAnim, m_Pressed ? 1.0f : 0.0f, 25.0f);
    PaintToolbarButtonChrome(context, m_Geometry, m_HoverAnim, m_PressAnim, false, m_Variant == Variant::Primary);

    const float uiScale = (std::max)(1.0f, DPIContext::GetScale());
    const float hPad = (m_HorizontalPadding > 0.0f
        ? m_HorizontalPadding
        : ThemeMetric(MetricToken::ButtonPaddingHorizontal)) * uiScale;
    const float iconGap = ThemeMetric(MetricToken::Space1) * uiScale;
    float x = m_Geometry.x + hPad;
    const float textSize = ThemeMetric(MetricToken::TextSizeToolbar) * uiScale;
    const float textY = LayoutMetrics::AlignTextTopY(m_Geometry, textSize);

    if (m_Icon.IsValid()) {
        const float iconSize = 16.0f * uiScale;
        const float iconY = m_Geometry.y + (m_Geometry.height - iconSize) * 0.5f;
        Rect iconBand{ x, iconY, iconSize, iconSize };

        Color iconColor = ThemeColor(ColorToken::IconSecondary);
        if (m_Variant == Variant::AddAction) {
            iconColor = ThemeColor(ColorToken::Success);
        } else if (m_HoverAnim > 0.01f) {
            iconColor = ThemeColor(ColorToken::IconHover);
        }
        IconPainter::Draw(context, m_Icon, iconBand, iconColor);
        x += iconSize + iconGap;
    }

    Color textColor = ThemeColor(ColorToken::TextPrimary);
    if (m_Variant == Variant::Primary) {
        textColor = Color::Pick(ThemeColor(ColorToken::TextPrimary), ThemeColor(ColorToken::AccentPrimary), 0.25f);
    }
    context.DrawText(m_Label, Point{ x, textY }, textColor, textSize, we::runtime::text::layout::FontWeight::Medium);

    if (m_ShowChevron) {
        const float tier = 12.0f * uiScale;
        const float chevronX = m_Geometry.x + m_Geometry.width - hPad - tier;
        Rect chevronBand{ chevronX, m_Geometry.y + (m_Geometry.height - tier) * 0.5f, tier, tier };
        IconPainter::Draw(context, WindIcons::ChevronDown16, chevronBand, ThemeColor(ColorToken::TextSecondary));
    }
}

void ToolbarLabeledButton::OnMouseDown(const MouseEvent& event) {
    if (event.button == MouseButton::Left) {
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
            auto menu = std::make_shared<ToolbarPopupMenu>(items);
            overlay->ShowPopup(menu, Point{ geom.x, geom.y + geom.height + 2.0f });
        }
    };

    if (m_Mode == ToolbarMode::Full) {
        m_CreateBtn = std::make_shared<ToolbarLabeledButton>("Add", WindIcons::Plus16, false, ToolbarLabeledButton::Variant::AddAction);
        m_ImportBtn = std::make_shared<ToolbarLabeledButton>("Import", WindIcons::FolderCreate16, false, ToolbarLabeledButton::Variant::Standard);
        m_SaveBtn = std::make_shared<ToolbarLabeledButton>("Save All", WindIcons::SaveAll16, false, ToolbarLabeledButton::Variant::Standard);

        m_CreateBtn->SetFlexShrink(0.0f);
        m_ImportBtn->SetFlexShrink(0.0f);
        m_SaveBtn->SetFlexShrink(0.0f);

        AddChild(m_CreateBtn);
        AddChild(m_ImportBtn);
        AddChild(MakeToolbarDivider());
        AddChild(m_SaveBtn);
    } else {
        // Asset pane toolbar: Add, Import, Save All, Filter, Search on left; Settings, Vertical Dots on right
        m_CreateBtn = std::make_shared<ToolbarLabeledButton>("Add", WindIcons::Plus16, false, ToolbarLabeledButton::Variant::AddAction);
        m_ImportBtn = std::make_shared<ToolbarLabeledButton>("Import", WindIcons::FolderCreate16, false, ToolbarLabeledButton::Variant::Standard);
        m_SaveBtn = std::make_shared<ToolbarLabeledButton>("Save All", WindIcons::SaveAll16, false, ToolbarLabeledButton::Variant::Standard);
        
        m_FilterIconBtn = std::make_shared<ToolbarIconToggle>(WindIcons::ListFilter16, "Filter");
        m_FilterIconBtn->SetFrameless(true);

        m_SearchBox = std::make_shared<SearchBox>();
        m_SearchBox->SetPlaceholder("Search Assets...");
        m_SearchBox->SetToolbarInset(true);
        m_SearchBox->SetFillWidth(false);
        const float uiScale = (std::max)(1.0f, DPIContext::GetScale());
        m_SearchBox->SetWidth(320.0f * uiScale);
        m_SearchBox->SetFlexGrow(0.0f);
        m_SearchBox->SetFlexShrink(0.0f);

        m_SettingsBtn = std::make_shared<ToolbarIconToggle>(WindIcons::Settings16, "Settings");
        m_SettingsBtn->SetFrameless(true);
        m_MoreBtn = std::make_shared<ToolbarIconToggle>(WindIcons::EllipsisVertical16, "More Options");
        m_MoreBtn->SetFrameless(true);

        m_CreateBtn->SetFlexShrink(0.0f);
        m_ImportBtn->SetFlexShrink(0.0f);
        m_SaveBtn->SetFlexShrink(0.0f);
        m_FilterIconBtn->SetFlexShrink(0.0f);
        m_SettingsBtn->SetFlexShrink(0.0f);
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

        m_FilterIconBtn->SetOnClicked([this, showMenuBelow]() {
            std::vector<ToolbarMenuItem> items;
            items.push_back({ "All Asset Types", false, true, kWindIconNone, true, nullptr });
            items.push_back({ "", true, false, kWindIconNone, true, nullptr });
            items.push_back({ "Blueprints", false, false, WindIcons::Blueprint16, true, nullptr });
            items.push_back({ "Materials", false, false, WindIcons::ColorPalette16, true, nullptr });
            items.push_back({ "Textures", false, false, WindIcons::ColorFill16, true, nullptr });
            items.push_back({ "Static Meshes", false, false, WindIcons::Box16, true, nullptr });
            items.push_back({ "Sounds", false, false, WindIcons::Speaker16, true, nullptr });

            showMenuBelow(m_FilterIconBtn, items);
            if (m_OnFilterClicked) m_OnFilterClicked();
        });

        auto spacer = std::make_shared<we::runtime::kindui::Spacer>();
        spacer->SetFlexGrow(1.0f);
        spacer->SetFlexShrink(1.0f);

        AddChild(m_CreateBtn);
        AddChild(m_ImportBtn);
        AddChild(m_SaveBtn);
        AddChild(m_FilterIconBtn);
        AddChild(m_SearchBox);
        AddChild(spacer);
        AddChild(m_SettingsBtn);
        AddChild(m_MoreBtn);
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
    m_OnFilterClicked = std::move(callback);
}

void ContentBrowserToolbarControls::SetOnSortClicked(std::function<void()> callback) {
    m_SortBtn->SetOnClicked(std::move(callback));
}

void ContentBrowserToolbarControls::SetOnImportClicked(std::function<void()> callback) {
    if (m_ImportBtn) {
        m_ImportBtn->SetOnClicked(std::move(callback));
    }
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
    if (m_SaveBtn) {
        m_SaveBtn->SetOnClicked(std::move(callback));
    }
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
