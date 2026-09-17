// ==============================================================================
// WindEffects — ContentBrowser — ContentBrowserToolbar
// UI widget used by the ContentBrowser module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "ContentBrowser/Widgets/ContentBrowserToolbar.h"
#include <KindUI/EditorUI.h>
#include "KindUI/Diagnostics/UiGeometryDebug.h"
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
using ::we::runtime::kindui::SearchBoxControl;
using ::we::runtime::kindui::Breadcrumb;
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

    Color bgIdle = we::runtime::kindui::ResolveColor(ColorToken::ControlBackground);
    Color bgHover = we::runtime::kindui::ResolveColor(ColorToken::HoverBackground);
    Color bgPress = we::runtime::kindui::ResolveColor(ColorToken::InputBackground);
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
    if (pressAnim > 0.01f) {
        Color pressShadow = we::runtime::kindui::ResolveColor(ColorToken::ShadowOverlay);
        pressShadow.a *= pressAnim;
        bgColor = we::runtime::kindui::ColorSpace::CompositeSrcOverOpaque(bgColor, pressShadow);
    }

    // Main button surface - all corners rounded
    context.DrawRoundedRect(rect, bgColor, radius);

    // Crisp outline around all corners
    Color borderColor = we::runtime::kindui::ResolveColor(ColorToken::BorderDefault);
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
    const float uiScale = (std::max)(1.0f, DPIContext::GetScale());
    const float padH = ThemeMetric(MetricToken::Space3) * uiScale;
    Padding(Margin{padH, 0.0f, padH, 0.0f});
    Gap(ThemeMetric(MetricToken::Space2) * uiScale);
    Align(AlignItems::Center);
}

