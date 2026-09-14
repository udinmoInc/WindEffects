// ==============================================================================
// WindEffects — MainFrame — StatusBar
// UI widget used by the MainFrame module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "Platform/Platform.h"
#include "Widgets/StatusBar.h"
#include "Widgets/CommandInput.h"
#include "Widgets/ToolButton.h"
#include <KindUI/EditorUI.h>
#include "KindUI/Diagnostics/UiGeometryDebug.h"
#include <algorithm>

using ::we::runtime::kindui::ColorToken;
using ::we::runtime::kindui::MetricToken;
using ::we::runtime::kindui::PaddingToken;
using ::we::runtime::kindui::Spacer;
using ::we::runtime::kindui::VerticalDivider;

namespace we::editor::shell {
using ::we::editor::toolbar::ToolButton;
using ::we::editor::toolbar::ToolButtonStyle;
using ::we::runtime::kindui::VerticalAlignment;
using ::we::runtime::kindui::DPIContext;
using ::we::runtime::kindui::Margin;
using ::we::runtime::kindui::AlignItems;
namespace WindIcons = ::we::runtime::kindui::WindIcons;
using ::we::runtime::kindui::kWindIconNone;
namespace ControlChrome = ::we::runtime::kindui::ControlChrome;

namespace {

    std::shared_ptr<ToolButton> MakeDockControl(
        we::runtime::kindui::WindIconRef icon,
        const std::string& label,
        const char* tooltip)
    {
        auto button = std::make_shared<ToolButton>(icon, label, nullptr, tooltip);
        button->SetButtonStyle(ToolButtonStyle::StatusBar);
        button->SetVerticalAlignment(VerticalAlignment::Center);
        button->SetAllowsPaintRetention(false);
        return button;
    }

    float UiScale() {
        return std::max(1.0f, DPIContext::GetScale());
    }

    std::shared_ptr<VerticalDivider> MakeStatusDivider() {
        auto divider = std::make_shared<VerticalDivider>();
        divider->SetFlexShrink(0.0f);
        divider->SetHeightRatio(1.0f);
        divider->SetVerticalAlignment(VerticalAlignment::Fill);
        divider->SetAllowsPaintRetention(false);
        return divider;
    }

