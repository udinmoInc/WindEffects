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

#include <KindUI/EditorUI.h>
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
    DetailsChromeRegionWidget() {
        // Hidden after unselect; must still Tick so SyncVisibility can show chrome again.
        SetTicksWhenHidden(true);
    }

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
        // SetVisible already RequestLayout + RequestPaint for height 0 ↔ content.
        SetVisible(active);
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

class SearchToolbarWrapper final : public DetailsChromeRegionWidget {
public:
    explicit SearchToolbarWrapper(IDetailsView* details) {
        SetDetails(details);
        m_Toolbar = std::make_shared<PanelToolbarRow>();
        m_Toolbar->SetFlexShrink(0.0f);
        m_Toolbar->SetOnSearchChanged([details](const std::string& text) {
            if (details) {
                details->SetSearchText(text);
            }
        });
        m_Toolbar->AddIconButton(WindIcons::Star16, []() {});
        m_Toolbar->AddIconButton(WindIcons::Settings16, []() {});
        m_Toolbar->Finalize();
        AddChild(m_Toolbar);
        SyncVisibility();
    }

    Size Measure(const Size& availableSize) override {
        if (!IsVisible()) {
            m_DesiredSize = Size{ availableSize.width, 0.0f };
            return m_DesiredSize;
        }
        m_DesiredSize = m_Toolbar->Measure(availableSize);
        return m_DesiredSize;
    }

    void Arrange(const Rect& allottedRect) override {
        m_Geometry = allottedRect;
        if (IsVisible()) {
            m_Toolbar->Arrange(allottedRect);
        }
    }

    void Paint(PaintContext& context) override {
        if (!IsVisible()) return;
        m_Toolbar->Paint(context);
    }

private:
    std::shared_ptr<PanelToolbarRow> m_Toolbar;
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
        // SetVisible already arms layout+paint; no extra InvalidateLayout.
        if (m_PropertyList) m_PropertyList->SetVisible(hasSelection);
        if (m_EmptyState) m_EmptyState->SetVisible(!hasSelection);
    }

    IDetailsView* m_Details = nullptr;
    std::shared_ptr<Widget> m_PropertyList;
    std::shared_ptr<we::runtime::kindui::EmptyState> m_EmptyState;
    bool m_HasSelection = true;
};

}

void PopulateDetailsPanelRegions(
    we::editor::dsl::PanelContext& p,
    const std::shared_ptr<Widget>& propertyList,
    IDetailsView* details)
{
    auto objectHeader = std::make_shared<ObjectTitleHeaderWrapper>(details);
    objectHeader->SetFlexShrink(0.0f);

    auto toolbar = std::make_shared<SearchToolbarWrapper>(details);
    toolbar->SetFlexShrink(0.0f);

    auto emptyState = MakeEmptyState(
        "Inspector",
        "Select an object to view and edit its properties");

    auto propertyContent = std::make_shared<DetailsContentRegion>(details, propertyList, emptyState);
    propertyContent->SetFlexGrow(1.0f);
    propertyContent->SetFlexShrink(1.0f);

    const float uiScale = (std::max)(1.0f, DPIContext::GetScale());
    const float rowGap = ResolveMetric(MetricToken::Space1) * uiScale;

    auto mainColumn = std::make_shared<Column>();
    mainColumn->SetFlexGrow(1.0f);
    mainColumn->SetFlexShrink(1.0f);
    mainColumn->Gap(0.0f);
    mainColumn->Padding(Margin{ 0.0f, 0.0f, 0.0f, 0.0f });

    mainColumn->AddChild(objectHeader);
    mainColumn->AddChild(toolbar);
    mainColumn->AddChild(propertyContent);

    p.Content(mainColumn);
}

}
}
