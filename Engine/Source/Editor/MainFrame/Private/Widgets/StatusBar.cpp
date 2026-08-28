#include "Platform/Platform.h"
#include "Widgets/StatusBar.h"
#include "Widgets/CommandInput.h"
#include "Widgets/ToolButton.h"
#include "KindUI/Layout/Spacer.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Theming/StyleRole.h"
#include "KindUI/Core/Icon.h"
#include "KindUI/Core/DPIContext.h"
#include <algorithm>

using ::we::runtime::kindui::ColorToken;
using ::we::runtime::kindui::MetricToken;
using ::we::runtime::kindui::PaddingToken;
using ::we::runtime::kindui::Spacer;

namespace we::editor::shell {
using ::we::editor::toolbar::ToolButton;
using ::we::editor::toolbar::ToolButtonStyle;
using ::we::runtime::kindui::VerticalAlignment;
using ::we::runtime::kindui::DPIContext;
using ::we::runtime::kindui::Margin;
using ::we::runtime::kindui::AlignItems;
namespace Icons = ::we::runtime::kindui::Icons;

namespace {

    std::shared_ptr<ToolButton> MakeFooterControl(
        const char* iconName,
        const std::string& label,
        bool isDropdown,
        const char* tooltip)
    {
        auto button = std::make_shared<ToolButton>(iconName, label, nullptr, tooltip);
        button->SetButtonStyle(ToolButtonStyle::ToolbarInline);
        button->SetChromeless(true);
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
    const float uiScale = UiScale();
    const float pad = ThemeMetric(MetricToken::Space2) * uiScale;
    Padding(Margin{pad, 0, pad, 0});
    Gap(ThemeMetric(MetricToken::Space2) * uiScale);
    Align(AlignItems::Center);

    m_LeftBox = std::make_shared<Row>();
    m_LeftBox->Gap(ThemeMetric(MetricToken::Space1));
    m_LeftBox->SetFlexShrink(0.0f);

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
    m_CommandInput->SetFlexGrow(1.0f);
    m_CommandInput->SetFlexShrink(1.0f);
    m_CommandInput->SetMaxWidth(300.0f * uiScale);
    AddChild(m_CommandInput);

    auto spacer = std::make_shared<Spacer>();
    spacer->SetFlexGrow(1.0f);
    AddChild(spacer);

    m_RightBox = std::make_shared<Row>();
    m_RightBox->Gap(ThemeMetric(MetricToken::Space1));
    m_RightBox->SetFlexShrink(0.0f);

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
    Size size = Row::Measure(availableSize);
    size.height = m_Height;
    m_DesiredSize = size;
    return m_DesiredSize;
}

void StatusBar::Arrange(const Rect& allottedRect) {
    const float barHeight = std::min(m_Height, allottedRect.height);
    const float barY = allottedRect.y + allottedRect.height - barHeight;
    m_Geometry = Rect{ allottedRect.x, barY, allottedRect.width, barHeight };
    Row::Arrange(m_Geometry);
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
    Row::Paint(context);
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

