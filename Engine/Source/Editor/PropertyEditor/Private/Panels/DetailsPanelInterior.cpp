// ==============================================================================
// WindEffects — PropertyEditor — DetailsPanelInterior
// Internal implementation for the PropertyEditor module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "PropertyEditor/PropertyEditorSession.h"
#include "PropertyEditor/IDetailsView.h"
#include "PropertyEditorInternal.h"

#include "KindUI/EditorWidgets.h"
#include "KindUI/Widgets/ObjectTitleBar.h"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

namespace we::editor::property {
namespace detail {
namespace {

using namespace we::runtime::kindui;
using ::we::runtime::kindui::panels::Panel;

[[nodiscard]] bool HasDetailsSelection(const IDetailsView* details) {
    return details && details->HasSelection();
}

class DetailsChromeRegionWidget : public Widget {
public:
    void SetDetails(IDetailsView* details) { m_Details = details; }

    void Tick(float deltaTime) override {
        Widget::Tick(deltaTime);
        SyncVisibility();
    }

protected:
    void SyncVisibility() {
        const bool active = HasDetailsSelection(m_Details);
        if (active == IsVisible()) {
            return;
        }
        SetVisible(active);
        InvalidateLayout();
    }

    IDetailsView* m_Details = nullptr;
};

class ObjectTitleHeaderWrapper final : public DetailsChromeRegionWidget {
public:
    explicit ObjectTitleHeaderWrapper(IDetailsView* details) {
        SetDetails(details);
        m_TitleBar = std::make_shared<ObjectTitleBar>();
        m_TitleBar->SetFlexShrink(0.0f);
        AddChild(m_TitleBar);
    }

    void Tick(float deltaTime) override {
        DetailsChromeRegionWidget::Tick(deltaTime);
        if (!m_Details || !IsVisible()) return;

        const std::string title = m_Details->GetObjectTitle();
        const auto icon = m_Details->GetObjectIcon();
        if (title != m_LastTitle || !(icon == m_LastIcon)) {
            m_LastTitle = title;
            m_LastIcon = icon;
            m_TitleBar->SetTitle(title);
            m_TitleBar->SetIcon(icon);
        }
    }

    Size Measure(const Size& availableSize) override {
        if (!IsVisible()) {
            m_DesiredSize = Size{ availableSize.width, 0.0f };
            return m_DesiredSize;
        }
        m_DesiredSize = m_TitleBar->Measure(availableSize);
        return m_DesiredSize;
    }

    void Arrange(const Rect& allottedRect) override {
        m_Geometry = allottedRect;
        m_TitleBar->Arrange(allottedRect);
    }

    void Paint(PaintContext& context) override {
        if (!IsVisible()) return;
        m_TitleBar->Paint(context);
    }

private:
    std::shared_ptr<ObjectTitleBar> m_TitleBar;
    std::string m_LastTitle;
    WindIconRef m_LastIcon = kWindIconNone;
};

class CategoryFilterTabsWrapper final : public DetailsChromeRegionWidget {
public:
    explicit CategoryFilterTabsWrapper(IDetailsView* details) {
        SetDetails(details);
        m_TabStrip = std::make_shared<FilterTabStrip>();
        m_TabStrip->SetOnTabSelected([details](const std::string& category) {
            if (details) {
                details->SetActiveCategory(category);
            }
        });
        AddChild(m_TabStrip);
    }

    void Tick(float deltaTime) override {
        DetailsChromeRegionWidget::Tick(deltaTime);
        if (!m_Details || !IsVisible()) return;

        const auto categories = m_Details->GetCategoryNames();
        if (categories != m_LastCategories) {
            m_LastCategories = categories;
            m_TabStrip->SetTabs(categories);
        }
        m_TabStrip->SetActiveTab(m_Details->GetActiveCategory());
    }

    Size Measure(const Size& availableSize) override {
        if (!IsVisible()) {
            m_DesiredSize = Size{ availableSize.width, 0.0f };
            return m_DesiredSize;
        }
        m_DesiredSize = m_TabStrip->Measure(availableSize);
        return m_DesiredSize;
    }

    void Arrange(const Rect& allottedRect) override {
        m_Geometry = allottedRect;
        m_TabStrip->Arrange(allottedRect);
    }

    void Paint(PaintContext& context) override {
        if (!IsVisible()) return;
        m_TabStrip->Paint(context);
    }

private:
    std::shared_ptr<FilterTabStrip> m_TabStrip;
    std::vector<std::string> m_LastCategories;
};

class SubOutlinerTreeWrapper final : public DetailsChromeRegionWidget {
public:
    explicit SubOutlinerTreeWrapper(IDetailsView* details) {
        SetDetails(details);
        m_Tree = std::make_shared<CompactTreeWidget>();
        m_Tree->SetOnItemClicked([details](const CompactTreeNode& item) {
            if (details) {
                details->SetActiveCategory(item.category);
            }
        });
        AddChild(m_Tree);
        RebuildItems();
    }

