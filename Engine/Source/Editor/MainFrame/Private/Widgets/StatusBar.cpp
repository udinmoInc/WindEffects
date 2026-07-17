#include "Widgets/StatusBar.h"
#include "Widgets/CommandInput.h"
#include "Widgets/ToolButton.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Theming/StyleRole.h"
#include "KindUI/Core/Icon.h"
#include "KindUI/Core/DPIContext.h"
#include <algorithm>

using ::we::runtime::kindui::ColorToken;
using ::we::runtime::kindui::MetricToken;
using ::we::runtime::kindui::PaddingToken;

namespace we::editor::shell {
using ::we::runtime::kindui::MouseButton;
using ::we::runtime::kindui::KeyEventType;
using ::we::runtime::kindui::IconPainter;
namespace Icons = ::we::runtime::kindui::Icons;
namespace IconMetrics = ::we::runtime::kindui::IconMetrics;


namespace {

    std::shared_ptr<ToolButton> MakeFooterControl(
        const char* iconName,
        const std::string& label,
        bool isDropdown,
        const char* tooltip)
    {
        auto button = std::make_shared<ToolButton>(iconName, label, nullptr, tooltip);
        button->SetButtonStyle(ToolButtonStyle::ToolbarInline);
        button->SetIsDropdown(isDropdown);
        button->SetVerticalAlignment(VerticalAlignment::Center);
        return button;
    }

    float UiScale() {
        return std::max(1.0f, DPIContext::GetScale());
    }

} // namespace

StatusBar::StatusBar() = default;

void StatusBar::Construct() {
    m_LeftBox = std::make_shared<Row>();
    m_LeftBox->Gap(ThemeMetric(MetricToken::Space1));

    m_AssetsPanelButton = MakeFooterControl(Icons::ContentBrowserName, "Content Drawer", false, "Content Browser");
    m_DiagnosticsPanelButton = MakeFooterControl(Icons::OutputLogName, "Output Log", false, "Output Log");

    m_AssetsPanelButton->SetOnClicked([this]() { SelectPanelTab(0, true); });
    m_DiagnosticsPanelButton->SetOnClicked([this]() { SelectPanelTab(1, true); });

    m_LeftBox->AddChild(m_AssetsPanelButton);
    m_LeftBox->AddChild(m_DiagnosticsPanelButton);
    AddChild(m_LeftBox);

    m_CommandInput = std::make_shared<CommandInput>();
    m_CommandInput->SetVerticalAlignment(VerticalAlignment::Center);
    m_CommandInput->SetPlaceholder("Console Commands...");
    AddChild(m_CommandInput);

    m_RightBox = std::make_shared<Row>();
    m_RightBox->Gap(ThemeMetric(MetricToken::Space1));

    m_OutputLogButton = MakeFooterControl(Icons::BuildName, "Source Control", false, "Source Control");
    m_BuildMenuButton = MakeFooterControl(Icons::ProfilerName, "FPS", false, "Frame Rate");
    m_TraceButton = MakeFooterControl(Icons::PackageName, "Memory", false, "Memory Usage");
    m_QualityMenuButton = MakeFooterControl(Icons::LitName, "RHI", false, "Graphics API");

    m_RightBox->AddChild(m_OutputLogButton);
    m_RightBox->AddChild(m_BuildMenuButton);
    m_RightBox->AddChild(m_TraceButton);
    m_RightBox->AddChild(m_QualityMenuButton);

    AddChild(m_RightBox);

    SelectPanelTab(0, false);
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
    const float padH = ThemeMetric(MetricToken::Space2) * uiScale;
    const float sectionGap = ThemeMetric(MetricToken::Space2) * uiScale;
    const Size sectionAvailable{
        std::max(0.0f, availableSize.width - padH * 2.0f),
        m_Height
    };

    if (m_LeftBox) {
        m_LeftBox->Measure(sectionAvailable);
    }
    if (m_RightBox) {
        m_RightBox->Measure(sectionAvailable);
    }
    if (m_CommandInput) {
        m_CommandInput->Measure(sectionAvailable);
    }

    m_DesiredSize = Size{ availableSize.width, m_Height };
    (void)sectionGap;
    return m_DesiredSize;
}

void StatusBar::Arrange(const Rect& allottedRect) {
    const float uiScale = UiScale();
    const float padL = ThemeMetric(MetricToken::Space2) * uiScale;
    const float padR = padL;
    const float sectionGap = ThemeMetric(MetricToken::Space2) * uiScale;
    const float minCommandWidth = 120.0f * uiScale;

    const float barHeight = std::min(m_Height, allottedRect.height);
    const float barY = allottedRect.y + allottedRect.height - barHeight;
    m_Geometry = Rect{ allottedRect.x, barY, allottedRect.width, barHeight };

    const float contentX = m_Geometry.x + padL;
    const float contentY = m_Geometry.y;
    const float contentH = barHeight;
    const float contentW = std::max(0.0f, m_Geometry.width - padL - padR);
    const Size sectionAvailable{ contentW, contentH };

    if (m_LeftBox) {
        m_LeftBox->Measure(sectionAvailable);
    }
    if (m_RightBox) {
        m_RightBox->Measure(sectionAvailable);
    }

    const float leftW = m_LeftBox ? m_LeftBox->GetDesiredSize().width : 0.0f;
    const float rightW = m_RightBox ? m_RightBox->GetDesiredSize().width : 0.0f;

    const float fixedW = leftW + rightW + sectionGap * 2.0f;
    float commandW = std::max(0.0f, contentW - fixedW);
    if (commandW > 0.0f) {
        commandW = std::max(minCommandWidth, commandW);
        if (leftW + sectionGap + commandW + sectionGap + rightW > contentW) {
            commandW = std::max(0.0f, contentW - leftW - rightW - sectionGap * 2.0f);
        }
    }

    float x = contentX;
    if (m_LeftBox) {
        m_LeftBox->Arrange(Rect{ x, contentY, leftW, contentH });
        x += leftW;
        if (commandW > 0.0f || rightW > 0.0f) {
            x += sectionGap;
        }
    }

    if (m_CommandInput && commandW > 0.0f) {
        m_CommandInput->Arrange(Rect{ x, contentY, commandW, contentH });
        x += commandW + sectionGap;
    }

    if (m_RightBox) {
        const float rightX = std::max(x, contentX + contentW - rightW);
        m_RightBox->Arrange(Rect{ rightX, contentY, std::min(rightW, contentW), contentH });
    }
}

void StatusBar::Paint(PaintContext& context) {
    context.DrawRect(m_Geometry, ThemeColor(ColorToken::StatusBarBackground));

    Rect topBorder{
        m_Geometry.x,
        m_Geometry.y,
        m_Geometry.width,
        ThemeMetric(MetricToken::BorderWidth)
    };
    context.DrawRect(topBorder, ThemeColor(ColorToken::Separator));

    if (m_LeftBox && m_LeftBox->IsVisible()) {
        m_LeftBox->Paint(context);
    }
    if (m_CommandInput && m_CommandInput->IsVisible()) {
        m_CommandInput->Paint(context);
    }
    if (m_RightBox && m_RightBox->IsVisible()) {
        m_RightBox->Paint(context);
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