    void DisablePaintRetentionRecursive(const std::shared_ptr<Widget>& widget) {
        if (!widget) {
            return;
        }
        widget->SetAllowsPaintRetention(false);
        for (const auto& child : widget->GetChildren()) {
            DisablePaintRetentionRecursive(child);
        }
    }

} // namespace

StatusBar::StatusBar()
    : m_Height(we::runtime::kindui::ResolveMetric(MetricToken::StatusBarHeight))
{
    SetAllowsPaintRetention(false);
}

StatusBar::~StatusBar() = default;

void StatusBar::Construct() {
    const float uiScale = UiScale();
    const float padH = ThemeMetric(MetricToken::Space3) * uiScale;
    Padding(Margin{ padH, 0, padH, 0 });
    Gap(ThemeMetric(MetricToken::Space2) * uiScale);
    Align(AlignItems::Center);

    // Left controls
    m_AssetsPanelButton = MakeDockControl(WindIcons::FolderSearch16, "Asset Explorer", "Asset Explorer");
    m_DiagnosticsPanelButton = MakeDockControl(WindIcons::Console16, "Output Log", "Output Log");

    m_AssetsPanelButton->SetOnClicked([this]() { SelectPanelTab(0, true); });
    m_DiagnosticsPanelButton->SetOnClicked([this]() { SelectPanelTab(1, true); });

    m_Divider1 = MakeStatusDivider();
    m_Divider2 = MakeStatusDivider();

    AddChild(m_AssetsPanelButton);
    AddChild(m_Divider1);
    AddChild(m_DiagnosticsPanelButton);
    AddChild(m_Divider2);

    // Command input field
    m_CommandInput = std::make_shared<CommandInput>();
    m_CommandInput->SetFlatChrome(true);
    m_CommandInput->SetVerticalAlignment(VerticalAlignment::Center);
    m_CommandInput->SetPlaceholder("Output Log Commands...");
    m_CommandInput->SetFlexGrow(0.0f);
    m_CommandInput->SetFlexShrink(0.0f);
    m_CommandInput->SetWidth(ThemeMetric(MetricToken::InputWidthDefault) * uiScale);
    m_CommandInput->SetHeight(ThemeMetric(MetricToken::ControlHeightCompact) * uiScale);
    m_CommandInput->SetAllowsPaintRetention(false);
    AddChild(m_CommandInput);

    m_Divider3 = MakeStatusDivider();
    AddChild(m_Divider3);

    // Right status controls
    m_OutputLogButton = MakeDockControl(WindIcons::GitPullRequestDraft16, "Source Control", "Source Control");
    m_BuildMenuButton = MakeDockControl(WindIcons::Fps16, "FPS", "Frame Rate");
    m_TraceButton = MakeDockControl(WindIcons::Database16, "Cache", "Cache Usage");
    m_QualityMenuButton = MakeDockControl(WindIcons::Rhi16, "RHI", "Graphics API");

    m_Divider4 = MakeStatusDivider();
    m_Divider5 = MakeStatusDivider();
    m_Divider6 = MakeStatusDivider();

    AddChild(m_OutputLogButton);
    AddChild(m_Divider4);
    AddChild(m_BuildMenuButton);
    AddChild(m_Divider5);
    AddChild(m_TraceButton);
    AddChild(m_Divider6);
    AddChild(m_QualityMenuButton);

    SelectPanelTab(0, false);
    DisablePaintRetentionRecursive(shared_from_this());
}

void StatusBar::SelectPanelTab(int index, bool notify) {
    if (index < 0 || index > 1) {
        return;
    }

    m_ActivePanelTab = index;
    if (m_AssetsPanelButton) {
        m_AssetsPanelButton->SetActive(index == 0);
    }
    if (m_DiagnosticsPanelButton) {
        m_DiagnosticsPanelButton->SetActive(index == 1);
    }

    if (notify && m_OnFooterTabChanged) {
        m_OnFooterTabChanged(index);
    }
}

Size StatusBar::Measure(const Size& availableSize) {
    const float uiScale = UiScale();
    m_Height = we::runtime::kindui::ResolveMetric(MetricToken::StatusBarHeight) * uiScale;
    Size size = Row::Measure(availableSize);
    size.height = m_Height;
    m_DesiredSize = size;
    return m_DesiredSize;
}

void StatusBar::Arrange(const Rect& allottedRect) {
    const float uiScale = UiScale();
    const float padH = ThemeMetric(MetricToken::Space3) * uiScale;
    const float gap = ThemeMetric(MetricToken::Space2) * uiScale;

    const float barHeight = (m_Height > 0.0f)
        ? (std::min)(m_Height, allottedRect.height)
        : allottedRect.height;
    const float barY = allottedRect.y + allottedRect.height - barHeight;

    Rect barRect{ allottedRect.x, barY, allottedRect.width, barHeight };
    CommitGeometry(barRect);

    // Measure children to get exact widths for flat positioning
    const Size contentAvail{ (std::max)(0.0f, allottedRect.width - padH * 2.0f), barHeight };

    const auto measureChild = [&](const std::shared_ptr<Widget>& w) -> Size {
        if (!w || !w->IsVisible()) return Size{ 0.0f, 0.0f };
        return w->Measure(contentAvail);
    };

    Size assetSz = measureChild(m_AssetsPanelButton);
    Size diagSz = measureChild(m_DiagnosticsPanelButton);
    Size inputSz = measureChild(m_CommandInput);
    Size sourceControlSz = measureChild(m_OutputLogButton);
    Size fpsSz = measureChild(m_BuildMenuButton);
    Size cacheSz = measureChild(m_TraceButton);
    Size rhiSz = measureChild(m_QualityMenuButton);
    const float divWidth = 1.0f;

    // Arrange Left Group: AssetsPanel -> Divider -> DiagnosticsPanel -> Divider -> CommandInput -> Divider
    float leftX = allottedRect.x + padH;
    const auto arrangeWidget = [&](const std::shared_ptr<Widget>& w, float x, float width, float height) {
        if (!w || !w->IsVisible()) return;
        const float cy = barY + (barHeight - height) * 0.5f;
        w->Arrange(Rect{ x, cy, width, height });
    };

    arrangeWidget(m_AssetsPanelButton, leftX, assetSz.width, barHeight);
    leftX += assetSz.width + gap;

    arrangeWidget(m_Divider1, leftX, divWidth, barHeight);
    leftX += divWidth + gap;

    arrangeWidget(m_DiagnosticsPanelButton, leftX, diagSz.width, barHeight);
    leftX += diagSz.width + gap;

    arrangeWidget(m_Divider2, leftX, divWidth, barHeight);
    leftX += divWidth + gap;

    arrangeWidget(m_CommandInput, leftX, inputSz.width > 0.0f ? inputSz.width : (ThemeMetric(MetricToken::InputWidthDefault) * uiScale), barHeight);
    leftX += (inputSz.width > 0.0f ? inputSz.width : (ThemeMetric(MetricToken::InputWidthDefault) * uiScale)) + gap;

    arrangeWidget(m_Divider3, leftX, divWidth, barHeight);

    // Arrange Right Group: SourceControl <- Divider <- FPS <- Divider <- Cache <- Divider <- RHI (from right edge)
    float rightX = allottedRect.x + allottedRect.width - padH;

    const auto arrangeRightWidget = [&](const std::shared_ptr<Widget>& w, float width, float height) {
        if (!w || !w->IsVisible()) return;
        rightX -= width;
        const float cy = barY + (barHeight - height) * 0.5f;
        w->Arrange(Rect{ rightX, cy, width, height });
        rightX -= gap;
    };

    arrangeRightWidget(m_QualityMenuButton, rhiSz.width, barHeight);
    arrangeRightWidget(m_Divider6, divWidth, barHeight);
    arrangeRightWidget(m_TraceButton, cacheSz.width, barHeight);
    arrangeRightWidget(m_Divider5, divWidth, barHeight);
    arrangeRightWidget(m_BuildMenuButton, fpsSz.width, barHeight);
    arrangeRightWidget(m_Divider4, divWidth, barHeight);
    arrangeRightWidget(m_OutputLogButton, sourceControlSz.width, barHeight);

    InvalidateRetainedPaintForRepaint();
}

void StatusBar::Paint(PaintContext& context) {
    InvalidateRetainedPaintForRepaint();
    context.PushSurfaceOwner("StatusBar", we::runtime::kindui::SurfaceRole::StatusBar);
    context.DrawSurface(m_Geometry, we::runtime::kindui::SurfaceRole::StatusBar, 0.0f, "StatusBar");

    // Crisp top border line separating status bar from workspace
    const float borderThickness = std::max(1.0f, ThemeMetric(MetricToken::PanelDividerWidth));
    context.DrawRect(Rect{ m_Geometry.x, std::floor(m_Geometry.y), m_Geometry.width, borderThickness },
        ThemeColor(ColorToken::Separator));

    // Footer chips must paint live geometry every frame (no retained replay).
    const bool prevRetention = context.IsPaintRetentionEnabled();
    context.SetPaintRetentionEnabled(false);

    for (auto& child : GetChildren()) {
        if (child && child->IsVisible()) {
            child->Paint(context);
        }
    }

    context.SetPaintRetentionEnabled(prevRetention);
    context.PopSurfaceOwner();

    if (we::runtime::kindui::UiGeometryDebug::IsEnabled()) {
        we::runtime::kindui::UiGeometryDebug::Get().TraceRegion(
            "StatusBar",
            m_Geometry,
            "EditorShell",
            0.0f,
            0.0f,
            we::runtime::kindui::ResolveMetric(MetricToken::TextSizeSmall));
    }
}

void StatusBar::SetActiveFooterTab(int index) {
    SelectPanelTab(index, false);
}

void StatusBar::SetOnFooterTabChanged(std::function<void(int)> onChanged) {
    m_OnFooterTabChanged = std::move(onChanged);
}

void StatusBar::SetOnCommandSubmitted(std::function<void(const std::string&)> onSubmitted) {
    if (m_CommandInput) {
        m_CommandInput->SetOnCommandSubmitted(std::move(onSubmitted));
    }
}

void StatusBar::SetOnOutputLogClicked(std::function<void()> onClicked) {
    m_OnOutputLogClicked = std::move(onClicked);
    if (m_DiagnosticsPanelButton) {
        m_DiagnosticsPanelButton->SetOnClicked([this]() {
            SelectPanelTab(1, true);
            if (m_OnOutputLogClicked) {
                m_OnOutputLogClicked();
            }
        });
    }
}

void StatusBar::SetOnBuildMenuClicked(std::function<void()> onClicked) {
    if (m_BuildMenuButton) {
        m_BuildMenuButton->SetOnClicked(std::move(onClicked));
    }
}

void StatusBar::SetOnTraceClicked(std::function<void()> onClicked) {
    if (m_TraceButton) {
        m_TraceButton->SetOnClicked(std::move(onClicked));
    }
}

void StatusBar::SetOnQualityMenuClicked(std::function<void()> onClicked) {
    if (m_QualityMenuButton) {
        m_QualityMenuButton->SetOnClicked(std::move(onClicked));
    }
}

} // namespace we::editor::shell