void ContentBrowserToolbarControls::InitializeChildren() {
    auto showDropdownMenuBelow = [this](const std::shared_ptr<Widget>& anchor,
        const std::vector<std::shared_ptr<::we::runtime::kindui::MenuItem>>& items) {
        if (!anchor) return;
        auto* overlay = GetPopupHost();
        if (!overlay) {
            overlay = ::we::programs::editor::GetEditorPopupHost();
        }
        if (overlay) {
            overlay->CloseAllPopups();
            const Rect geom = anchor->GetGeometry();
            auto menu = std::make_shared<::we::runtime::kindui::DropdownMenu>(items);
            overlay->ShowAnchoredPopup(
                menu, geom, ::we::runtime::kindui::PopupPlacementMode::BottomPreferred);
        }
    };

    m_CreateBtn = std::make_shared<ToolbarButton>("Add", WindIcons::Plus16);
    m_CreateBtn->SetFlexShrink(0.0f);

    m_ImportBtn = std::make_shared<ToolbarButton>("Import", WindIcons::Import16);
    m_ImportBtn->SetFlexShrink(0.0f);

    m_SaveBtn = std::make_shared<ToolbarButton>("Save All", WindIcons::SaveAll16);
    m_SaveBtn->SetFlexShrink(0.0f);

    m_ImportBtn->SetOnClicked([this]() {
        if (m_OnImportClicked) m_OnImportClicked();
    });

    m_SaveBtn->SetOnClicked([this]() {
        if (m_OnSaveClicked) m_OnSaveClicked();
    });

    m_BackBtn = std::make_shared<ToolbarIconButton>(WindIcons::CircleArrowLeft16, "Back");
    m_BackBtn->SetFlexShrink(0.0f);

    m_ForwardBtn = std::make_shared<ToolbarIconButton>(WindIcons::CircleArrowRight16, "Forward");
    m_ForwardBtn->SetFlexShrink(0.0f);

    m_FolderBtn = std::make_shared<ToolbarIconButton>(WindIcons::Folder16, "Folder");
    m_FolderBtn->SetFlexShrink(0.0f);

    m_Breadcrumb = std::make_shared<Breadcrumb>();
    m_Breadcrumb->SetFlexGrow(1.0f);
    m_Breadcrumb->SetFlexShrink(1.0f);
    m_Breadcrumb->SetPath({ "All", "Content" });

    m_SettingsBtn = std::make_shared<ToolbarIconButton>(WindIcons::Settings16, "Settings");
    m_SettingsBtn->SetFlexShrink(0.0f);

    m_MoreBtn = std::make_shared<ToolbarIconButton>(WindIcons::EllipsisVertical16, "More Options");
    m_MoreBtn->SetFlexShrink(0.0f);

    m_SettingsBtn->SetOnClicked([this, showDropdownMenuBelow]() {
        std::vector<std::shared_ptr<::we::runtime::kindui::MenuItem>> items;

        // View Modes Submenu
        auto viewModesItem = std::make_shared<::we::runtime::kindui::MenuItem>();
        viewModesItem->label = "View Modes";
        viewModesItem->icon = WindIcons::Grid16;

        auto tilesItem = std::make_shared<::we::runtime::kindui::MenuItem>();
        tilesItem->label = "Tiles View";
        tilesItem->icon = WindIcons::Grid16;
        tilesItem->onClick = [this]() {
            if (m_OnViewModeChanged) m_OnViewModeChanged(ContentViewMode::Tiles);
        };
        viewModesItem->submenu.push_back(tilesItem);

        auto listItem = std::make_shared<::we::runtime::kindui::MenuItem>();
        listItem->label = "List View";
        listItem->icon = WindIcons::ListFilter16;
        listItem->onClick = [this]() {
            if (m_OnViewModeChanged) m_OnViewModeChanged(ContentViewMode::List);
        };
        viewModesItem->submenu.push_back(listItem);

        auto largeItem = std::make_shared<::we::runtime::kindui::MenuItem>();
        largeItem->label = "Large Icons";
        largeItem->icon = WindIcons::Square16;
        largeItem->onClick = [this]() {
            if (m_OnViewModeChanged) m_OnViewModeChanged(ContentViewMode::LargeIcons);
        };
        viewModesItem->submenu.push_back(largeItem);

        auto mediumItem = std::make_shared<::we::runtime::kindui::MenuItem>();
        mediumItem->label = "Medium Icons";
        mediumItem->icon = WindIcons::Square16;
        mediumItem->onClick = [this]() {
            if (m_OnViewModeChanged) m_OnViewModeChanged(ContentViewMode::MediumIcons);
        };
        viewModesItem->submenu.push_back(mediumItem);

        auto smallItem = std::make_shared<::we::runtime::kindui::MenuItem>();
        smallItem->label = "Small Icons";
        smallItem->icon = WindIcons::Square16;
        smallItem->onClick = [this]() {
            if (m_OnViewModeChanged) m_OnViewModeChanged(ContentViewMode::SmallIcons);
        };
        viewModesItem->submenu.push_back(smallItem);

        items.push_back(viewModesItem);

        // Content Options Submenu
        auto contentOptItem = std::make_shared<::we::runtime::kindui::MenuItem>();
        contentOptItem->label = "Content Options";
        contentOptItem->icon = WindIcons::ListFilter16;

        auto showFolders = std::make_shared<::we::runtime::kindui::MenuItem>();
        showFolders->label = "Show Folders";
        showFolders->isCheckable = true;
        showFolders->checked = true;
        contentOptItem->submenu.push_back(showFolders);

        auto showHidden = std::make_shared<::we::runtime::kindui::MenuItem>();
        showHidden->label = "Show Hidden Assets";
        showHidden->isCheckable = true;
        showHidden->checked = false;
        contentOptItem->submenu.push_back(showHidden);

        auto showEngine = std::make_shared<::we::runtime::kindui::MenuItem>();
        showEngine->label = "Show Engine Content";
        showEngine->isCheckable = true;
        showEngine->checked = false;
        contentOptItem->submenu.push_back(showEngine);

        auto showPlugin = std::make_shared<::we::runtime::kindui::MenuItem>();
        showPlugin->label = "Show Plugin Content";
        showPlugin->isCheckable = true;
        showPlugin->checked = false;
        contentOptItem->submenu.push_back(showPlugin);

        items.push_back(contentOptItem);

        showDropdownMenuBelow(m_SettingsBtn, items);
        if (m_OnSettingsClicked) m_OnSettingsClicked();
    });

    m_MoreBtn->SetOnClicked([this, showDropdownMenuBelow]() {
        std::vector<std::shared_ptr<::we::runtime::kindui::MenuItem>> items;

        auto refreshItem = std::make_shared<::we::runtime::kindui::MenuItem>();
        refreshItem->label = "Refresh";
        refreshItem->icon = WindIcons::Refresh16;
        items.push_back(refreshItem);

        auto expandItem = std::make_shared<::we::runtime::kindui::MenuItem>();
        expandItem->label = "Expand All";
        expandItem->icon = WindIcons::ChevronDown16;
        expandItem->onClick = [this]() {
            if (m_OnExpandAllClicked) m_OnExpandAllClicked();
        };
        items.push_back(expandItem);

        auto collapseItem = std::make_shared<::we::runtime::kindui::MenuItem>();
        collapseItem->label = "Collapse All";
        collapseItem->icon = WindIcons::ChevronUp16;
        collapseItem->onClick = [this]() {
            if (m_OnCollapseAllClicked) m_OnCollapseAllClicked();
        };
        items.push_back(collapseItem);

        auto sep = std::make_shared<::we::runtime::kindui::MenuItem>();
        sep->label = "";
        items.push_back(sep);

        auto dockItem = std::make_shared<::we::runtime::kindui::MenuItem>();
        dockItem->label = "Dock in Layout";
        dockItem->icon = WindIcons::Window16;
        items.push_back(dockItem);

        auto newTabItem = std::make_shared<::we::runtime::kindui::MenuItem>();
        newTabItem->label = "Open in New Tab";
        newTabItem->icon = WindIcons::Plus16;
        items.push_back(newTabItem);

        showDropdownMenuBelow(m_MoreBtn, items);
        if (m_OnMoreClicked) m_OnMoreClicked();
    });

    m_CreateBtn->SetOnClicked([this, showDropdownMenuBelow]() {
        std::vector<std::shared_ptr<::we::runtime::kindui::MenuItem>> items;

        auto importAsset = std::make_shared<::we::runtime::kindui::MenuItem>();
        importAsset->label = "Import Asset...";
        importAsset->icon = WindIcons::FolderCreate16;
        importAsset->onClick = [this]() {
            if (m_OnImportClicked) m_OnImportClicked();
        };
        items.push_back(importAsset);

        auto sep1 = std::make_shared<::we::runtime::kindui::MenuItem>();
        sep1->label = "";
        items.push_back(sep1);

        auto newFolder = std::make_shared<::we::runtime::kindui::MenuItem>();
        newFolder->label = "New Folder";
        newFolder->icon = WindIcons::FolderCreate16;
        items.push_back(newFolder);

        auto sep2 = std::make_shared<::we::runtime::kindui::MenuItem>();
        sep2->label = "";
        items.push_back(sep2);

        auto bpClass = std::make_shared<::we::runtime::kindui::MenuItem>();
        bpClass->label = "Blueprint Class";
        bpClass->icon = WindIcons::Blueprint16;
        items.push_back(bpClass);

        auto material = std::make_shared<::we::runtime::kindui::MenuItem>();
        material->label = "Material";
        material->icon = WindIcons::ColorPalette16;
        items.push_back(material);

        auto particle = std::make_shared<::we::runtime::kindui::MenuItem>();
        particle->label = "Particle System";
        particle->icon = WindIcons::Sun16;
        items.push_back(particle);

        auto sound = std::make_shared<::we::runtime::kindui::MenuItem>();
        sound->label = "Sound Cue";
        sound->icon = WindIcons::Speaker16;
        items.push_back(sound);

        auto level = std::make_shared<::we::runtime::kindui::MenuItem>();
        level->label = "Level";
        level->icon = WindIcons::Globe16;
        items.push_back(level);

        showDropdownMenuBelow(m_CreateBtn, items);
        if (m_OnCreateClicked) m_OnCreateClicked();
    });

    AddChild(m_CreateBtn);
    AddChild(m_ImportBtn);
    AddChild(m_SaveBtn);
    AddChild(MakeToolbarDivider());
    AddChild(m_BackBtn);
    AddChild(m_ForwardBtn);
    AddChild(m_FolderBtn);
    AddChild(m_Breadcrumb);
    AddChild(MakeToolbarDivider());
    AddChild(m_SettingsBtn);
    AddChild(m_MoreBtn);
}

Size ContentBrowserToolbarControls::Measure(const Size& availableSize) {
    Size size = Row::Measure(availableSize);
    const float uiScale = (std::max)(1.0f, DPIContext::GetScale());
    size.height = (std::max)(28.0f * uiScale, PanelChrome::ToolbarRowHeight());
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
    context.DrawSurface(m_Geometry, ::we::runtime::kindui::SurfaceRole::Panel, 0.0f, "ContentBrowserToolbarControls");
    Row::Paint(context);

    if (m_DrawBottomBorder) {
        const float uiScale = (std::max)(1.0f, DPIContext::GetScale());
        const float borderH = 1.0f * uiScale;
        const Color borderColor = we::runtime::kindui::ResolveColor(ColorToken::Separator);
        context.DrawRect(Rect{ m_Geometry.x, m_Geometry.y + m_Geometry.height - borderH, m_Geometry.width, borderH }, borderColor);
    }

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

void ContentBrowserToolbarControls::SetOnExpandAllClicked(std::function<void()> callback) {
    m_OnExpandAllClicked = std::move(callback);
}

void ContentBrowserToolbarControls::SetOnCollapseAllClicked(std::function<void()> callback) {
    m_OnCollapseAllClicked = std::move(callback);
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

}

