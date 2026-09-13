// ==============================================================================
// WindEffects — ContentBrowser — ContentBrowserToolbar
// Public API surface for the ContentBrowser module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "ContentBrowser/Models/ContentBrowserModel.h"
#include "KindUI/Core/Widget.h"
#include "KindUI/Layout/Flex.h"
#include "KindUI/Core/Style.h"
#include "KindUI/Core/Widgets/DesignSystemControls.h"
#include "KindUI/Core/Widgets/ToolbarIconButton.h"
#include "KindUI/Core/Widgets/ToolbarNavigationButton.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Theming/StyleRole.h"
#include "KindUI/Core/WindIcon.h"
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace we::editor::widgets { class SearchBox; }

namespace we::editor::contentbrowser {
using ::we::runtime::kindui::Widget;
using ::we::runtime::kindui::Size;
using ::we::runtime::kindui::Rect;
using ::we::runtime::kindui::Point;
using ::we::runtime::kindui::Color;
using ::we::runtime::kindui::PaintContext;
using ::we::runtime::kindui::MouseEvent;
using ::we::runtime::kindui::WidgetStyle;
using ::we::runtime::kindui::PrimaryButton;
using ::we::runtime::kindui::SecondaryButton;
using ::we::runtime::kindui::ToolbarNavigationButton;
using ::we::runtime::kindui::ToolbarIconButton;
using ::we::runtime::kindui::ToolbarButton;

class Breadcrumb;

// Premium AAA toolbar with reusable components.
class ContentBrowserToolbarControls : public we::runtime::kindui::Row {
public:
    enum class ToolbarMode {
        Full,
        AssetPane
    };

    static std::shared_ptr<ContentBrowserToolbarControls> Create(ToolbarMode mode = ToolbarMode::Full);

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;

    void OnMouseDown(const MouseEvent& event) override;
    void OnMouseUp(const MouseEvent& event) override;
    void OnMouseMove(const MouseEvent& event) override;

    std::shared_ptr<::we::editor::widgets::SearchBox> GetSearchBox() const { return m_SearchBox; }
    std::shared_ptr<Breadcrumb> GetBreadcrumb() const { return m_Breadcrumb; }
    std::shared_ptr<ToolbarIconButton> GetBackBtn() const { return m_BackBtn; }
    std::shared_ptr<ToolbarIconButton> GetForwardBtn() const { return m_ForwardBtn; }
    std::shared_ptr<ToolbarIconButton> GetFolderBtn() const { return m_FolderBtn; }

    void SetOnFilterClicked(std::function<void()> callback);
    void SetOnSortClicked(std::function<void()> callback);
    void SetOnImportClicked(std::function<void()> callback);
    void SetOnCreateClicked(std::function<void()> callback);
    void SetOnSaveClicked(std::function<void()> callback);
    void SetOnFabClicked(std::function<void()> callback);
    void SetOnPreviousClicked(std::function<void()> callback);
    void SetOnNextClicked(std::function<void()> callback);
    void SetOnFolderClicked(std::function<void()> callback);
    void SetOnViewModeChanged(std::function<void(ContentViewMode)> callback);
    void SetOnSettingsClicked(std::function<void()> callback);
    void SetOnMoreClicked(std::function<void()> callback);

private:
    ContentBrowserToolbarControls(ToolbarMode mode);
    void InitializeChildren();
    void ArrangeControlRow(const Rect& row, float contentLeft, float contentRight);

    ToolbarMode m_Mode;
    std::shared_ptr<Breadcrumb> m_Breadcrumb;
    std::shared_ptr<::we::editor::widgets::SearchBox> m_SearchBox;

    std::shared_ptr<ToolbarButton> m_CreateBtn;
    std::shared_ptr<ToolbarButton> m_ImportBtn;
    std::shared_ptr<ToolbarButton> m_SaveBtn;
    std::shared_ptr<ToolbarButton> m_FabBtn;
    std::shared_ptr<ToolbarIconButton> m_BackBtn;
    std::shared_ptr<ToolbarIconButton> m_ForwardBtn;
    std::shared_ptr<ToolbarIconButton> m_FolderBtn;

    // Legacy / secondary controls (for AssetPane mode)
    std::shared_ptr<ToolbarIconButton> m_GridViewBtn;
    std::shared_ptr<ToolbarIconButton> m_ListViewBtn;
    std::shared_ptr<ToolbarIconButton> m_SettingsBtn;
    std::shared_ptr<ToolbarIconButton> m_MoreBtn;
    std::shared_ptr<ToolbarIconButton> m_FilterIconBtn;
    std::shared_ptr<ToolbarButton> m_SortBtn;
    std::shared_ptr<ToolbarButton> m_FilterBtn;

    std::function<void(ContentViewMode)> m_OnViewModeChanged;
    std::function<void()> m_OnSettingsClicked;
    std::function<void()> m_OnMoreClicked;
    std::function<void()> m_OnCreateClicked;
    std::function<void()> m_OnImportClicked;
    std::function<void()> m_OnSaveClicked;
    std::function<void()> m_OnFilterClicked;
};

}