    void Tick(float deltaTime) override {
        DetailsChromeRegionWidget::Tick(deltaTime);
        if (!m_Details || !IsVisible()) return;

        RebuildItems();
        m_Tree->SetActiveCategory(m_Details->GetActiveCategory());
    }

    Size Measure(const Size& availableSize) override {
        if (!IsVisible()) {
            m_DesiredSize = Size{ availableSize.width, 0.0f };
            return m_DesiredSize;
        }
        m_DesiredSize = m_Tree->Measure(availableSize);
        return m_DesiredSize;
    }

    void Arrange(const Rect& allottedRect) override {
        m_Geometry = allottedRect;
        m_Tree->Arrange(allottedRect);
    }

    void Paint(PaintContext& context) override {
        if (!IsVisible()) return;
        m_Tree->Paint(context);
    }

private:
    void RebuildItems() {
        if (!m_Details) return;
        const std::string title = m_Details->GetObjectTitle();
        const auto icon = m_Details->GetObjectIcon();
        const std::string rootTitle = title.empty() ? "Actor (Self)" : title + " (Self)";

        if (rootTitle != m_LastTitle) {
            m_LastTitle = rootTitle;
            std::vector<CompactTreeNode> items;
            items.push_back({
                "root",
                rootTitle,
                "",
                "",
                icon.IsValid() ? icon : WindIcons::Folder16,
                0,
                false,
                true
            });
            m_Tree->SetItems(items);
        }
    }

    std::shared_ptr<CompactTreeWidget> m_Tree;
    std::string m_LastTitle;
};

class DetailsContentRegion final : public Column {
public:
    DetailsContentRegion(
        IDetailsView* details,
        const std::shared_ptr<Widget>& propertyList,
        const std::shared_ptr<we::runtime::kindui::EmptyState>& emptyState)
        : m_Details(details), m_PropertyList(propertyList), m_EmptyState(emptyState) {

        if (propertyList) {
            propertyList->SetFlexGrow(1.0f);
            propertyList->SetFlexShrink(1.0f);
            AddChild(propertyList);
        }
        emptyState->SetFlexGrow(1.0f);
        emptyState->SetFlexShrink(1.0f);
        AddChild(emptyState);
        SyncVisibility();
    }

    void Tick(float deltaTime) override {
        Column::Tick(deltaTime);
        SyncVisibility();
    }

private:
    void SyncVisibility() {
        const bool hasSelection = HasDetailsSelection(m_Details);
        if (hasSelection == m_HasSelection) {
            return;
        }
        m_HasSelection = hasSelection;
        if (m_PropertyList) m_PropertyList->SetVisible(hasSelection);
        if (m_EmptyState) m_EmptyState->SetVisible(!hasSelection);
        InvalidateLayout();
    }

    IDetailsView* m_Details = nullptr;
    std::shared_ptr<Widget> m_PropertyList;
    std::shared_ptr<we::runtime::kindui::EmptyState> m_EmptyState;
    bool m_HasSelection = true;
};

}

std::shared_ptr<Widget> CreateSubOutlinerWidget(IDetailsView* details) {
    auto subOutliner = std::make_shared<SubOutlinerTreeWrapper>(details);
    subOutliner->SetFlexShrink(0.0f);
    return subOutliner;
}

void PopulateDetailsPanelRegions(
    we::editor::dsl::PanelContext& p,
    const std::shared_ptr<Widget>& propertyList,
    IDetailsView* details)
{
    auto objectHeader = std::make_shared<ObjectTitleHeaderWrapper>(details);
    objectHeader->SetFlexShrink(0.0f);

    auto subOutliner = std::make_shared<SubOutlinerTreeWrapper>(details);
    subOutliner->SetFlexShrink(0.0f);

    auto toolbar = std::make_shared<PanelToolbarRow>();
    toolbar->SetFlexShrink(0.0f);
    toolbar->AddLeadingIconButton(WindIcons::ListFilter16, []() {});
    toolbar->SetOnSearchChanged([details](const std::string& text) {
        if (details) {
            details->SetSearchText(text);
        }
    });
    toolbar->AddIconButton(WindIcons::Star16, []() {});
    toolbar->AddIconButton(WindIcons::Settings16, []() {});
    toolbar->Finalize();

    auto categoryTabs = std::make_shared<CategoryFilterTabsWrapper>(details);
    categoryTabs->SetFlexShrink(0.0f);

    auto emptyState = MakeEmptyState(
        "Inspector",
        "Select an object to view and edit its properties");

    auto propertyContent = std::make_shared<DetailsContentRegion>(details, propertyList, emptyState);
    propertyContent->SetFlexGrow(1.0f);
    propertyContent->SetFlexShrink(1.0f);

    auto mainColumn = std::make_shared<Column>();
    mainColumn->SetFlexGrow(1.0f);
    mainColumn->SetFlexShrink(1.0f);

    mainColumn->AddChild(objectHeader);
    mainColumn->AddChild(subOutliner);
    mainColumn->AddChild(toolbar);
    mainColumn->AddChild(categoryTabs);
    mainColumn->AddChild(propertyContent);

    p.Content(mainColumn);
}

}
}
